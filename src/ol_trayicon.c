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
#include "config.h"
#if HAVE_APP_INDICATOR
#include <libappindicator/app-indicator.h>
#endif  /* HAVE_APP_INDICATOR */
#include "ol_trayicon.h"
#include "ol_app.h"
#include "ol_intl.h"
#include "ol_menu.h"
#include "ol_stock.h"
#include "ol_player.h"
#include "ol_config_proxy.h"
#include "ol_debug.h"
#include <gtk/gtkstatusicon.h>
#include <gtk/gtktooltip.h>

#if HAVE_APP_INDICATOR
static AppIndicator *indicator = NULL;
#else  /* HAVE_APP_INDICATOR */
static const char *UNKNOWN_TITLE = N_("Unknown title");
static const char *UNKNOWN_ARTIST = N_("Unknown artist");
static const char *INFO_FORMAT =
  "<big><b>%s</b></big>\n"
  "  %s";
static const char *INFO_FORMAT_ALBUM =
  "<big><b>%s</b></big>\n"
  "  %s - <i>%s</i>";
static GtkStatusIcon *status_icon = NULL;
static gboolean internal_query_tooltip (GtkStatusIcon *status_icon,
                                        gint           x,
                                        gint           y,
                                        gboolean       keyboard_mode,
                                        GtkTooltip    *tooltip,
                                        gpointer       user_data);
static void
activate (GtkStatusIcon* status_icon,
          gpointer user_data)
{
  OlConfigProxy *config = ol_config_proxy_get_instance ();
  ol_config_proxy_set_bool (config, ".visible",
                            !ol_config_proxy_get_bool (config, ".visible"));
}

static gboolean
internal_query_tooltip (GtkStatusIcon *status_icon,
                        gint           x,
                        gint           y,
                        gboolean       keyboard_mode,
                        GtkTooltip    *tooltip,
                        gpointer       user_data)
{
  OlMetadata *metadata = ol_app_get_current_music ();
  if (metadata == NULL ||
      (ol_metadata_get_title (metadata) == NULL &&
       ol_metadata_get_artist (metadata) == NULL))
  {
    gtk_tooltip_set_text (tooltip, _("OSD Lyrics"));
  }
  else
  {
    const char *title = ol_metadata_get_title (metadata);
    if (title == NULL)
      title = _(UNKNOWN_TITLE);
    const char *artist = ol_metadata_get_artist (metadata);
    if (artist == NULL)
      artist = _(UNKNOWN_ARTIST);
    const char *album = ol_metadata_get_album (metadata);
    char *markup = NULL;
    if (album == NULL)
    {
      markup = g_markup_printf_escaped (INFO_FORMAT,
                                        title,
                                        artist);
    }
    else
    {
      markup = g_markup_printf_escaped (INFO_FORMAT_ALBUM,
                                        title,
                                        artist,
                                        album);
    }
    gtk_tooltip_set_markup (tooltip, markup);
    g_free (markup);

    OlPlayer *player = ol_app_get_player ();
    const char *icon_path = ol_player_get_icon_path (player);
    GdkPixbuf *icon = NULL;
    if (icon_path != NULL)
    {
      icon = gdk_pixbuf_new_from_file_at_scale (icon_path, 48, 48, FALSE, NULL);
    }
    gtk_tooltip_set_icon (tooltip,
                          icon);
    if (icon != NULL)
      g_object_unref (icon);
  }
  return TRUE;
}

static void
popup (GtkStatusIcon *status_icon,
       guint button,
       guint activate_time,
       gpointer data)
{
  GtkWidget *popup_menu = ol_menu_get_popup ();
  gtk_menu_popup (GTK_MENU(popup_menu),
                  NULL,
                  NULL,
                  gtk_status_icon_position_menu,
                  status_icon,
                  button,
                  activate_time);
}

static void scroll (GtkStatusIcon *icon, GdkEventScroll *event, gpointer userdata)
{
	int sign = event->direction == GDK_SCROLL_UP ? 1 : -1;
	ol_app_adjust_lyric_offset(sign * 200);
}
#endif  /* HAVE_APP_INDICATOR */

void ol_trayicon_init ()
{
#if HAVE_APP_INDICATOR
  if (indicator == NULL)
  {
    indicator = app_indicator_new (PACKAGE,
                                   OL_STOCK_TRAYICON,
                                   APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_status (indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_attention_icon (indicator, OL_STOCK_TRAYICON);
    app_indicator_set_menu (indicator, GTK_MENU (ol_menu_get_popup ()));
  }
#else

  if (status_icon == NULL)
  {
    status_icon = gtk_status_icon_new_from_icon_name (OL_STOCK_TRAYICON);
    gtk_status_icon_set_tooltip_text (status_icon, _(PROGRAM_NAME));
    gtk_status_icon_set_name (status_icon, PACKAGE_NAME);
    gtk_status_icon_set_title (status_icon, _(PROGRAM_NAME));
    gtk_status_icon_set_visible (status_icon, TRUE);

    /* Connect signals */
    g_signal_connect (G_OBJECT (status_icon), "popup-menu",
                      G_CALLBACK (popup), NULL);

    g_signal_connect (G_OBJECT (status_icon), "query-tooltip",
                      G_CALLBACK (internal_query_tooltip), NULL);

    g_signal_connect (G_OBJECT (status_icon), "activate",
                      G_CALLBACK (activate), NULL);

    g_signal_connect (G_OBJECT (status_icon), "scroll-event",
                      G_CALLBACK (scroll), NULL);
  }
#endif  /* HAVE_APP_INDICATOR */
}

void
ol_trayicon_free ()
{
#if HAVE_APP_INDICATOR
  if (indicator != NULL)
  {
    g_object_unref (indicator);
    indicator = NULL;
  }
#else
  if (status_icon != NULL)
  {
    g_object_unref (status_icon);
    status_icon = NULL;
  }
#endif
}

void
ol_trayicon_status_changed (enum OlPlayerStatus status)
{
#if HAVE_APP_INDICATOR
  if (indicator != NULL)
    app_indicator_set_menu (indicator, GTK_MENU (ol_menu_get_popup ()));
#else
  /* Do nothing */
#endif

}
