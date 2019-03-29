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
#include "ol_color.h"
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "ol_debug.h"

OlColor
ol_color_from_string (const char *color_str)
{
  OlColor ret = ol_color_black;
  if (color_str == NULL)
    return ret;
  if (strlen (color_str) != 7)
    return ret;
  if (color_str[0] != '#')
    return ret;
  int i = 1;
  char colors[3][3] = {{0}};
  for (i = 1; i < 7; i++)
  {
    if ((color_str[i] >= '0' && color_str[i] <= '9') ||
        (color_str[i] >= 'a' && color_str[i] <= 'f') ||
        (color_str[i] >= 'A' && color_str[i] <= 'F'))
    {
      colors[(i - 1) / 2][(i - 1) % 2] = color_str[i];
    }
    else
    {
      return ret;
    }
  }
  unsigned int color_int[3];
  for (i = 0; i < 3; i++)
  {
    sscanf (colors[i], "%x", &color_int[i]);
  }
  ret.r = color_int[0] / 255.0;
  ret.g = color_int[1] / 255.0;
  ret.b = color_int[2] / 255.0;
  return ret;
}

const char*
ol_color_to_string (OlColor color)
{
  static char ret[10] = "";
  unsigned int r, g, b;
  r = color.r * 255;
  g = color.g * 255;
  b = color.b * 255;
  snprintf (ret, 10, "#%02x%02x%02x", r, g, b);
  ol_debugf ("color:%s\n", ret);
  return ret;
}

const OlColor ol_color_black = {0.0, 0.0, 0.0};

OlColor*
ol_color_from_str_list (const char **str_list, int *len)
{
  ol_assert_ret (str_list != NULL, NULL);
  int l = g_strv_length ((char**)str_list);
  OlColor *ret = g_new (OlColor, l);
  int i;
  for (i = 0; i < l; i++)
  {
    ret[i] = ol_color_from_string (str_list[i]);
  }
  if (len != NULL)
    *len = l;
  return ret;
}

char**
ol_color_to_str_list (const OlColor *colors, int len)
{
  ol_assert_ret (colors != NULL && len > 0, NULL);
  char **ret = g_new (char*, len + 1);
  ret[len] = NULL;
  int i;
  for (i = 0; i < len; i++)
  {
    ret[i] = g_strdup (ol_color_to_string (colors[i]));
  }
  return ret;
}
