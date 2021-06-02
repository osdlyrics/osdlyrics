/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier
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
#include <gtk/gtkwidget.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtkaboutdialog.h>
#include <gdk/gdkpixbuf.h>
#include "config.h"
#include "ol_intl.h"
#include "ol_about.h"
#include "ol_gui.h"
#include "ol_debug.h"

void ol_about_close_clicked (GtkWidget *widget);
void ol_about_response (GtkDialog *dialog, gint response_id, gpointer user_data);

void
ol_about_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  ol_debugf ("response_id:%d\n", response_id);
  switch (response_id)
  {
  case GTK_RESPONSE_CANCEL:     /* Close button in about dialog */
    gtk_widget_hide (GTK_WIDGET (dialog));
    break;
  }
}

void
ol_about_close_clicked (GtkWidget *widget)
{
  ol_log_func ();
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);
  if (gtk_widget_is_toplevel (toplevel))
  {
    gtk_widget_hide (toplevel);
  }
}

void
ol_about_show ()
{
  static GtkWidget *window = NULL;
  if (window == NULL)
  {
    window = ol_gui_get_widget ("aboutdialog");
    gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG (window), PROGRAM_NAME);
    gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (window), VERSION);
    g_signal_connect (window, "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    GdkPixbuf *logo = gdk_pixbuf_new_from_file (ICONDIR "/osdlyrics.png", NULL);
    if (logo)
    {
      gtk_about_dialog_set_logo (GTK_ABOUT_DIALOG (window),
                                 logo);
    }
  }
  ol_assert (window != NULL);
  gtk_widget_show (GTK_WIDGET (window));
}
