/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier <tigersoldi@gmail.com>
 *
 * This file is part of OSD Lyrics.
 * 
 * OSD Lyrics is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OSD Lyrics is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OSD Lyrics.  If not, see <https://www.gnu.org/licenses/>. 
 */
#include <stdio.h>
#include <glib.h>
#include "ol_path_pattern.h"
#include "ol_utils.h"
#include "ol_debug.h"

#define BUFFER_SIZE 1024
#define MAX_PATH_LEN 1024

/** 
 * @brief duplicates the string and replace all invalid characters to `_' 
 * 
 * @param str string needs to be replaced
 * 
 * @return replaced string, should be free with g_free
 */
static char* replace_invalid_str (const char *str);

/** 
 * @brief Gets the filename from a uri
 * 
 * @param dest The buffer to store the filename
 * @param dest_len The size of the buffer
 * @param uri The URI of a file
 * @param with_ext If FALSE, the returned filename will not contain extention (i.e. `.mp3')
 * 
 * @return The end of the destination if succeeded, or NULL if failed.
 */
static char* ol_uri_get_filename (char *dest,
                                  size_t dest_len,
                                  const char *uri,
                                  gboolean with_ext);

int
ol_path_get_lrc_pathname (const char *path_pattern,
                          const char *file_pattern,
                          OlMetadata *metadata,
                          char *pathname,
                          size_t len)
{
  if (path_pattern == NULL || file_pattern == NULL ||
      metadata == NULL || pathname == NULL || len <= 0)
    return -1;
  int current = ol_path_expand_path_pattern (path_pattern,
                                                metadata,
                                                pathname,
                                                len);
  if (current == -1)
    return -1;
  if (pathname[current - 1] != '/')
  {
    pathname[current] = '/';
    current++;
    pathname[current]  = '\0';
  }
  int offset = ol_path_expand_file_pattern (file_pattern,
                                               metadata,
                                               pathname + current,
                                               len - current);
  if (offset == -1)
  {
    ol_debugf ("  Expand pattern '%s' failed\n", file_pattern);
    return -1;
  }
  current += offset;
  if (ol_stricmp (&pathname[current - 4], ".lrc", 4) != 0)
  {
    char *end = ol_memcpy (pathname + current, len - current,
                             ".lrc", 4);
    if (end == NULL)
      return -1;
    current = end - pathname;
  }
  return current;
}

static char*
replace_invalid_str (const char *str)
{
  if (str == NULL)
    return NULL;
  char *ret = g_strdup (str);
  char *rep = ret;
  while ((rep = strchr (rep, '/')) != NULL)
  {
    *rep = '_';
  }
  return ret;
}

int
ol_path_expand_file_pattern (const char *pattern,
                             OlMetadata *metadata,
                             char *filename,
                             size_t len)
{
  char buffer[BUFFER_SIZE];
  if (pattern == NULL)
    return -1;
  if (metadata == NULL)
    return -1;
  if (filename == NULL || len <= 0)
    return -1;
  const char *ptr1 = pattern;
  const char *pat_end = pattern + strlen (pattern);
  char *current = filename;
  char *end = filename + len;
  while (ptr1 < pat_end)
  {
    const char *ptr2 = strchr (ptr1, '%'); /* find place holder */
    if (ptr2 == NULL)                    /* no more place holders */
      ptr2 = pat_end; 
    current = ol_memcpy (current, end - current, ptr1, ptr2 - ptr1);
    if (current == NULL)        /* The buffer is not big enough */
      return -1;
    if (ptr2 < pat_end)
    {
      const char *append = NULL;
      gboolean free_append = FALSE;
      size_t delta = 2;
      switch (*(ptr2 + 1))
      {
      case 't':                   /* title */
        append = ol_metadata_get_title (metadata);
        break;
      case 'p':                   /* artist */
        append = ol_metadata_get_artist (metadata);
        break;
      case 'a':                 /* album */
        append = ol_metadata_get_album (metadata);
        break;
      case 'n':                 /* track number */
        snprintf (buffer, BUFFER_SIZE, "%d", ol_metadata_get_track_number (metadata));
        append = buffer;
        break;
      case 'f':                 /* file name */
        if (ol_metadata_get_uri (metadata) == NULL)
          append = NULL;
        else
        {
          if (ol_uri_get_filename (buffer, BUFFER_SIZE, ol_metadata_get_uri (metadata), FALSE)) {
            append = replace_invalid_str (buffer);
            free_append = TRUE;
          }
        }
        break;
      case '%':
        append = "%";
        break;
      default:
        append = "%";
        delta = 1;
        break;
      } /* switch */
      if (append == NULL)
      {
        ol_debugf ("  append is NULL, pattern: %c\n", *(ptr2 + 1));
        return -1;
      }
      current = ol_memcpy (current, end - current,
                             append, strlen (append));
      if (free_append)
        g_free ((char*)append);
      if (current == NULL)
        return -1;
      ptr2 += delta;
    } /* if (ptr2 < pat_end) */
    ptr1 = ptr2;
  } /* while */
  return current - filename;
}

static const char*
ol_uri_get_query_start (const char* uri)
{
  const char *end = g_utf8_strrchr (uri, strlen (uri), '?');
  if (end == NULL)
    end = uri + strlen (uri);
  const char *begin = end;
  while (begin >= uri && *begin != '/') begin--;
  if (*begin == '/') begin++;
  if (begin >= end)
    return NULL;
  return begin;
}

static char*
ol_uri_get_filename (char *dest,
                     size_t dest_len,
                     const char *uri,
                     gboolean with_ext)
{
  if (dest == NULL || dest_len <= 0 || uri == NULL)
    return NULL;
  const char *end = g_utf8_strrchr (uri, strlen (uri), '?');
  if (end == NULL)
    end = uri + strlen (uri);
  const char *begin = ol_uri_get_query_start (uri);
  gchar *file_name = g_uri_unescape_segment (begin, end, NULL);
  if (file_name == NULL)
    return NULL;
  if (!with_ext)
  {
    gchar *ext = g_utf8_strrchr (file_name, strlen (uri), '.');
    if (ext != NULL)
      *ext = '\0';
  }
  char *ret = ol_memcpy (dest, dest_len, file_name, strlen (file_name));
  g_free (file_name);
  return ret;
}

static char*
ol_uri_get_path (char *dest,
                 size_t dest_len,
                 const char *uri)
{
  ol_assert_ret (uri != NULL, NULL);
  char *dirname = NULL;
  if (uri[0] == '/')
  {
    dirname = g_path_get_dirname (uri);
  }
  else
  {
    GError *error = NULL;
    char *pathname = g_filename_from_uri (uri, NULL, &error);
    if (pathname == NULL)
    {
      ol_errorf ("Cannot get pathname from uri %s: %s\n",
                 uri, error->message);
      g_error_free (error);
      return NULL;
    }
    dirname = g_path_get_dirname (pathname);
    g_free (pathname);
  }
  char *ret = ol_memcpy (dest, dest_len, dirname, strlen (dirname));
  g_free (dirname);
  return ret;
}

int
ol_path_expand_path_pattern (const char *pattern,
                             OlMetadata *metadata,
                             char *filename,
                             size_t len)
{
  if (pattern == NULL || filename == NULL || len <= 0)
    return -1;
  if (strcmp (pattern, "%") == 0) /* use music's path */
  {
    if (metadata == NULL || ol_metadata_get_uri (metadata) == NULL)
      return -1;
    char *end = ol_uri_get_path (filename, len, ol_metadata_get_uri (metadata));
    if (end == NULL)
      return -1;
    return end - filename;
  }
  if (pattern[0] == '~' && pattern[1] == '/') /* relative to home */
  {
    const char *home_dir;
    home_dir = g_get_home_dir ();
    char *end = ol_memcpy (filename, len, home_dir, strlen (home_dir));
    if (end == NULL)
      return -1;
    end = ol_memcpy (end, filename + len - end,
                       pattern + 1, strlen (pattern + 1));
    if (end == NULL)
      return -1;
    return end - filename;
  }
  /* Others, copy directly */
  char *end = ol_memcpy (filename, len, pattern, strlen (pattern));
  if (end == NULL)
    return -1;
  else
    return end - filename;
}

gboolean
ol_path_pattern_for_each (char **path_patterns,
                          char **name_patterns,
                          OlMetadata *info,
                          OlPathFunc func,
                          gpointer data)
{
  ol_log_func ();
  ol_assert_ret (path_patterns != NULL, FALSE);
  ol_assert_ret (name_patterns != NULL, FALSE);
  ol_assert_ret (info != NULL, FALSE);
  ol_assert_ret (func != NULL, FALSE);
  char file_name[MAX_PATH_LEN] = "";
  int i, j;
  for (i = 0; path_patterns[i]; i++)
    for (j = 0; name_patterns[j]; j++)
    {
      ol_debugf ("  path:%s, name:%s\n", 
                 path_patterns[i], 
                 name_patterns[j]);
      if ((ol_path_get_lrc_pathname (path_patterns[i],
                                     name_patterns[j],
                                     info,
                                     file_name,
                                     MAX_PATH_LEN)) != -1)
      {
        ol_debugf ("  %s\n", file_name);
        if (func (file_name, data))
        {
          return TRUE;
        }
      } /* if */
    } /* for j */
  return FALSE;
}

