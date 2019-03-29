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
#include <sys/wait.h>           /* For WEXITSTATUS */
#include "ol_utils_cmdline.h"
#include "ol_debug.h"

gboolean
ol_cmd_get_string (const char *cmd, char **retval)
{
  /* ol_log_func (); */
  /* ol_debugf ("  cmd: %s\n", cmd); */
  ol_assert_ret (cmd != NULL, FALSE);
  int flags = 0;
  if (retval == NULL)
    flags |= G_SPAWN_STDOUT_TO_DEV_NULL;
  flags |=  G_SPAWN_STDERR_TO_DEV_NULL;
  int exit_status;
  if (!g_spawn_command_line_sync (cmd, retval, NULL, &exit_status, NULL))
    return FALSE;
  if (WEXITSTATUS(exit_status) != 0)
    return FALSE;
  return TRUE;
}

gboolean
ol_cmd_get_int (const char *cmd, int *retval)
{
  char *output;
  gboolean ret = ol_cmd_get_string (cmd, &output);
  if ((output != NULL) && (retval != NULL))
  {
    sscanf (output, "%d", retval);
  }
  if (output != NULL)
    g_free (output);
  return ret;
}

gboolean
ol_cmd_exec (const char *cmd)
{
  char *output = NULL;
  gboolean ret = ol_cmd_get_string (cmd, &output);
  if (output != NULL)
    g_free (output);
  return ret;
}
