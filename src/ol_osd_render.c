/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2009-2011  Tiger Soldier <tigersoldi@gmail.com>
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
#include <string.h>
#include <math.h>
#include "ol_osd_render.h"
#include "ol_gussian_blur.h"
#include "ol_debug.h"

static const int DEFAULT_OUTLINE_WIDTH = 3;
static const char *DEFAULT_FONT_NAME = "serif 30";

static void ol_osd_render_update_font (OlOsdRenderContext *context);
static void ol_osd_render_update_font_height (OlOsdRenderContext *context);

OlOsdRenderContext *
ol_osd_render_context_new ()
{
  OlOsdRenderContext *context = g_new (OlOsdRenderContext, 1);
  context->font_name = g_strdup (DEFAULT_FONT_NAME);
  int i;
  for (i = 0; i < OL_LINEAR_COLOR_COUNT; i++)
  {
    context->linear_colors[i] = ol_color_black;
  }
  context->linear_pos[0] = 0.0;
  context->linear_pos[1] = 0.5;
  context->linear_pos[2] = 1.0;
  context->pango_context = gdk_pango_context_get ();
  context->pango_layout = pango_layout_new (context->pango_context);
  context->text = NULL;
  context->blur_radius = 0.0;
  ol_osd_render_update_font (context);
  ol_osd_render_set_outline_width (context, DEFAULT_OUTLINE_WIDTH);
  return context;
}

void
ol_osd_render_context_destroy (OlOsdRenderContext *context)
{
  ol_assert (context != NULL);
  if (context->font_name != NULL)
    g_free (context->font_name);
  if (context->pango_layout != NULL)
    g_object_unref (context->pango_layout);
  if (context->pango_context != NULL)
    g_object_unref (context->pango_context);
  if (context->text != NULL)
    g_free (context->text);
  g_free (context);
}

void
ol_osd_render_paint_text (OlOsdRenderContext *context,
                          cairo_t *cr,
                          const char *text,
                          double xpos,
                          double ypos)
{
  ol_assert (context != NULL);
  ol_assert (cr != NULL);
  ol_assert (text != NULL);
  ol_osd_render_set_text (context, text);
  int width, height;
  xpos += context->outline_width / 2.0 + context->blur_radius;
  ypos += context->outline_width / 2.0 + context->blur_radius;
  pango_layout_get_pixel_size (context->pango_layout, &width, &height);
  /* draws the outline of the text */
  cairo_move_to (cr, xpos, ypos);
  cairo_save (cr);
  pango_cairo_layout_path(cr, context->pango_layout);
  cairo_set_source_rgb (cr, ol_color_black.r, ol_color_black.g, ol_color_black.b);
  if (context->outline_width > 0)
  {
    cairo_set_line_width (cr, context->outline_width);
    if (context->blur_radius > 1e-4)
    {
      cairo_stroke_preserve (cr);
      cairo_fill (cr);
      ol_gussian_blur (cairo_get_target (cr), context->blur_radius);
    }
    else
    {
      cairo_stroke (cr);
    }
  }
  cairo_restore (cr);
  cairo_save (cr);
  cairo_new_path (cr);
  /* creates the linear pattern */
  cairo_pattern_t *pattern = cairo_pattern_create_linear (xpos, ypos, xpos, ypos + height);
  int i;
  for (i = 0; i < OL_LINEAR_COLOR_COUNT; i++)
  {
    cairo_pattern_add_color_stop_rgb(pattern,
                                     context->linear_pos[i],
                                     context->linear_colors[i].r,
                                     context->linear_colors[i].g,
                                     context->linear_colors[i].b);
  }
  cairo_set_source (cr, pattern);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  /* draws the text */
  cairo_move_to (cr, xpos, ypos);
  pango_cairo_show_layout (cr, context->pango_layout);
  cairo_pattern_destroy (pattern);
  cairo_restore (cr);
}

void
ol_osd_render_get_pixel_size (OlOsdRenderContext *context,
                              const char *text,
                              int *width,
                              int *height)
{
  ol_assert (context != NULL);
  ol_assert (text != NULL);
  ol_assert (width != NULL || height != NULL);
  ol_osd_render_set_text (context, text);
  int w, h;
  pango_layout_get_pixel_size (context->pango_layout, &w, &h);
  if (width != NULL)
    *width = round (w + context->outline_width + context->blur_radius * 2);
  if (height != NULL)
    *height = round (h + context->outline_width + context->blur_radius * 2);
}

void
ol_osd_render_set_linear_color (OlOsdRenderContext *context,
                                int index,
                                OlColor color)
{
  ol_assert (context != NULL);
  ol_assert (index >= 0 && index < OL_LINEAR_COLOR_COUNT);
  context->linear_colors[index] = color;
}

void
ol_osd_render_set_text (OlOsdRenderContext* context,
                        const char *text)
{
  ol_assert (context != NULL);
  ol_assert (text != NULL);
  if (context->text != NULL)
  {
    if (strcmp (context->text, text) == 0)
      return;
    g_free (context->text);
  }
  context->text = g_strdup (text);
  pango_layout_set_text (context->pango_layout, text, -1);
}

void
ol_osd_render_set_font_name (OlOsdRenderContext *context,
                             const char *font_name)
{
  ol_assert (context != NULL);
  ol_assert (font_name != NULL);
  char *new_name = g_strdup (font_name);
  if (context->font_name != NULL)
  {
    g_free (context->font_name);
  }
  context->font_name = new_name;
  ol_osd_render_update_font (context);
}

char *
ol_osd_render_get_font_name (OlOsdRenderContext *context)
{
  ol_assert_ret (context != NULL, NULL);
  return g_strdup (context->font_name);
}

int
ol_osd_render_get_font_height (OlOsdRenderContext *context)
{
  ol_assert_ret (context != NULL, 0);
  return context->font_height;
}

void
ol_osd_render_set_outline_width (OlOsdRenderContext *context,
                                 const int width)
{
  ol_assert (context != NULL);
  ol_assert (width >= 0);
  context->outline_width = width;
}

int
ol_osd_render_get_outline_width (OlOsdRenderContext *context)
{
  ol_assert_ret (context != NULL, 0);
  return context->outline_width;
}

static void
ol_osd_render_update_font_height (OlOsdRenderContext *context)
{
  PangoFontMetrics *metrics = pango_context_get_metrics (context->pango_context,
                                                         pango_layout_get_font_description (context->pango_layout), /* font desc */
                                                         NULL); /* languague */
  if (metrics == NULL)
  {
    ol_errorf ("Cannot get font metrics\n");
  }
  context->font_height = 0;
  int ascent, descent;
  ascent = pango_font_metrics_get_ascent (metrics);
  descent = pango_font_metrics_get_descent (metrics);
  pango_font_metrics_unref (metrics);
  context->font_height += PANGO_PIXELS (ascent + descent);
}

static void
ol_osd_render_update_font (OlOsdRenderContext *context)
{
  ol_assert (context != NULL);
  PangoFontDescription *font_desc = pango_font_description_from_string (context->font_name);
  pango_layout_set_font_description (context->pango_layout, font_desc);
  pango_font_description_free (font_desc);
  ol_osd_render_update_font_height (context);
}

void
ol_osd_render_set_blur_radius (OlOsdRenderContext *context,
                               double radius)
{
  ol_assert (context != NULL);
  if (radius < 0.0)
    radius = 0.0;
  context->blur_radius = radius;
}

double
ol_osd_render_get_blur_radius (OlOsdRenderContext *context)
{
  ol_assert_ret (context != NULL, 0.0);
  return context->blur_radius;
}
