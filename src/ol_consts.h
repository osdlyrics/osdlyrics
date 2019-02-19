/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011  Tiger Soldier <tigersoldi@gmail.com>
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
#ifndef _OL_CONSTS_H_
#define _OL_CONSTS_H_

/* The D-Bus well-known name for OSD Lyrics Daemon */
#define OL_SERVICE_DAEMON "org.osdlyrics.Daemon"
/* The interface of daemon controlling */
#define OL_IFACE_DAEMON "org.osdlyrics.Daemon"
/* The object path of daemon controlling */
#define OL_OBJECT_DAEMON "/org/osdlyrics/Daemon"

/* The interface of player module */
#define OL_IFACE_PLAYER "org.osdlyrics.Player"
/* The object path of player support */
#define OL_OBJECT_PLAYER "/org/osdlyrics/Player"

/* The interface of lyrics module */
#define OL_IFACE_LYRICS "org.osdlyrics.Lyrics"
/* The object path of lyrics module */
#define OL_OBJECT_LYRICS "/org/osdlyrics/Lyrics"

/* The interface of LyricSource module */
#define OL_IFACE_LYRIC_SOURCE "org.osdlyrics.LyricSource"
/* The object path of LyricSource module */
#define OL_OBJECT_LYRIC_SOURCE "/org/osdlyrics/LyricSource"

/* The D-Bus well-known name for OSD Lyrics Config */
#define OL_SERVICE_CONFIG "org.osdlyrics.Config"
/* The interface of config service */
#define OL_IFACE_CONFIG "org.osdlyrics.Config"
/* The object path of config service */
#define OL_OBJECT_CONFIG "/org/osdlyrics/Config"

/* The interface of MPRIS2 */
#define OL_IFACE_MPRIS2_PLAYER "org.mpris.MediaPlayer2.Player"
/* The object path of player control of MPRIS1 */
#define OL_OBJECT_MPRIS2 "/org/mpris/MediaPlayer2"

/* The bus name of the GUI process */
#define OL_CLIENT_BUS_NAME "org.osdlyrics.Client.Gtk"

/* Exceptions */
#define OL_ERROR_MALFORMED_KEY "org.osdlyrics.Error.MalformedKey"
#define OL_ERROR_VALUE_NOT_EXIST "org.osdlyrics.Error.ValueNotExist"

#endif /* _OL_CONSTS_H_ */
