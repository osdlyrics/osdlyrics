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


#include "ol_cell_renderer_button.h"

enum {
  CLICKED,
  LAST_SIGNAL
};

static guint cell_renderer_button_signals[LAST_SIGNAL];

G_DEFINE_TYPE (OlCellRendererButton,
               ol_cell_renderer_button,
               GTK_TYPE_CELL_RENDERER_PIXBUF)

static gboolean
ol_cell_renderer_button_activate (GtkCellRenderer *cell,
                                  GdkEvent *event,
                                  GtkWidget *widget,
                                  const gchar *path,
                                  const GdkRectangle *background_area,
                                  const GdkRectangle *cell_area,
                                  GtkCellRendererState flags)
{
  if (event == NULL || event->type != GDK_BUTTON_PRESS)
    return FALSE;
  GdkEventButton *button = &event->button;
  if (button->x >= cell_area->x && button->x < cell_area->x + cell_area->width &&
      button->y >= cell_area->y && button->y < cell_area->y + cell_area->height)
  {
    g_signal_emit (cell, cell_renderer_button_signals[CLICKED], 0, path);
    return TRUE;
  }
  return FALSE;
}

static void
ol_cell_renderer_button_init (OlCellRendererButton *cellbutton)
{
  g_object_set (cellbutton,
                "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE,
                "xpad", 2,
                "ypad", 2,
                NULL);
}

static void
ol_cell_renderer_button_class_init (OlCellRendererButtonClass *klass)
{
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (klass);
  cell_class->activate = ol_cell_renderer_button_activate;

  cell_renderer_button_signals[CLICKED] =
    g_signal_new ("clicked",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (OlCellRendererButtonClass, clicked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

GtkCellRenderer *
ol_cell_renderer_button_new (const gchar *icon_name)
{
  return g_object_new (OL_TYPE_CELL_RENDERER_BUTTON,
                       "icon-name", icon_name,
                       NULL);
}
