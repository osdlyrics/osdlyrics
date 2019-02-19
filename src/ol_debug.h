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
#ifndef _OL_DEBUG_H_
#define _OL_DEBUG_H_

#include <stdio.h>

enum OlDebugLevel {
  OL_LOG_NONE = -1,
  OL_ERROR = 0,
  OL_DEBUG = 1,
  OL_INFO = 2,
  OL_N_LEVELS,
};

#define ol_logf(level, ...)           do {ol_log_printf (level,        \
                                                         __FILE__,     \
                                                         __LINE__,     \
                                                         __FUNCTION__, \
                                                         __VA_ARGS__); } while (0)
#define ol_log_func()                 do {ol_logf (OL_INFO, "%s\n", __FUNCTION__); } while (0)
#define ol_debugf(...)                do {ol_logf (OL_DEBUG, __VA_ARGS__); } while (0)
#define ol_debug(...)                 do {ol_logf (OL_DEBUG, "%s\n", __VA_ARGS__); } while (0)
#define ol_errorf(...)                do {ol_logf (OL_ERROR, __VA_ARGS__); } while (0)
#define ol_error(...)                 do {ol_logf (OL_ERROR, "%s\n", __VA_ARGS__); } while (0)
#define ol_infof(...)                 do {ol_logf (OL_INFO, __VA_ARGS__); } while (0)
#define ol_info(...)                  do {ol_logf (OL_INFO, "%s\n", __VA_ARGS__); } while (0)
#define ol_assert(assertion)          do {if (!(assertion)) {      \
      ol_logf (OL_ERROR, "assert %s failed\n", #assertion);        \
      return;                                                      \
    }} while (0)
#define ol_assert_ret(assertion, ret) do {if (!(assertion)) {   \
      ol_logf (OL_ERROR, "assert %s failed\n", #assertion);     \
      return (ret);                                             \
    }} while (0)

void ol_log_printf (int level, const char *file, int line, const char *funcname,
                    const char *fmt, ...);

/** 
 * @brief Sets the message level to be logged
 *
 * @param level The level of message to be logged. There are 4 levels:<br />
 *                OL_LOG_NONE: Log nothing
 *                OL_ERROR: Log only messages with OL_ERROR
 *                OL_DEBUG: Log messages with OL_ERROR and OL_DEBUG
 *                OL_INFO: Log all messages
 */
void ol_log_set_level (enum OlDebugLevel level);

/** 
 * @brief Sets the file to store log messages.
 * 
 * @param logfile The filename of the log file. if logfile is "-", it represents
 *                standard output
 * @return Return 1 if succeeded, or 0 if failed.
 */
int ol_log_set_file (const char *logfile);

#endif /* _OL_DEBUG_H_ */
