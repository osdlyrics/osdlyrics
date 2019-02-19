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
#include "ol_keybindings.h"
#include "ol_keybinding_settings.h"
#include "ol_keybinder.h"
#include "ol_commands.h"

static GtkAccelGroup *accel = NULL;

static void ol_hide_accel (gpointer userdata);

static void
ol_hide_accel (gpointer userdata)
{
}
  

void
ol_keybinding_init ()
{
  ol_keybinder_init ();
  GtkAccelGroup *accel = ol_keybinding_get_accel_group ();
  GClosure *hide_closure = g_cclosure_new ((GCallback)ol_hide_accel,
                                           NULL,
                                           NULL);
  gtk_accel_map_add_entry ("<OSD Lyrics>/Hide",
                           gdk_keyval_from_name ("h"),
                           GDK_CONTROL_MASK | GDK_SHIFT_MASK);
  gtk_accel_group_connect_by_path (accel,
                                   "<OSD Lyrics>/Hide",
                                   hide_closure);
  gtk_accel_map_add_entry ("<OSD Lyrics>/Lock",
                           gdk_keyval_from_name ("l"),
                           GDK_CONTROL_MASK | GDK_SHIFT_MASK);
  gtk_accel_group_connect_by_path (accel,
                                   "<OSD Lyrics>/Hide",
                                   hide_closure);
  ol_keybinder_bind ("<Ctrl><Shift>H", ol_show_hide, NULL);
  ol_keybinder_bind ("<Ctrl><Shift>L", ol_osd_lock_unlock, NULL);
}

GtkAccelGroup*
ol_keybinding_get_accel_group ()
{
  if (accel == NULL)
  {
    accel = gtk_accel_group_new ();
  }
  return accel;
}
