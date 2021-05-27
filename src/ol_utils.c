/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier <tigersoldier@gmail.com>
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib-object.h>

#include "ol_utils.h"
#include "ol_debug.h"

const gchar*
ol_get_string_from_hash_table (GHashTable *hash_table, const gchar *key)
{
  if (!hash_table)
    return NULL;
  GValue *value;
  value = (GValue *) g_hash_table_lookup(hash_table, key);
  if (value != NULL && G_VALUE_HOLDS_STRING (value))
  {
    return (const gchar*) g_value_get_string (value);
  }
  else
  {
    ol_debugf ("Type of %s is %s, not string\n",
               key, value != NULL ? G_VALUE_TYPE_NAME (value) : "NULL");
    return NULL;
  }
}

gchar**
ol_get_str_list_from_hash_table (GHashTable *hash_table, const gchar *key)
{
  if (!hash_table)
    return NULL;
  GValue *value;
  value = (GValue *) g_hash_table_lookup(hash_table, key);
  if (value != NULL &&
      (G_VALUE_TYPE (value) == G_TYPE_STRV || G_VALUE_HOLDS_BOXED (value)))
  {
    return (gchar**) g_value_get_boxed (value);
  }
  else
  {
    ol_debugf ("Type of %s is %s, not string list\n",
               key, value != NULL ? G_VALUE_TYPE_NAME (value) : "NULL");
    return NULL;
  }
}

gint
ol_get_int_from_hash_table (GHashTable *hash_table, const gchar *key)
{
  if (!hash_table)
    return -1;
  GValue *value;
  value = (GValue *) g_hash_table_lookup(hash_table, key);
  if (value != NULL && G_VALUE_HOLDS_INT(value))
    return  g_value_get_int (value);
  else
    return -1;
}

guint
ol_get_uint_from_hash_table (GHashTable *hash_table, const gchar *key)
{
  if (!hash_table)
    return 0;
  GValue *value;
  value = (GValue *) g_hash_table_lookup(hash_table, key);
  if (value != NULL && G_VALUE_HOLDS_UINT(value))
    return  g_value_get_uint (value);
  else
    return 0;
}

gint64
ol_get_int64_from_hash_table (GHashTable *hash_table, const gchar *key)
{
  if (!hash_table)
    return -1;
  GValue *value;
  value = (GValue *) g_hash_table_lookup(hash_table, key);
  if (value != NULL && G_VALUE_HOLDS_INT64(value))
    return  g_value_get_int64 (value);
  else
    return -1;
}

gboolean
ol_is_string_empty (const char *str)
{
  if (str == NULL)
    return TRUE;
  int len = strlen (str);
  int i;
  for (i = 0; i < len; i++)
  {
    if (str[i] != ' ')
      return FALSE;
  }
  return TRUE;
}

#ifdef PATH_MAX
static int pathmax = PATH_MAX;
#else
static int pathmax = 0;
#endif
#define PATH_MAX_GUESS 1024

char *
ol_path_alloc(void)
{
  char *ptr;

  if(pathmax == 0)
  {
    errno = 0;
    if((pathmax = pathconf("/", _PC_PATH_MAX)) < 0)
    {
      if(errno == 0)
      {
        pathmax = PATH_MAX_GUESS;
      }
      else
      {
        ol_debugf ("pathconf error for _PC_PATH_MAX\n");
        return NULL;
      }
    }
    else
    {
      pathmax++;
    }
  }

  if((ptr = calloc(pathmax, sizeof(char))) == NULL) {
    ol_debugf ("malloc error for pathname");
    return NULL;
  }
  return ptr;
}

int
ol_stricmp (const char *str1, const char *str2, const ssize_t count)
{
  const char *ptr1 = str1;
  const char *ptr2 = str2;
  int len1 = strlen(str1);
  int len2 = strlen(str2);
  int min = len1 > len2 ? len2 : len1;
  if (count >= 0 && count < min)
    min = count;

  while((ptr1 < str1+min) && (ptr2 < str2+min)) {
    if (isalpha (*ptr1) && isalpha (*ptr2)) {
      if (tolower (*ptr1) != tolower (*ptr2))
        return *ptr1 > *ptr2 ? 1 : -1;
    } else {
      if (*ptr1 != *ptr2)
        return *ptr1 > *ptr2 ? 1 : -1;
    }
    ptr1++;
    ptr2++;
  }
  return 0;
}

size_t
ol_lcs (const char *str1, const char *str2)
{
  if (str1 == NULL ||
      str2 == NULL)
    return 0;
  size_t len[2];
  len[0] = strlen (str1);
  len[1] = strlen (str2);
  if (len[0] == 0 || len[1] == 0)
    return 0;
  size_t i, j;
  int **data;
  data = g_new (int*, len[0]);
  for (i = 0; i < len[0]; i++)
  {
    data[i] = g_new (int, len[1]);
    for (j = 0; j < len[1]; j++)
    {
      data[i][j] = 0;
      if (i > 0 && j > 0)
      {
        data[i][j] = data[i - 1][j - 1];
      }
      if ((tolower (str1[i]) == tolower (str2[j])))
        data[i][j]++;
      if (i > 0 && data[i - 1][j] > data[i][j])
        data[i][j] = data[i - 1][j];
      if (j > 0 && data[i][j - 1] > data[i][j])
        data[i][j] = data[i][j - 1];
    }
  }
  int ret = data[len[0] - 1][len[1] - 1];
  for (i = 0; i < len[0]; i++)
    g_free (data[i]);
  g_free (data);
  return ret;
}

char*
ol_memcpy (char *dest,
             size_t dest_len,
             const char *src,
             size_t src_len)
{
  if (dest == NULL || dest_len <= 0 || src == NULL || src_len < 0)
    return NULL;
  size_t src_real_len = strlen (src);
  if (src_len > src_real_len)
    src_len = src_real_len;
  if (dest_len < src_len + 1)   /* The space in dest is not enough */
  {
    dest[0] = '\0';
    return NULL;
  }
  memcpy (dest, src, src_len);
  dest[src_len] = '\0';
  return dest + src_len;
}

gboolean
ol_streq (const char *str1, const char *str2)
{
  if (str1 == str2)
    return TRUE;
  if (str1 == NULL || str2 == NULL)
    return FALSE;
  return (strcmp (str1, str2) == 0);
}

char *
ol_strptrcpy (char **dest, const char *src)
{
  ol_assert_ret (dest != NULL, NULL);
  if (*dest != NULL)
    g_free (*dest);
  if (src == NULL)
    *dest = NULL;
  else
    *dest = g_strdup (src);
  return *dest;
}

char *
ol_split_a_line (char *str)
{
  ol_assert_ret (str != NULL, NULL);
  while (*str != '\n' && *str != '\0') str++;
  if (*str == '\n')
  {
    *str = '\0';
    return str + 1;
  }
  else
    return NULL;
}

char *
ol_trim_string (char *str)
{
  if (str == NULL)
    return NULL;
  while (isspace (*str))
  {
    *str = '\0';
    str++;
  }
  if (*str == '\0')             /* The whole string is space */
    return NULL;
  size_t len = strlen (str) - 1;
  while (isspace (str[len]))
  {
    str[len] = '\0';
    len--;
  }
  return str;
}

gboolean
ol_path_is_file (const char *filename)
{
  /* ol_log_func (); */
  if (filename == NULL)
    return FALSE;
  struct stat buf;
  /* ol_debugf ("  stat:%d mode:%d\n", stat (filename, &buf), (int)buf.st_mode); */
  return stat (filename, &buf) == 0 && S_ISREG (buf.st_mode);
}

ssize_t
ol_file_len (const char *filename)
{
  ol_assert_ret (filename != NULL, -1);
  struct stat buf;
  if (stat (filename, &buf) != 0)
    return -1;
  return buf.st_size;
}

char*
ol_encode_hex (const char *data, ssize_t len)
{
  ol_assert_ret (data != NULL, NULL);
  if (len < 0)
    len = strlen (data);
  size_t hex_len = len * 2 + 1;
  gchar *hex = g_new (gchar, hex_len);
  gchar *current = hex;
  for (; len > 0; len--, current += 2, data++)
  {
    sprintf (current, "%02x", (unsigned char)*data);
  }
  *current = '\0';
  return hex;
}

void
ol_path_splitext (const char *path, char **root, char **ext)
{
  if (path != NULL)
  {
    char *period = strrchr (path, '.');
    if (period != NULL &&
        (period - path == 0 || *(period - 1) == G_DIR_SEPARATOR))
      period = NULL;
    if (period == NULL)
    {
      if (root != NULL)
        *root = g_strdup (path);
      if (ext != NULL)
        *ext = NULL;
    }
    else
    {
      if (root != NULL)
        *root = g_strndup (path, period - path);
      if (ext != NULL)
        *ext = g_strdup (period);
    }
  }
  else
  {
    if (root != NULL)
      *root = NULL;
    if (ext != NULL)
      *ext = NULL;
  }
}

gint
ol_app_info_cmp (GAppInfo *a, GAppInfo *b)
{
  return strcasecmp (g_app_info_get_display_name (a),
                     g_app_info_get_display_name (b));
}

gboolean
ol_launch_app (const char *cmdline)
{
  ol_assert_ret (cmdline != NULL, FALSE);
  GAppInfo* app = g_app_info_create_from_commandline (cmdline, "", 0, NULL);
  gboolean ret = g_app_info_launch (app, NULL, NULL, NULL);
  g_object_unref (G_OBJECT (app));
  return ret;
}

gboolean
ol_traverse_dir (const char *dirname,
                 gboolean recursive,
                 gboolean (*traverse_func) (const char *path,
                                            const char *filename,
                                            gpointer userdata),
                 gpointer userdata)
{
  ol_assert_ret (dirname != NULL, FALSE);
  ol_assert_ret (traverse_func != NULL, FALSE);
  GError *error = NULL;
  GDir *dir = g_dir_open (dirname, 0, &error);
  if (!dir)
  {
    ol_error ("Cannot open directory %s: %s\n", dirname, error->message);
    return FALSE;
  }
  const gchar *filename = NULL;
  while ((filename = g_dir_read_name (dir)) != NULL)
  {
    if (!traverse_func (dirname, filename, userdata))
      return FALSE;
    if (recursive)
    {
      gchar *filepath = g_build_path (G_DIR_SEPARATOR_S,
                                      dirname, filename, NULL);
      if (g_file_test (filepath, G_FILE_TEST_IS_DIR))
        if (!ol_traverse_dir (filepath, recursive, traverse_func, userdata))
        {
          g_free (filepath);
          return FALSE;
        }
      g_free (filepath);
    }
  }
  return TRUE;
}
