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
#ifndef __OL_PLAYER_H__
#define __OL_PLAYER_H__

#include <glib.h>
#include <glib-object.h>
#include "ol_metadata.h"

enum OlPlayerStatus {
  OL_PLAYER_PLAYING = 0,
  OL_PLAYER_PAUSED,
  OL_PLAYER_STOPPED,
  OL_PLAYER_UNKNOWN,
  OL_PLAYER_ERROR,
};

enum OlPlayerCaps {
  OL_PLAYER_PLAY =        1 << 0,
  OL_PLAYER_PREV =        1 << 1,
  OL_PLAYER_NEXT =        1 << 2,
  OL_PLAYER_SEEK =        1 << 3,
  OL_PLAYER_STOP =        1 << 4,
  OL_PLAYER_PAUSE =       1 << 5,
  OL_PLAYER_PLAY_PAUSE =  OL_PLAYER_PLAY | OL_PLAYER_PAUSE,
};

#define OL_TYPE_PLAYER                          \
  (ol_player_get_type ())
#define OL_PLAYER(obj)                                  \
  (G_TYPE_CHECK_INSTANCE_CAST (obj, OL_TYPE_PLAYER, OlPlayer))
#define OL_PLAYER_CLASS(klass)                                        \
  (G_TYPE_CHECK_CLASS_CAST (klass, OL_TYPE_PLAYER, OlPlayerClass))
#define OL_IS_PLAYER(obj)                       \
  (G_TYPE_CHECK_INSTANCE_TYPE (obj, OL_TYPE_PLAYER))
#define OL_IS_PLAYER_CLASS(klass)                       \
  (G_TYPE_CHECK_CLASS_TYPE (klass, OL_TYPE_PLAYER))
#define OL_PLAYER_GET_CLASS(obj)                                        \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), OL_TYPE_PLAYER, OlPlayerClass))

typedef struct _OlPlayer OlPlayer;

typedef struct _OlPlayerClass OlPlayerClass;

struct _OlPlayer
{
  GObject parent;
  gpointer priv; /** Private data pointer */
};

struct _OlPlayerClass
{
  GObjectClass parent_class;
};

GType ol_player_get_type (void);

/**
 * Creates a new instance of OlPlayer
 *
 *
 * @return
 */
OlPlayer *ol_player_new (void);

/**
 * @brief Checks whether OSD Lyrics has connected to a supported player.
 *
 * @return TRUE if a supported player is running
 */
gboolean ol_player_is_connected (OlPlayer *player);

/**
 * @brief Gets the name of player
 *
 * @return The returned string is owned by player and should not be freed.
 *         If no supported player is running, NULL will be returned.
 */
const char *ol_player_get_name (OlPlayer *player);

/**
 * @brief Gets the full path of the icon of the current player.
 *
 * @param player
 *
 * @return The path of the icon file, or NULL. The string is owned by the player
 *         and should not be freed.
 */
const char *ol_player_get_icon_path (OlPlayer *player);

/**
 * @brief Gets the metadata of the current metadata
 *
 * @param metadata Return location of music info
 *
 * @return If no support player is connected, return FALSE. In this case,
 *         parameter metadata will not be changed.
 */
gboolean ol_player_get_metadata (OlPlayer *player, OlMetadata *metadata);

/**
 * @brief Gets the elapsed time of the current music
 *
 * @param position Return location of the elasped time, in millisecond
 *
 * @return TRUE if succeeded
 */
gboolean ol_player_get_position (OlPlayer *player, guint64 *pos_ms);

/**
 * @brief Gets the status of the player.
 * The status of a player can be playing, paused or stopped.
 *
 * @return The status of the player, or OL_PLAYER_ERROR if failed
 */
enum OlPlayerStatus ol_player_get_status (OlPlayer *player);

/**
 * @brief Gets which operations are supported by the player controller
 *
 * @return A combination of OlPlayerCapacity, or -1 if failed.
 */
int ol_player_get_caps (OlPlayer *player);

/**
 * @brief Starts playing music. If the player supports this operation,
 * OL_PLAYER_PLAY will be set in its capacity
 *
 * @return FALSE if the operation failed or the player controller dosen't support
 *         this operation.
 */
gboolean ol_player_play (OlPlayer *player);

/**
 * @brief Pauses the current music. The elasped time will not change. If the player
 * supports this operation, OL_PLAYER_STOP will be set in its capacity
 *
 * @param player
 *
 * @return FALSE if the operation failed or the player controller dosen't support
 *         this operation.
 */
gboolean ol_player_pause (OlPlayer *player);

/**
 * @brief Stop playing music. The elapsed time will be reset to 0. If the player
 * supports this operation, OL_PLAYER_STOP will be set in its capacity
 *
 * @param player
 *
 * @return FALSE if the operation failed or the player controller dosen't support
 *         this operation.
 */
gboolean ol_player_stop (OlPlayer *player);

/**
 * @brief Plays the previous music. If the player supports this operation,
 * OL_PLAYER_PREV will be set in its capacity
 *
 * @param player
 *
 * @return FALSE if the operation failed or the player controller dosen't support
 *         this operation.
 */
gboolean ol_player_prev (OlPlayer *player);

/**
 * @brief Plays the next music. If the player supports this operation,
 * OL_PLAYER_NEXT will be set in its capacity
 *
 * @param player
 *
 * @return FALSE if the operation failed or the player controller dosen't support
 *         this operation.
 */
gboolean ol_player_next (OlPlayer *player);

/**
 * @brief Seek the current music to a given position. If the player supports this
 * operation, OL_PLAYER_SEEK will be set in its capacity
 *
 * Note that the actuall time may not equals to the given posision. You may need to
 * call ol_player_get_played_time after this function is called.
 *
 * @param pos_ms The target position in millisecond.
 *
 * @return FALSE if the operation failed or the player controller dosen't support
 *         this operation.
 */
gboolean ol_player_seek (OlPlayer *player, guint64 pos_ms);

#endif // __OL_PLAYER_H__
