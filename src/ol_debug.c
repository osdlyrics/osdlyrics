/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier
 *
 * This file is part of OSD Lyrics.
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
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "ol_debug.h"

#define COLOR_BOLD                    "\033[1m"
#define COLOR_ITALIC                  "\033[3m"
#define COLOR_RED                     "\033[31m"
#define COLOR_YELLOW                  "\033[33m"
#define COLOR_GREEN                   "\033[32m"
#define COLOR_RESET                   "\033[0m"
#define COLOR_ERROR                   COLOR_BOLD COLOR_RED
#define COLOR_DEBUG                   COLOR_BOLD COLOR_YELLOW
#define COLOR_INFO                    COLOR_BOLD COLOR_GREEN
int gANSIEscapes = 0;
static const char *LEVEL_MSG[] = {
  COLOR_ERROR "*Error*" COLOR_RESET,
  COLOR_DEBUG "*Debug*" COLOR_RESET,
  COLOR_INFO "*Info*" COLOR_RESET,
};

static FILE *flog = NULL;
static int debug_level = OL_ERROR;

static int
_ensure_flog ()
{
  if (flog == NULL)
    /* return ol_log_set_file ("-"); */
    flog = stdout;
  return 1;
}

void
ol_log_printf (int level, const char *file, int line, const char *funcname,
               const char *fmt, ...)
{
  ol_assert (_ensure_flog ());
  ol_assert (flog != NULL);
  ol_assert (level >= 0);
  ol_assert (level < OL_N_LEVELS);
  if (level > debug_level)
    return;
  va_list ap;
  va_start (ap, fmt);
  fprintf (flog, "%s: in function " COLOR_BOLD "%s" COLOR_RESET
           ": %s[%d]\n",
           LEVEL_MSG[level], funcname, file, line);
  vfprintf (flog, fmt, ap);
  va_end (ap);
}

void
ol_log_set_level (enum OlDebugLevel level)
{
  ol_assert (level >= -1);
  ol_assert (level < OL_N_LEVELS);
  debug_level = level;
}

int
ol_log_set_file (const char *logfile)
{
  ol_assert_ret (logfile != NULL, 0);
  if (flog != NULL)
  {
    fclose (flog);
    flog = NULL;
  }
  if (strcmp (logfile, "-") == 0)
  {
    flog = fdopen (STDOUT_FILENO, "w");
  }
  else
  {
    flog = fopen (logfile, "w");
  }
  return flog != NULL;
}
