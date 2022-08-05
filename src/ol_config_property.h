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
 * @file   ol_config_property.h
 * @author Tiger Soldier <tigersoldi@gmail.com>
 * @date   Sun Jun 21 09:34:30 2009
 * 
 * @brief  Defines properties that can be configured
 * 
 * 
 */

#ifndef _OL_CONFIG_PROPERTY_H_
#define _OL_CONFIG_PROPERTY_H_
#include "config.h"

static const char *OL_CONFIG_DEFAULT_DOWNLOAD_ENGINE[] = {
  "ttPlayer",
  "xiami",
  NULL,
};

static const char *OL_CONFIG_ACTIVE_LRC_COLOR[] = {
  "#FF8000", "#FFFF00", "#FF8000", NULL,
};
static const char *OL_CONFIG_INACTIVE_LRC_COLOR[] = {
  "#99FFFF", "#0000FF", "#99FFFF", NULL,
};

static const char *OL_CONFIG_DEFAULT_LRC_PATH[] = {
  "~/.lyrics",
  "%",
  NULL,
};

static const char *OL_CONFIG_DEFAULT_LRC_FILENAME[] = {
  "%p-%t",
  "%t-%p",
  "%f",
  "%t",
  NULL,
};

typedef struct _OlConfigStrListValue OlConfigStrListValue;
struct _OlConfigStrListValue
{
  const char *key;
  int len;
  const char **default_value;
};

typedef struct _OlConfigBoolValue OlConfigBoolValue;
struct _OlConfigBoolValue
{
  const char *key;
  gboolean default_value;
};

typedef struct _OlConfigStringValue OlConfigStringValue;
struct _OlConfigStringValue
{
  const char *key;
  char *default_value;
};

typedef struct _OlConfigIntValue OlConfigIntValue;
struct _OlConfigIntValue
{
  const char *key;
  int min;
  int max;
  int default_value;
};

typedef struct _OlConfigDoubleValue OlConfigDoubleValue;
struct _OlConfigDoubleValue
{
  const char *key;
  double min;
  double max;
  double default_value;
};

static const OlConfigStrListValue config_str_list[] = {
  {"Download/download-engine", -1, OL_CONFIG_DEFAULT_DOWNLOAD_ENGINE},
  {"OSD/active-lrc-color", 3, OL_CONFIG_ACTIVE_LRC_COLOR},
  {"OSD/inactive-lrc-color", 3, OL_CONFIG_INACTIVE_LRC_COLOR},
  {"General/lrc-path", -1, OL_CONFIG_DEFAULT_LRC_PATH},
  {"General/lrc-filename", -1, OL_CONFIG_DEFAULT_LRC_FILENAME},
};

static const OlConfigBoolValue config_bool[] = {
  {"OSD/locked", TRUE},
  {"OSD/visible_when_stopped", TRUE},
  {"OSD/translucent-on-mouse-over", TRUE},
  {"Download/download-first-lyric", FALSE},
  {"General/display-mode-osd", TRUE},
  {"General/display-mode-scroll", TRUE},
  {"General/notify-music", TRUE},
};

static const OlConfigIntValue config_int[] = {
  {"OSD/width", 1, 10000, 1024},
  {"OSD/outline-width", 0, 10, 3},
  {"OSD/line-count", 1, 2, 1},
  {"OSD/x", 0, 10000, 0},
  {"OSD/y", 0, 10000, 0},
  {"Download/proxy-port", 1, 65535, 7070},
  {"ScrollMode/width", 1, 10000, 500},
  {"ScrollMode/height", 1, 10000, 400},
  {"ScrollMode/x", 0, 10000, 0},
  {"ScrollMode/y", 0, 10000, 0},
};

static const OlConfigDoubleValue config_double[] = {
  {"OSD/lrc-align-0", 0.0, 1.0, 0.0},
  {"OSD/lrc-align-1", 0.0, 1.0, 1.0},
  {"ScrollMode/opacity", 0.0, 1.0, 0.9},
  {"OSD/blur-radius", 0.0, 5.0, 2.0},
};

static const OlConfigStringValue config_str[] = {
  {"OSD/font-name", "serif 30"},
  {"Download/proxy", "no"},
  {"Download/proxy-type", "http"},
  {"Download/proxy-host", ""},
  {"Download/proxy-username", ""},
  {"Download/proxy-password", ""},
  {"General/startup-player", ""},
  {"OSD/osd-window-mode", "dock"},
  {"ScrollMode/font-name", "Sans 12"},
  {"ScrollMode/bg-color", "#000000"},
  {"ScrollMode/active-lrc-color", "#E3CF00"},
  {"ScrollMode/inactive-lrc-color", "#FAEBD6"},
  {"ScrollMode/scroll-mode", "always"},
};

#endif /* _OL_CONFIG_PROPERTY_H_ */
