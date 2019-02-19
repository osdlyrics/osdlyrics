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

#include "ol_config_updater.h"
#include "ol_config_proxy.h"
#include "ol_config_property.h"

static void
_update_default_values (void)
{
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  int i;
  for (i = 0; i < G_N_ELEMENTS (config_bool); i++)
  {
    ol_config_proxy_set_bool_default (config,
                                      config_bool[i].key,
                                      config_bool[i].default_value);
  }
  for (i = 0; i < G_N_ELEMENTS (config_int); i++)
  {
    ol_config_proxy_set_int_default (config,
                                     config_int[i].key,
                                     config_int[i].default_value);
  }
  for (i = 0; i < G_N_ELEMENTS (config_double); i++)
  {
    ol_config_proxy_set_double_default (config,
                                        config_double[i].key,
                                        config_double[i].default_value);
  }
  for (i = 0; i < G_N_ELEMENTS (config_str); i++)
  {
    ol_config_proxy_set_string_default (config,
                                        config_str[i].key,
                                        config_str[i].default_value);
  }
  for (i = 0; i < G_N_ELEMENTS (config_str_list); i++)
  {
    int len = 0;
    if (config_str_list[i].len > 0)
    {
      len = config_str_list[i].len;
    }
    else
    {
      while (config_str_list[i].default_value[len] != NULL)
        len++;
    }
    ol_config_proxy_set_str_list_default (config,
                                          config_str_list[i].key,
                                          config_str_list[i].default_value,
                                          len);
  }
  ol_config_proxy_sync (config);
}

void
ol_config_update (void)
{
  _update_default_values ();
}
