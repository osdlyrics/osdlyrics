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
#include <glib.h>
#include "ol_utils_dcop.h"
#include "ol_debug.h"

enum {
  BUFFER_SIZE = 512,
};

gboolean
ol_dcop_get_string (const gchar *cmd, gchar **returnval)
{
  ol_assert_ret (cmd != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  FILE *pPipe = popen (cmd, "r");
  if (!pPipe)
    return FALSE;
  gchar buffer[BUFFER_SIZE] = "";
  if (!fgets (buffer, BUFFER_SIZE, pPipe)) {
    pclose(pPipe);
    return FALSE;
  }
  pclose (pPipe);
  strtok (buffer,"\n");
  if (*returnval != NULL)
  {
    g_free (*returnval);
  }
  *returnval = g_strdup (buffer);
  return TRUE;
}

gboolean
ol_dcop_get_uint (const gchar *cmd, guint *returnval)
{
  ol_assert_ret (cmd != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  gchar *ret = NULL;
  if (!ol_dcop_get_string (cmd, &ret))
    return FALSE;
  *returnval = atoi (ret);
  g_free (ret);
  return TRUE;
}

gboolean
ol_dcop_get_boolean (const gchar *cmd, gboolean *returnval)
{
  ol_log_func ();
  ol_assert_ret (cmd != NULL, FALSE);
  ol_assert_ret (returnval != NULL, FALSE);
  gchar *ret = NULL;
  if (!ol_dcop_get_string (cmd, &ret))
    return FALSE;
  *returnval = (strcmp (ret, "true") == 0);
  ol_debugf ("returns %s\n", ret);
  g_free (ret);
  return TRUE;
}
