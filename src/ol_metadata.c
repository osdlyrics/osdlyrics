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
#include <string.h>
#include "ol_metadata.h"
#include "ol_utils.h"
#include "ol_debug.h"

const int DEFAULT_TRACK_NUM = -1;

struct _OlMetadata
{
  char *title;                 /* The title of the track */
  char *artist;                /* The artist of the track */
  char *album;                 /* The album name of the track */
  int track_number;            /* The track number of the track */
  char *uri;                   /* URI of the track */
  char *art;                   /* URI of the album art */
  guint64 duration;            /* Length of the track */
};

static void internal_set_string (char **string,
                                 const char *val);
static int internal_streq (const char *lhs,
                           const char *rhs);
static int internal_snprint (void *buffer,
                             size_t count,
                             char *val);
static void ol_metadata_init (OlMetadata *metadata);


static void
internal_set_string (char **string,
                     const char *val)
{
  ol_assert (string != NULL);
  if (*string != NULL)
  {
    g_free (*string);
    *string = NULL;
  }
  if (val != NULL)
    *string = g_strdup (val);
}

OlMetadata *
ol_metadata_new (void)
{
  OlMetadata *metadata = g_new (OlMetadata, 1);
  ol_metadata_init (metadata);
  return metadata;
}

OlMetadata *
ol_metadata_new_from_variant (GVariant *variant)
{
  ol_assert_ret (g_variant_is_of_type (variant, G_VARIANT_TYPE ("a{sv}")),
                 NULL);
  OlMetadata *metadata = ol_metadata_new ();
  GVariantIter *iter;
  gchar *key;
  GVariant *value;
  g_variant_get (variant, "a{sv}", &iter);
  while (g_variant_iter_loop (iter, "{&sv}", &key, &value))
  {
    if (g_str_equal (key, "xesam:title"))
    {
      ol_metadata_set_title (metadata, g_variant_get_string (value, NULL));
    }
    else if (g_str_equal (key, "xesam:artist"))
    {
      /* In case of some poor implementations set artist as a string */
      if (g_variant_is_of_type (value, G_VARIANT_TYPE_STRING))
      {
        ol_metadata_set_artist (metadata, g_variant_get_string (value, NULL));
      }
      else if (g_variant_is_of_type (value, G_VARIANT_TYPE ("as")))
      {
        GVariantIter *subiter;
        gchar *art_one;
        gchar *artist;
        gchar **artist_list = g_new (gchar*, g_variant_n_children (value) + 1);
        gint i = 0;
        artist_list[g_variant_n_children (value)] = NULL;
        g_variant_get (value, "as", &subiter);
        while (g_variant_iter_loop (subiter, "s", &art_one))
        {
          artist_list[i] = g_strdup (art_one);
          i++;
        }
        artist = g_strjoinv(", ", artist_list);
        ol_metadata_set_artist (metadata, artist);
        g_strfreev (artist_list);
        g_free (artist);
        g_variant_iter_free (subiter);
      }
      else
      {
        ol_errorf ("Unknown type of artist: %s\n",
                   g_variant_get_type_string (value));
      }
    }
    else if (g_str_equal (key, "xesam:album"))
    {
      ol_metadata_set_album (metadata, g_variant_get_string (value, NULL));
    }
    else if (g_str_equal (key, "mpris:artUrl"))
    {
      ol_metadata_set_art (metadata, g_variant_get_string (value, NULL));
    }
    else if (g_str_equal (key, "xesam:url"))
    {
      ol_metadata_set_uri (metadata, g_variant_get_string (value, NULL));
    }
    else if (g_str_equal (key, "xesam:trackNumber"))
    {
      ol_metadata_set_track_number (metadata, g_variant_get_int32 (value));
    }
    else if (g_str_equal (key, "mpris:length"))
    {
      ol_metadata_set_duration (metadata, g_variant_get_int64 (value));
    }
  }
  g_variant_iter_free (iter);
  return metadata;
}

void
ol_metadata_free (OlMetadata *metadata)
{
  if (metadata == NULL)
    return;
  ol_metadata_clear (metadata);
  g_free (metadata);
}

static void
ol_metadata_init (OlMetadata *metadata)
{
  ol_assert (metadata != NULL);
  metadata->artist = NULL;
  metadata->title = NULL;
  metadata->album = NULL;
  metadata->uri = NULL;
  metadata->art = NULL;
  metadata->duration = 0;
  metadata->track_number = DEFAULT_TRACK_NUM;
}

void
ol_metadata_clear (OlMetadata *metadata)
{
  ol_assert (metadata != NULL);
  ol_metadata_set_title (metadata, NULL);
  ol_metadata_set_artist (metadata, NULL);
  ol_metadata_set_album (metadata, NULL);
  ol_metadata_set_track_number (metadata, DEFAULT_TRACK_NUM);
  ol_metadata_set_uri (metadata, NULL);
  ol_metadata_set_art (metadata, NULL);
  ol_metadata_set_duration (metadata, 0);
}

void
ol_metadata_copy (OlMetadata *dest, const OlMetadata *src)
{
  ol_assert (dest != NULL);
  ol_assert (src != NULL);
  if (dest == src)
    return;
  ol_metadata_set_title (dest, src->title);
  ol_metadata_set_artist (dest, src->artist);
  ol_metadata_set_album (dest, src->album);
  ol_metadata_set_track_number (dest, src->track_number);
  ol_metadata_set_uri (dest, src->uri);
  ol_metadata_set_art (dest, src->art);
  ol_metadata_set_duration (dest, src->duration);
}

OlMetadata *
ol_metadata_dup (const OlMetadata *src)
{
  OlMetadata *metadata = ol_metadata_new ();
  ol_metadata_copy (metadata, src);
  return metadata;
}

void
ol_metadata_set_title (OlMetadata *metadata,
                       const char *title)
{
  ol_assert (metadata != NULL);
  internal_set_string (&(metadata->title), title);
}

const char *
ol_metadata_get_title (const OlMetadata *metadata)
{
  ol_assert_ret (metadata != NULL, NULL);
  return metadata->title;
}

void
ol_metadata_set_artist (OlMetadata *metadata,
                        const char *artist)
{
  ol_assert (metadata != NULL);
  internal_set_string (&(metadata->artist), artist);
}

const char *
ol_metadata_get_artist (const OlMetadata *metadata)
{
  ol_assert_ret (metadata != NULL, NULL);
  return metadata->artist;
}

void
ol_metadata_set_album (OlMetadata *metadata,
                       const char *album)
{
  ol_assert (metadata != NULL);
  internal_set_string (&(metadata->album), album);
}

const char *
ol_metadata_get_album (const OlMetadata *metadata)
{
  ol_assert_ret (metadata != NULL, NULL);
  return metadata->album;
}

void
ol_metadata_set_track_number (OlMetadata *metadata,
                              int track_number)
{
  ol_assert (metadata != NULL);
  metadata->track_number = track_number;
}

void
ol_metadata_set_track_number_from_string (OlMetadata *metadata,
                                          const char *track_number)
{
  ol_assert (metadata != NULL);
  if (track_number == NULL)
    metadata->track_number = DEFAULT_TRACK_NUM;
  else
    sscanf (track_number, "%d", &metadata->track_number);
}

int
ol_metadata_get_track_number (const OlMetadata *metadata)
{
  ol_assert_ret (metadata != NULL, DEFAULT_TRACK_NUM);
  return metadata->track_number;
}

void
ol_metadata_set_uri (OlMetadata *metadata,
                     const char *uri)
{
  ol_assert (metadata != NULL);
  internal_set_string (&(metadata->uri), uri);
}

const char *
ol_metadata_get_uri (const OlMetadata *metadata)
{
  ol_assert_ret (metadata != NULL, NULL);
  return metadata->uri;
}

void
ol_metadata_set_art (OlMetadata *metadata,
                     const char *art)
{
  ol_assert (metadata != NULL);
  internal_set_string (&(metadata->art), art);
}

const char *
ol_metadata_get_art (const OlMetadata *metadata)
{
  ol_assert_ret (metadata != NULL, NULL);
  return metadata->art;
}

void
ol_metadata_set_duration (OlMetadata *metadata,
                          guint64 duration)
{
  ol_assert (metadata != NULL);
  metadata->duration = duration;
}

guint64
ol_metadata_get_duration (const OlMetadata *metadata)
{
  ol_assert_ret (metadata != NULL, 0);
  return metadata->duration;
}

static gboolean artist_valid (const OlMetadata *metadata)
{
  ol_assert_ret (metadata != NULL, FALSE);

  /* List of invalid artist names */
  static const char * const invalid_arts[] = {"unknown", "未知", "群星", NULL};

  if (metadata->artist == NULL || strlen (metadata->artist) == 0)
  {
    return FALSE;
  }

  gboolean valid = TRUE;
  char * artist = g_ascii_strdown (metadata->artist, -1);
  for (const char * const *a = invalid_arts; *a; a++)
  {
    /* 
     * To minimise the risks of false positives, only consider it a match if
     * the artist field starts with the string
     */
    if (g_strstr_len (artist, -1, *a) == artist)
    {
      valid = FALSE;
      break;
    }
  }
  g_free(artist);

  return valid;
}

void
ol_metadata_sanitize_title_artist (OlMetadata *metadata)
{
  ol_assert (metadata != NULL);

  if (metadata->title == NULL || artist_valid (metadata))
  {
    return;
  }

  char *orig_title = metadata->title;
  char *orig_artist = metadata->artist;
  char *new_title = orig_title;
  char *new_artist = orig_artist;
  char *tmp, *rhs;

  /* Remove track number, if any */
  tmp = strstr (new_title, ".");
  if (tmp != NULL && tmp != new_title)
  {
    gboolean is_track_number = TRUE;
    char *p;
    for (p = new_title; p < tmp; p++)
    {
      if (!g_ascii_isdigit (*p) && !g_ascii_isspace(*p))
      {
        is_track_number = FALSE;
        break;
      }
    }

    if (is_track_number)
    {
      rhs = g_strstrip (tmp + 1);
      if (strlen (rhs) > 0)
      {
        new_title = rhs;
      }
    }
  }
  
  /*
   * If any separator is that found in the title (in that order), set the
   * artist to the LHS and the title to the RHS
   */
  static const char * const separators[] = {"--", " - ", "-", NULL};

  for (const char * const *sep = separators; *sep; sep++)
  {
    if ((tmp = strstr (new_title, *sep)))
    {
      rhs = g_strstrip (tmp + strlen (*sep));
      if (strlen (rhs) > 0)
      {
        *tmp = '\0';
        new_artist = new_title;
        new_title = rhs;
        break;
      }
    }
  }

  if (new_artist != orig_artist)
  {
    metadata->artist = g_strdup (new_artist);
    g_free (orig_artist);
  }

  if (new_title != orig_title)
  {
    metadata->title = g_strdup (new_title);
    g_free (orig_title);
  }
}

static int
internal_snprint (void *buffer,
                  size_t count,
                  char *val)
{
  if (val == NULL)
    val = "";
  return snprintf (buffer, count, "%s\n", val);
}

int
ol_metadata_serialize (OlMetadata *metadata,
                       char *buffer,
                       size_t count)
{
  ol_assert_ret (metadata != NULL, 0);
  int cnt = 0;
  if (buffer == NULL)
  {
    if (metadata->title != NULL)
      cnt += strlen (metadata->title);
    cnt++;
    if (metadata->artist != NULL)
      cnt += strlen (metadata->artist);
    cnt++;
    if (metadata->album != NULL)
      cnt += strlen (metadata->album);
    cnt++;
    static char tmpbuf[100];
    cnt += snprintf (tmpbuf, 100, "%d\n", metadata->track_number);
    if (metadata->uri != NULL)
      cnt += strlen (metadata->uri);
    cnt++;
  }
  else
  {
    cnt += internal_snprint (buffer + cnt,
                             count - cnt,
                             metadata->title);
    cnt += internal_snprint (buffer + cnt,
                             count - cnt,
                             metadata->artist);
    cnt += internal_snprint (buffer + cnt,
                             count - cnt,
                             metadata->album);
    cnt += snprintf (buffer + cnt,
                     count - cnt,
                     "%d\n",
                     metadata->track_number);
    cnt += internal_snprint (buffer + cnt,
                             count - cnt,
                             metadata->uri);
    cnt += internal_snprint (buffer + cnt,
                             count - cnt,
                             metadata->art);
    cnt += snprintf (buffer + cnt,
                     count - cnt,
                     "%"G_GUINT64_FORMAT"\n",
                     metadata->duration);
    if (cnt < count)
      buffer[cnt] = '\0';
    else if (count > 0)
      buffer[count - 1] = '\0';
  }
  return cnt;
}

int
ol_metadata_deserialize (OlMetadata *metadata,
                         const char *data)
{
  ol_assert_ret (metadata != NULL, 0);
  ol_assert_ret (data != NULL, 0);
  int ret = 1;
  char *buffer = g_strdup (data);
  char *title = NULL, *artist = NULL, *album = NULL, *track_number = NULL,
    *uri = NULL, *art = NULL, *duration = NULL;
  title = artist = album = track_number = uri = NULL;
  title = buffer;
  if ((artist = ol_split_a_line (title)) == NULL)
    ret = 0;
  else if ((album = ol_split_a_line (artist)) == NULL)
    ret = 0;
  else if ((track_number = ol_split_a_line (album)) == NULL)
    ret = 0;
  else if ((uri = ol_split_a_line (track_number)) == NULL)
    ret = 0;
  else if ((art = ol_split_a_line (track_number)) == NULL)
    ret = 0;
  else if ((duration = ol_split_a_line (track_number)) == NULL)
    ret = 0;
  if ((ol_split_a_line (duration)) == NULL)
    ret = 0;
  if (ret)
  {
    int tn = 0;
    sscanf (track_number, "%d", &tn);
    ol_metadata_set_title (metadata, title);
    ol_metadata_set_artist (metadata, artist);
    ol_metadata_set_album (metadata, album);
    ol_metadata_set_track_number (metadata, tn);
    ol_metadata_set_uri (metadata, uri);
    ol_metadata_set_art (metadata, art);
    ol_metadata_set_duration (metadata, g_ascii_strtoull (duration, NULL, 10));
  }
  g_free (buffer);
  return ret;
}

static int
internal_streq (const char *lhs,
                const char *rhs)
{
  if (lhs == rhs)
    return 1;
  if (lhs == NULL || rhs == NULL)
    return 0;
  return strcmp (lhs, rhs) == 0;
}

int
ol_metadata_equal (const OlMetadata *lhs,
                   const OlMetadata *rhs)
{
  if (lhs == rhs)
    return 1;
  if (lhs == NULL || rhs == NULL)
    return 0;
  if (!internal_streq (lhs->title, rhs->title))
    return 0;
  if (!internal_streq (lhs->artist, rhs->artist))
    return 0;
  if (!internal_streq (lhs->album, rhs->album))
    return 0;
  if (lhs->track_number != rhs->track_number)
    return 0;
  if (!internal_streq (lhs->uri, rhs->uri))
    return 0;
  if (!internal_streq (lhs->art, rhs->art))
    return 0;
  if (lhs->duration != rhs->duration)
    return 0;
  return 1;
}

static void
_add_string_to_dict_builder (GVariantBuilder *builder,
                             const char *key,
                             const char *value)
{
  if (value)
    g_variant_builder_add (builder, "{sv}", key, g_variant_new_string (value));
}

GVariant *
ol_metadata_to_variant (OlMetadata *metadata)
{
  ol_assert_ret (metadata != NULL, NULL);
  GVariantBuilder *builder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
  _add_string_to_dict_builder (builder, "title", metadata->title);
  _add_string_to_dict_builder (builder, "artist", metadata->artist);
  _add_string_to_dict_builder (builder, "album", metadata->album);
  _add_string_to_dict_builder (builder, "location", metadata->uri);
  _add_string_to_dict_builder (builder, "arturl", metadata->art);
  g_variant_builder_add (builder,
                         "{sv}",
                         "time",
                         g_variant_new_uint32 (metadata->duration / 1000));
  g_variant_builder_add (builder,
                         "{sv}",
                         "mtime",
                         g_variant_new_uint32 (metadata->duration));
  if (metadata->track_number > 0)
  {
    gchar *tracknum = g_strdup_printf ("%d", metadata->track_number);
    g_variant_builder_add (builder,
                           "{sv}",
                           "tracknumber",
                           g_variant_new_string (tracknum));
    g_free (tracknum);
  }
  GVariant *ret = g_variant_builder_end (builder);
  g_variant_builder_unref (builder);
  return ret;
}
