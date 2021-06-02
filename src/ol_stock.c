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
#include <gtk/gtkiconfactory.h>
#include "ol_stock.h"


static GtkIconFactory *icon_factory = NULL;
const char *ICON_LIST[] = {
  OL_STOCK_TRAYICON,
  OL_STOCK_LOADING,
  OL_STOCK_OSD_BG,
  OL_STOCK_OSD_PLAY,
  OL_STOCK_OSD_PAUSE,
  OL_STOCK_OSD_STOP,
  OL_STOCK_OSD_PREV,
  OL_STOCK_OSD_NEXT,
  OL_STOCK_SCROLL_CLOSE,
};

void
ol_stock_init ()
{
  if (icon_factory == NULL)
  {
    icon_factory = gtk_icon_factory_new ();
    int i;
    for (i = 0; i < G_N_ELEMENTS (ICON_LIST); i++)
    {
      GtkIconSet *icon_set = gtk_icon_set_new ();
      GtkIconSource *icon_source = gtk_icon_source_new ();
      gtk_icon_source_set_icon_name (icon_source,
                                     ICON_LIST[i]);
      gtk_icon_set_add_source (icon_set, icon_source);
      gtk_icon_source_free (icon_source);
      gtk_icon_factory_add (icon_factory,
                            ICON_LIST[i],
                            icon_set);
    }
    gtk_icon_factory_add_default (icon_factory);
  }
}
