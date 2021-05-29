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
#ifndef _OL_IMAGE_BUTTON_H_
#define _OL_IMAGE_BUTTON_H_

#include <gtk/gtk.h>

#define OL_IMAGE_BUTTON(obj)                  GTK_CHECK_CAST (obj, ol_image_button_get_type (), OlImageButton)
#define OL_IMAGE_BUTTON_CLASS(klass)          GTK_CHECK_CLASS_CAST (klass, ol_image_button_get_type (), OlImageButtonClass)
#define OL_IS_IMAGE_BUTTON(obj)               GTK_CHECK_TYPE (obj, ol_image_button_get_type ())
#define OL_IMAGE_BUTTON_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), ol_image_button_get_type (), OlImageButtonClass))

typedef struct _OlImageButton OlImageButton;
typedef struct _OlImageButtonClass OlImageButtonClass;

struct _OlImageButton
{
  GtkButton button;
  gpointer priv; /** Private data pointer */
};

struct _OlImageButtonClass
{
  GtkButtonClass button_class;
};

GtkType ol_image_button_get_type (void);

/**
 * @brief Create a new image button
 *
 *
 * @return A new instance of image button
 */
GtkWidget *ol_image_button_new (void);

/**
 * @brief Sets the image of the button
 *
 * The image should contains 4 frames: normal, hover, pressed, disabled, from
 * left to right. The width of each frame must be equal.
 *
 * @param btn
 * @param image The image of the button
 */
void ol_image_button_set_pixbuf (OlImageButton *btn, GdkPixbuf *image);

#endif /* _OL_IMAGE_BUTTON_H_ */
