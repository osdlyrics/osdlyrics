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
/**
 * @file   ol_utils_cmdline.h
 * @author Tiger Soldier <tigersoldi@gmail.com>
 * @date   Sat Jan  9 11:58:23 2010
 * 
 * @brief  Utilities for running applications from command line.
 * 
 * 
 */

#ifndef _OL_UTILS_CMDLINE_H_
#define _OL_UTILS_CMDLINE_H_

#include <glib.h>

/** 
 * @brief Execute a command and get its output as a string
 * 
 * @param cmd The command to execute
 * @param retval The return location of the output string, or NULL. \
 *              Note: The passed string will NOT be freed. \
 *              The return string should be freed with g_free.
 * 
 * @return If execute failed or the exit code of command is not 0, return FALSE
 */
gboolean ol_cmd_get_string (const char *cmd, char **retval);

/** 
 * @brief Execute a command and get its output as an integer
 * 
 * @param cmd The command to execute
 * @param retval The return location of the output integer, or NULL. \
 *               If the output cannot be parsed as a number, it will remain what it was.
 * 
 * @return If execute failed or the exit code of command is not 0, return FALSE
 */
gboolean ol_cmd_get_int (const char *cmd, int *retval);

/** 
 * @brief Execute a command and get its output as a string
 * 
 * @param cmd The command to execute
 * 
 * @return If execute failed or the exit code of command is not 0, return FALSE
 */
gboolean ol_cmd_exec (const char *cmd);

#endif /* _OL_UTILS_CMDLINE_H_ */
