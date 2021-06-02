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
#ifndef __OL_OSD_RENDER_H__
#define __OL_OSD_RENDER_H__

#include <gdk/gdkpango.h>
#include <gdk/gdkcairo.h>
#include "ol_color.h"

enum {
  OL_LINEAR_COLOR_COUNT = 3,
};

typedef struct
{
  char *font_name;
  int outline_width;
  OlColor linear_colors[OL_LINEAR_COLOR_COUNT];
  double linear_pos[OL_LINEAR_COLOR_COUNT];
  PangoContext *pango_context;
  PangoLayout *pango_layout;
  char *text;
  double blur_radius;
  int font_height;
} OlOsdRenderContext;

/**
 * @brief Creates a new context
 * The new context should be destroyed by ol_osd_render_context_destroy
 *
 * @return The new context
 */
OlOsdRenderContext* ol_osd_render_context_new ();
/**
 * @brief Destroys an OlOsdRenderContext
 *
 * @param context The context to be destroyed
 */
void ol_osd_render_context_destroy (OlOsdRenderContext *context);

/**
 * @brief Sets the font name for a context
 *
 * @param context An OlOsdRenderContext;
 * @param font_family Font name, must not be NULL
 */
void ol_osd_render_set_font_name (OlOsdRenderContext *context,
                                  const char *font_name);
/**
 * @brief Gets the font name for a context
 *
 * @param context An OlOsdRenderContext
 * @return The font name of the context, must be freed by g_free
 */
char* ol_osd_render_get_font_name (OlOsdRenderContext *context);

/**
 * @brief Sets the outline width
 *
 * @param context An OlOsdRenderContext
 * @param width Outline width, must be positive
 */
void ol_osd_render_set_outline_width (OlOsdRenderContext *context,
                                      const int width);

/**
 * @brief Gets the outline width for a context
 *
 * @param context An OlOsdRenderContext;
 *
 * @return The outline width for the context
 */
int ol_osd_render_get_outline_width (OlOsdRenderContext *context);

/**
 * @brief Gets the height of the font of a context
 *
 * @param context An OlOsdRenderContext
 *
 * @return The height of the font
 */
int ol_osd_render_get_font_height (OlOsdRenderContext *context);

/**
 * @brief Sets linear color
 *
 * @param context An OlOsdRenderContext
 * @param index The index of the color
 * @param color The color to be set
 */

void ol_osd_render_set_linear_color (OlOsdRenderContext *context,
                                     int index,
                                     OlColor color);

/**
 * @brief Paints text to pixmap
 *
 * @param context The context of the renderer
 * @param canvas The GdkDrawable to be drawn
 * @param text The text to be painted
 * @param x The horizontal position
 * @param y The vertectical position
 */
void ol_osd_render_paint_text (OlOsdRenderContext *context,
                               cairo_t *canvas,
                               const char *text,
                               double x,
                               double y);

/**
 * @brief Gets the width and height of the text
 *
 * @param context The context of the renderer, must not be NULL
 * @param text The text to be calculated, must not be NULL
 * @param width The width of the text
 * @param height The height of the text
 */
void ol_osd_render_get_pixel_size (OlOsdRenderContext *context,
                                   const char *text,
                                   int *width,
                                   int *height);
/**
 * @brief Sets text to the context
 *
 * @param context An OlOsdRenderContext
 * @param text Text to be set
 */
void ol_osd_render_set_text (OlOsdRenderContext* context,
                             const char *text);

/**
 * Sets the blur radius of shadow.
 *
 * If the radius is greater than 0, the outline of text will be blurred as shadow.
 *
 * @param context
 * @param radius The blur radius in pixel, non-positive value to disable blurring.
 */
void ol_osd_render_set_blur_radius (OlOsdRenderContext *context,
                                    double radius);

double ol_osd_render_get_blur_radius (OlOsdRenderContext *context);
#endif /* __OL_OSD_RENDER_H__ */
