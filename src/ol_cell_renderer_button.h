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
#ifndef _OL_CELLRENDERERBUTTON_H_
#define _OL_CELLRENDERERBUTTON_H_

#include <gtk/gtkcellrenderer.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkprivate.h>

#define OL_TYPE_CELL_RENDERER_BUTTON		(ol_cell_renderer_button_get_type ())
#define OL_CELL_RENDERER_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), OL_TYPE_CELL_RENDERER_BUTTON, OlCellRendererButton))
#define OL_CELL_RENDERER_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), OL_TYPE_CELL_RENDERER_BUTTON, OlCellRendererButtonClass))
#define OL_IS_CELL_RENDERER_BUTTON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), OL_TYPE_CELL_RENDERER_BUTTON))
#define OL_IS_CELL_RENDERER_BUTTON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), OL_TYPE_CELL_RENDERER_BUTTON))
#define OL_CELL_RENDERER_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OL_TYPE_CELL_RENDERER_BUTTON, OlCellRendererButtonClass))

typedef struct _OlCellRendererButton      OlCellRendererButton;
typedef struct _OlCellRendererButtonClass OlCellRendererButtonClass;

struct _OlCellRendererButton
{
  GtkCellRenderer parent;

  /*< private >*/
  gchar *GSEAL (text);
  gchar *GSEAL (stock_id);

  PangoFontDescription *GSEAL (font);
  /* gint GSEAL (rise); */
  /* gint GSEAL (fixed_height_rows); */

};

struct _OlCellRendererButtonClass
{
  GtkCellRendererClass parent_class;

  void (* edited) (OlCellRendererButton *cell_renderer_button,
		   const gchar         *path,
		   const gchar         *new_text);
  void (* clicked) (OlCellRendererButton *cell_renderer_button,
		   const gchar         *path);

};

GType ol_cell_renderer_button_get_type (void) G_GNUC_CONST;
GtkCellRenderer *ol_cell_renderer_button_new (void);

#endif /* _OL_CELLRENDERERBUTTON_H_ */
