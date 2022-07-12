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
#include "ol_commands.h"
#include "ol_config_proxy.h"
#include "ol_debug.h"

void
ol_osd_lock_unlock ()
{
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  ol_assert (config != NULL);
  ol_config_proxy_set_bool (config, "OSD/locked",
                            !ol_config_proxy_get_bool (config, "OSD/locked"));
}

void
ol_show_hide ()
{
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  ol_assert (config != NULL);
  ol_config_proxy_set_bool (config, "OSD/visible",
                            !ol_config_proxy_get_bool (config, "OSD/visible"));
}
