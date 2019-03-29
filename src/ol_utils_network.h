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
#ifndef _OL_UTILS_NETWORK_H_
#define _OL_UTILS_NETWORK_H_

enum OlProxyType {
  OL_PROXY_NONE,
  OL_PROXY_ENVAR,
  OL_PROXY_HTTP,
  OL_PROXY_SOCKS4,
  OL_PROXY_SOCKS5,
};

enum OlProxyType ol_get_proxy_type ();
char *ol_get_proxy_host ();
int ol_get_proxy_port ();
char *ol_get_proxy_username ();
char *ol_get_proxy_password ();

#endif /* _OL_UTILS_NETWORK_H_ */
