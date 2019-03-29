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
#include <glib.h>
#include "ol_player.h"
#include "ol_display_module.h"
#include "ol_osd_module.h"
#include "ol_scroll_module.h"
#include "ol_utils.h"
#include "ol_debug.h"

#define call(func, ...)   do { if ((func) != NULL) (func) (__VA_ARGS__); } while (0)

static struct OlDisplayClass* _get_class (const char *name);
static void _class_free (struct OlDisplayClass *klass);
/** 
 * @brief Sets the data of module.
 *
 * @param OlDisplayModule 
 * @param data 
 */
static void _set_data (struct OlDisplayModule *module, void *data);
static void _register_class (struct OlDisplayClass *klass);

static GPtrArray *classes = NULL;


void
ol_display_module_init ()
{
  if (classes == NULL)
  {
    classes = g_ptr_array_new_with_free_func ((GDestroyNotify)_class_free);
    _register_class (ol_osd_module_get_class ());
    _register_class (ol_scroll_module_get_class ());
  }
}

void
ol_display_module_unload ()
{
  if (classes != NULL)
  {
    g_ptr_array_free (classes, TRUE);
    classes = NULL;
  }
}

static void
_register_class (struct OlDisplayClass *klass)
{
  g_ptr_array_add (classes, klass);
}

static struct OlDisplayClass*
_get_class (const char *name)
{
  ol_assert_ret (name != NULL, NULL);
  int i;
  for (i = 0; i < classes->len; i++)
  {
    struct OlDisplayClass *klass = (struct OlDisplayClass *)(g_ptr_array_index (classes, i));
    if (ol_stricmp (klass->name,
                    name,
                    -1) == 0)
      return klass;
  }
  return NULL;
}

struct OlDisplayClass*
ol_display_class_new (const char *name,
                      OlDisplayInitFunc init_func,
                      OlDisplayFreeFunc free_func)
{
  ol_assert_ret (name != NULL, NULL);
  ol_assert_ret (init_func != NULL, NULL);
  ol_assert_ret (free_func != NULL, NULL);
  struct OlDisplayClass *klass = g_new0 (struct OlDisplayClass, 1);
  klass->name = g_strdup (name);
  klass->init = init_func;
  klass->free = free_func;
  return klass;
}

static void
_class_free (struct OlDisplayClass *klass)
{
  ol_assert (klass != NULL);
  if (klass->name != NULL)
  {
    g_free (klass->name);
    klass->name = NULL;
  }
  g_free (klass);
}

static void
_set_data (struct OlDisplayModule *module, void *data)
{
  ol_assert (module != NULL);
  module->data = data;
}

void*
ol_display_module_get_data (struct OlDisplayModule *module)
{
  ol_assert_ret (module != NULL, NULL);
  return module->data;
}

struct OlDisplayModule*
ol_display_module_new (const char *name,
                       OlPlayer *player)
{
  ol_assert_ret (name != NULL, NULL);
  struct OlDisplayModule *module = NULL;
  struct OlDisplayClass *klass = _get_class (name);
  if (klass == NULL)
  {
    ol_error ("Module %s not exists.\n", name);
    /* Fall back to the first display module. */
    return g_ptr_array_index (classes, 0);
  }
  module = g_new0 (struct OlDisplayModule, 1);
  module->klass = klass;
  _set_data (module, klass->init (module, player));
  return module;
}

void
ol_display_module_free (struct OlDisplayModule *module)
{
  ol_assert (module != NULL);
  module->klass->free (module);
  g_free (module);
}

void
ol_display_module_set_played_time (struct OlDisplayModule *module,
                                   guint64 played_time)
{
  ol_assert (module != NULL);
  call (module->klass->set_played_time, module, played_time);
}

void
ol_display_module_set_lrc (struct OlDisplayModule *module,
                           OlLrc *lrc)
{
  ol_assert (module != NULL);
  call (module->klass->set_lrc, module, lrc);
}

void
ol_display_module_set_message (struct OlDisplayModule *module,
                               const char *message,
                               int duration_ms)
{
  ol_assert (module != NULL);
  call (module->klass->set_message, module, message, duration_ms);
}

void
ol_display_module_search_message (struct OlDisplayModule *module,
                                  const char *message)
{
  ol_assert (module != NULL);
  call (module->klass->search_message, module, message);
}

void
ol_display_module_search_fail_message (struct OlDisplayModule *module,
                                       const char *message)
{
  ol_assert (module != NULL);
  call (module->klass->search_fail_message, module, message);
}

void
ol_display_module_download_fail_message (struct OlDisplayModule *module,
                                         const char *message)
{
  ol_assert (module != NULL);
  call (module->klass->download_fail_message, module, message);
}

void
ol_display_module_clear_message (struct OlDisplayModule *module)
{
  ol_assert (module != NULL);
  call (module->klass->clear_message, module);
}
