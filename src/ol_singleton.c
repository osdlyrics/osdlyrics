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
 * along with OSD Lyrics.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <glib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include "ol_singleton.h"
#include "config.h"
#include "ol_debug.h"
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

const char *LOCK_FILENAME = "singleton.lock";

/* Try to lock a file, From APUE */
int64_t
_lockfile (int fd)
{
  struct flock fl;
  fl.l_type = F_WRLCK;
  fl.l_start = 0;
  fl.l_whence = SEEK_SET;
  fl.l_len = 0;
  return (fcntl (fd, F_SETLK, &fl));
}

int
ol_is_running ()
{
  ol_log_func ();
  int ret = 1;
  char *dir = g_strdup_printf ("%s/%s/", g_get_user_config_dir (), PACKAGE_NAME);
  if (g_mkdir_with_parents (dir, 0755) == -1)
  {
    ol_error ("Failed to endure config dir");
  }
  else
  {
    char *path = g_strdup_printf ("%s/%s/%s", g_get_user_config_dir (), PACKAGE_NAME, LOCK_FILENAME);
    int fd = open (path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
      ol_error ("Failed to open or create singleton lock");
    }
    else
    {
      if (_lockfile (fd) < 0)
      {
        if (errno == EACCES || errno == EAGAIN)
        {
          close (fd);
        }
        ol_infof ("Can not lock file %s: %s\n", path, strerror (errno));
      }
      else
      {
        if (ftruncate (fd, 0) != 0)
        {
          ol_errorf ("Failed to truncate singleton lock: %s\n",
                     strerror (errno));
        }
        char buf[16];
        sprintf (buf, "%ld", (long)getpid ());
        if (write (fd, buf, strlen (buf)) != strlen (buf))
          ol_errorf ("Failed to write pid in singleton lock\n");
        ret = 0;
      }
    }
    g_free (path);
  }
  g_free (dir);
  return ret;
}
