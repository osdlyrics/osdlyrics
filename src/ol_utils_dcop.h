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
 * @file   ol_utils_dcop.h
 * @author Tiger Soldier <tigersoldi@gmail.com>
 * @date   Mon May 18 14:15:26 2009
 * 
 * @brief  Utilities for DCOP operation
 * 
 * 
 */
#ifndef _OL_UTILS_DCOP_H_
#define _OL_UTILS_DCOP_H_
/** 
 * @brief Executes the command in cmd, fetch the output as a string and return it .
 * 
 * @param cmd The DCOP command
 * @param returnval The point to the returned string, should be freed by g_free
 * 
 * @return If succeeded, return TRUE
 */
gboolean ol_dcop_get_string (const gchar *cmd, gchar **returnval);

gboolean ol_dcop_get_uint (const gchar *cmd, guint *returnval);

gboolean ol_dcop_get_boolean (const gchar *cmd, gboolean *returnval);

#endif /* _OL_UTILS_DCOP_H_ */
