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
#include "ol_gui.h"
#include "ol_intl.h"
#include "ol_debug.h"

const char *BUILDER_FILE = GUIDIR "/dialogs.glade";
static GtkBuilder *builder = NULL;

static void internal_init ();

static void
internal_init ()
{
  if (builder == NULL)
  {
    builder = gtk_builder_new ();
    ol_assert (builder != NULL);
    gtk_builder_set_translation_domain (builder, PACKAGE);
    gtk_builder_add_from_file (builder, BUILDER_FILE, NULL);
    gtk_builder_connect_signals (builder, NULL);
  }
}

GtkWidget* 
ol_gui_get_widget (const char *name)
{
  ol_assert_ret (name != NULL, NULL);
  internal_init ();
  ol_assert_ret (builder != NULL, NULL);
  GObject *obj = gtk_builder_get_object (builder, name);
  if (obj != NULL && GTK_IS_WIDGET (obj))
    return GTK_WIDGET (obj);
  else
    return NULL;
}
