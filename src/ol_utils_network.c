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
#include "ol_utils_network.h"
#include <string.h>
#include "ol_config_proxy.h"
#include "ol_debug.h"

static enum OlProxyType _get_proxy_type_from_system ();

static enum OlProxyType
_get_proxy_type_from_system ()
{
  /* TODO: We need to detect something like GNOME or KDE */
  return OL_PROXY_ENVAR;
}

enum OlProxyType
ol_get_proxy_type ()
{
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  char *proxy_setting = ol_config_proxy_get_string (config, "Download/proxy");
  enum OlProxyType type = OL_PROXY_NONE;
  if (proxy_setting != NULL &&
      strcmp (proxy_setting, "system") == 0)
  {
    type = _get_proxy_type_from_system ();
  }
  else if (proxy_setting != NULL &&
           strcmp (proxy_setting, "manual") == 0)
  {
    char *proxy_type = ol_config_proxy_get_string (config, "Download/proxy-type");
    if (proxy_type != NULL && strcmp (proxy_type, "socks4") == 0)
      type = OL_PROXY_SOCKS4;
    else if (proxy_type != NULL && strcmp (proxy_type, "socks5") == 0)
      type = OL_PROXY_SOCKS5;
    else
      type = OL_PROXY_HTTP;
    if (proxy_type != NULL)
      g_free (proxy_type);
  }
  if (proxy_setting != NULL)
    g_free (proxy_setting);
  return type;
}

char *
ol_get_proxy_host ()
{
  /* TODO: detect GNOME or KDE */
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  return ol_config_proxy_get_string (config, "Download/proxy-host");
}

int
ol_get_proxy_port ()
{
  /* TODO: detect GNOME or KDE */
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  return ol_config_proxy_get_int (config, "Download/proxy-port");
}

char *
ol_get_proxy_username ()
{
  /* TODO: detect GNOME or KDE */
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  return ol_config_proxy_get_string (config, "Download/proxy-username");
}

char *
ol_get_proxy_password ()
{
  /* TODO: detect GNOME or KDE */
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  return ol_config_proxy_get_string (config, "Download/proxy-password");
}
