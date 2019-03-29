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
#ifndef _OL_DISPLAY_MODULE_H_
#define _OL_DISPLAY_MODULE_H_

#include "ol_metadata.h"
#include "ol_lrc.h"
#include "ol_player.h"

struct OlLrc;
struct OlPlayer;
enum OlPlayerStatus;

struct OlDisplayModule
{
  const struct OlDisplayClass *klass;
  void *data;
};

typedef void* (*OlDisplayInitFunc) (struct OlDisplayModule *module,
                                    OlPlayer *player);
typedef void (*OlDisplayFreeFunc) (struct OlDisplayModule *module);

struct OlDisplayClass
{
  char *name;
  OlDisplayInitFunc init;
  OlDisplayFreeFunc free;
  void (*set_lrc) (struct OlDisplayModule *module,
                   OlLrc *lrc_file);
  void (*set_played_time) (struct OlDisplayModule *module,
                           guint64 played_time);
  void (*set_message) (struct OlDisplayModule *module,
                       const char *message,
                       int duration_ms);
  void (*search_message) (struct OlDisplayModule *module,
                          const char *message);
  void (*search_fail_message) (struct OlDisplayModule *module,
                               const char *message);
  void (*download_fail_message) (struct OlDisplayModule *module,
                                 const char *message);
  void (*clear_message) (struct OlDisplayModule *module);
};

/** functions for implementing concrete modules **/

/** 
 * @brief Create a new class for display module
 *
 * This should be used in ol_foo_moduel_get_class, where foo is the name of
 * the display module
 *
 * @param name The name of the new display module
 * @param init_func The function to initialize the private data
 * @param free_func The function to free the private data
 * 
 * @return A new class of display module, with all functions but init and free
 *         set to NULL
 */
struct OlDisplayClass* ol_display_class_new (const char *name,
                                             OlDisplayInitFunc init_func,
                                             OlDisplayFreeFunc free_func);

void* ol_display_module_get_data (struct OlDisplayModule *module);

/** functions for display module controlling **/

/** 
 * Initialize the display module.
 * 
 * This function must be called before any other functions
 */
void ol_display_module_init ();
void ol_display_module_unload ();
/** 
 * @brief Create a display module of given type
 * 
 * @param name The name of the type of display module, case insensitive
 * @param player The player proxy to use
 * 
 * @return The new display module. If the display module with the given name
 *         not exists, a default display module is returned. The returned module
 *         must be freed with ol_display_module_free.
 */
struct OlDisplayModule *ol_display_module_new (const char *name,
                                               OlPlayer *player);

void ol_display_module_free (struct OlDisplayModule *module);
void ol_display_module_set_played_time (struct OlDisplayModule *module,
                                        guint64 played_time);
void ol_display_module_set_lrc (struct OlDisplayModule *module,
                                OlLrc *lrc);
void ol_display_module_set_message (struct OlDisplayModule *module,
                                    const char *message,
                                    int duration_ms);
void ol_display_module_search_message (struct OlDisplayModule *module,
                                       const char *message);
void ol_display_module_search_fail_message (struct OlDisplayModule *module,
                                            const char *message);
void ol_display_module_download_fail_message (struct OlDisplayModule *module,
                                              const char *message);
void ol_display_module_clear_message (struct OlDisplayModule *module);

#endif /* _OL_DISPLAY_MODULE_H_ */
