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
#include "ol_osd_window.h"
#include "ol_utils.h"
#include "ol_debug.h"

#define OL_OSD_WINDOW_GET_PRIVATE(obj)  \
    ((OlOsdWindowPrivate *)((OL_OSD_WINDOW(obj))->priv))

enum {
  PROP_0,
  PROP_LOCKED,
};

static const OlColor DEFAULT_INACTIVE_COLORS[OL_LINEAR_COLOR_COUNT]= {
  {0.6, 1.0, 1.0},
  {0.0, 0.0, 1.0},
  {0.6, 1.0, 1.0},
};
static const OlColor DEFAULT_ACTIVE_COLORS[OL_LINEAR_COLOR_COUNT]= {
  {0.4, 0.15, 0.0},
  {1.0, 1.0, 0.0},
  {1.0, 0.5, 0.0},
};
static const int MOUSE_TIMER_INTERVAL = 100;

static const int DEFAULT_HEIGHT = 100;
static const double LINE_PADDING = 0.0;
static const int BORDER_WIDTH = 5;
static const int DEFAULT_WIDTH = 1024;
static const int MAX_LYRIC_LEN = 256;
static const int DEFAULT_FADE_IN_SIZE = 20;

enum DragState
{
  DRAG_NONE,
  DRAG_MOVE,
  DRAG_EAST,
  DRAG_WEST,
};

typedef struct __OlOsdWindowPrivate OlOsdWindowPrivate;
struct __OlOsdWindowPrivate
{
  gint width;
  gint osd_height;
  gint raw_x;
  gint raw_y;
  gboolean locked;
  gboolean composited;          /* whether the screen is composited */
  gboolean update_shape;
  double lyric_xpos[OL_OSD_WINDOW_MAX_LINE_COUNT];
  gulong composited_signal;
  gint mouse_x;
  gint mouse_y;
  gint old_x;
  gint old_y;
  gint old_width;
  gboolean visible;
  guint mouse_timer_id;
  gulong configure_event;
  gboolean mouse_over;
  gboolean mouse_over_lyrics;
  GtkRequisition child_requisition;
  cairo_surface_t *active_lyric_surfaces[OL_OSD_WINDOW_MAX_LINE_COUNT];
  cairo_surface_t *inactive_lyric_surfaces[OL_OSD_WINDOW_MAX_LINE_COUNT];
  GdkPixmap *shape_pixmap;
  double blur_radius;
  enum OlOsdWindowMode mode;
  enum DragState drag_state;
};

struct OsdLrc
{
  gint id;
  gchar *lyric;
};

G_DEFINE_TYPE_WITH_PRIVATE (OlOsdWindow, ol_osd_window, GTK_TYPE_WINDOW)
static void ol_osd_window_destroy (GtkObject *object);
static void ol_osd_window_set_property (GObject *object, guint prop_id,
                                        const GValue *value, GParamSpec *pspec);
static void ol_osd_window_get_property (GObject *object, guint prop_id,
                                        GValue *value, GParamSpec *pspec);
static void ol_osd_window_size_allocate (GtkWidget *widget,
                                         GtkAllocation *allocation);
static void ol_osd_window_size_request (GtkWidget *widget,
                                        GtkRequisition *requisition);
static gboolean ol_osd_window_panel_visible (OlOsdWindow *osd);
static gboolean ol_osd_window_expose_before (GtkWidget *widget, GdkEventExpose *event);
static gboolean ol_osd_window_expose_after (GtkWidget *widget, GdkEventExpose *event);
static gboolean ol_osd_window_paint_bg (OlOsdWindow *osd, cairo_t *cr);
static gboolean ol_osd_window_enter_notify (GtkWidget *widget, GdkEventCrossing *event);
static gboolean ol_osd_window_leave_notify (GtkWidget *widget, GdkEventCrossing *event);
static gboolean ol_osd_window_button_press (GtkWidget *widget, GdkEventButton *event);
static gboolean ol_osd_window_motion_notify (GtkWidget *widget, GdkEventMotion *event);
static gboolean ol_osd_window_button_release (GtkWidget *widget, GdkEventButton *event);
static gboolean ol_osd_window_configure_event (GtkWindow *window,
                                               GdkEventConfigure *event,
                                               gpointer user_data);
static gboolean ol_osd_window_map_cb (GtkWidget *widget,
                                      GdkEvent  *event,
                                      gpointer   user_data);
static gboolean ol_osd_window_unmap_cb (GtkWidget *widget,
                                        GdkEvent  *event,
                                        gpointer   user_data);
static void ol_osd_window_realize_cb (GtkWidget *widget,
                                      gpointer user_data);
static void ol_osd_window_unrealize_cb (GtkWidget *widget,
                                        gpointer user_data);

static int ol_osd_window_compute_osd_height (OlOsdWindow *osd);
static int ol_osd_window_compute_window_height (OlOsdWindow *osd);
static double ol_osd_window_compute_lyric_xpos (OlOsdWindow *osd,
                                                int line,
                                                double percentage);
static int ol_osd_window_compute_lyric_ypos (OlOsdWindow *osd);
static void ol_osd_window_paint_lyrics (OlOsdWindow *osd, cairo_t *cr);
static cairo_pattern_t *ol_osd_window_create_text_mask (OlOsdWindow *osd,
                                                        cairo_t *cr,
                                                        int line,
                                                        gdouble text_xpos,
                                                        gdouble alpha);
static void ol_osd_window_emit_move (OlOsdWindow *osd);
static void ol_osd_window_emit_resize (OlOsdWindow *osd);
static GdkWindowEdge ol_osd_window_get_edge_on_point (OlOsdWindow *osd,
                                                           gint x,
                                                           gint y);
static void ol_osd_window_update_child_size_requisition (OlOsdWindow *osd);
static void ol_osd_window_update_layout (OlOsdWindow *osd);
static void ol_osd_window_move_resize (OlOsdWindow *osd,
                                       int x,
                                       int y,
                                       int width,
                                       int height,
                                       enum DragState drag_type);
static void ol_osd_window_paint (OlOsdWindow *osd);
static void ol_osd_window_clear_cairo (cairo_t *cr);
static void ol_osd_window_reset_shape_pixmap (OlOsdWindow *osd);
static void ol_osd_window_update_shape (OlOsdWindow *osd);
static void ol_osd_window_queue_reshape (OlOsdWindow *osd);
static void ol_osd_window_set_input_shape_mask (OlOsdWindow *osd,
                                                gboolean disable_input);
static void ol_osd_draw_lyric_surface (OlOsdWindow *osd,
                                       cairo_surface_t **surface,
                                       const char *lyric);
static void ol_osd_window_update_lyric_surface (OlOsdWindow *osd, int line);
static void ol_osd_window_update_lyric_rect (OlOsdWindow *osd, int line);
static void ol_osd_window_update_colormap (OlOsdWindow *osd);
static void ol_osd_window_screen_composited_changed (GdkScreen *screen,
                                                     gpointer userdata);
static gboolean ol_osd_window_mouse_timer (gpointer data);
static void ol_osd_window_update_child_allocation (OlOsdWindow *osd);
static void ol_osd_window_check_mouse_leave (OlOsdWindow *osd);
static gboolean ol_osd_window_should_blur (OlOsdWindow *osd);
static void ol_osd_window_update_render_blur_radius (OlOsdWindow *osd);
/* static void ol_osd_window_update_child_visibility (OlOsdWindow *osd); */

static void _paint_rect (cairo_t *cr, GdkPixbuf *source,
                         double src_x, double src_y,
                         double src_w, double src_h,
                         double des_x, double des_y,
                         double dex_w, double des_h);
static gboolean _point_in_rect (int x, int y, GdkRectangle *rect);
static void ol_osd_window_queue_resize (OlOsdWindow *osd);

static void
_paint_rect (cairo_t *cr, GdkPixbuf *source,
             double src_x, double src_y,
             double src_w, double src_h,
             double des_x, double des_y,
             double des_w, double des_h)
{
  ol_assert (cr != NULL);
  ol_assert (source != NULL);
  cairo_save (cr);
  double sw = des_w / src_w;
  double sh = des_h / src_h;
  cairo_translate (cr, des_x, des_y);
  cairo_rectangle (cr, 0, 0,
                   des_w, des_h);
  cairo_scale (cr, sw, sh);
  cairo_clip (cr);
  gdk_cairo_set_source_pixbuf (cr,
                               source,
                               -src_x,
                               -src_y);
  cairo_paint (cr);
  cairo_restore (cr);
}

static gboolean
ol_osd_window_paint_bg (OlOsdWindow *osd, cairo_t *cr)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), FALSE);
  GtkWidget *widget = GTK_WIDGET (osd);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  int w, h;
  gdk_drawable_get_size (widget->window, &w, &h);
  ol_osd_window_clear_cairo (cr);
  if (ol_osd_window_panel_visible(osd))
  {
    if (osd->bg_pixbuf != NULL)
    {
      int sw, sh;
      if (priv->composited)
      {
        ol_osd_window_clear_cairo (cr);
      }
      else
      {
        cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
        cairo_set_source_rgb (cr, 0.9, 0.9, 0.9);
        cairo_rectangle (cr, 0, 0, w, h);
        cairo_fill (cr);
      }
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
      sw = gdk_pixbuf_get_width (osd->bg_pixbuf);
      sh = gdk_pixbuf_get_height (osd->bg_pixbuf);
      _paint_rect (cr, osd->bg_pixbuf,
                   0, 0, BORDER_WIDTH, BORDER_WIDTH,
                   0, 0, BORDER_WIDTH, BORDER_WIDTH);
      _paint_rect (cr, osd->bg_pixbuf,
                   0, sh - BORDER_WIDTH, BORDER_WIDTH, BORDER_WIDTH,
                   0, h - BORDER_WIDTH, BORDER_WIDTH, BORDER_WIDTH);
      _paint_rect (cr, osd->bg_pixbuf,
                   sw - BORDER_WIDTH, 0, BORDER_WIDTH, BORDER_WIDTH,
                   w - BORDER_WIDTH, 0, BORDER_WIDTH, BORDER_WIDTH);
      _paint_rect (cr, osd->bg_pixbuf,
                   sw - BORDER_WIDTH, sh - BORDER_WIDTH,
                   BORDER_WIDTH, BORDER_WIDTH,
                   w - BORDER_WIDTH, h - BORDER_WIDTH,
                   BORDER_WIDTH, BORDER_WIDTH);
      _paint_rect (cr, osd->bg_pixbuf,
                   0, BORDER_WIDTH, BORDER_WIDTH, sh - BORDER_WIDTH * 2,
                   0, BORDER_WIDTH, BORDER_WIDTH, h - BORDER_WIDTH * 2);
      _paint_rect (cr, osd->bg_pixbuf,
                   sw - BORDER_WIDTH, BORDER_WIDTH,
                   BORDER_WIDTH, sh - BORDER_WIDTH * 2,
                   w - BORDER_WIDTH, BORDER_WIDTH,
                   BORDER_WIDTH, h - BORDER_WIDTH * 2);
      _paint_rect (cr, osd->bg_pixbuf,
                   BORDER_WIDTH, 0, sw - BORDER_WIDTH * 2, BORDER_WIDTH,
                   BORDER_WIDTH, 0, w - BORDER_WIDTH * 2, BORDER_WIDTH);
      _paint_rect (cr, osd->bg_pixbuf,
                   BORDER_WIDTH, sh - BORDER_WIDTH,
                   sw - BORDER_WIDTH * 2, BORDER_WIDTH,
                   BORDER_WIDTH, h - BORDER_WIDTH,
                   w - BORDER_WIDTH * 2, BORDER_WIDTH);
      _paint_rect (cr, osd->bg_pixbuf,
                   BORDER_WIDTH, BORDER_WIDTH,
                   sw - BORDER_WIDTH * 2, sh - BORDER_WIDTH * 2,
                   BORDER_WIDTH, BORDER_WIDTH,
                   w - BORDER_WIDTH * 2, h - BORDER_WIDTH * 2);

    }
    else
    {
      gtk_paint_box (widget->style,
                     widget->window,
                     GTK_STATE_NORMAL, GTK_SHADOW_IN,
                     NULL, widget, "buttondefault",
                     0, 0, w, h);

    }
  }
  return TRUE;
}

static void
ol_osd_window_update_colormap (OlOsdWindow *osd)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  GtkWidget *widget = GTK_WIDGET (osd);
  gboolean mapped = gtk_widget_get_mapped (widget);
  gboolean realized = gtk_widget_get_realized (widget);
  GdkScreen *screen = gtk_widget_get_screen (widget);
  GdkColormap* colormap = gdk_screen_get_rgba_colormap (screen);
  if (colormap == NULL)
    colormap = gdk_screen_get_rgb_colormap (screen);
  if (mapped)
    gtk_widget_unmap (widget);
  if (realized)
  {
    gtk_widget_unrealize (widget);
  }
  gtk_widget_set_colormap (widget, colormap);
  if (realized)
  {
    gtk_widget_realize (widget);
  }
  if (mapped)
    gtk_widget_map (widget);
}

static void
ol_osd_window_screen_composited_changed (GdkScreen *screen, gpointer userdata)
{
  ol_log_func ();
  ol_assert (OL_IS_OSD_WINDOW (userdata));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (userdata);
  GtkWidget *widget = GTK_WIDGET (userdata);
  OlOsdWindow *osd = OL_OSD_WINDOW (userdata);
  priv->composited = gdk_screen_is_composited (screen);
  if (priv->blur_radius > 0)
  {
    ol_osd_window_update_render_blur_radius (osd);
    int i;
    for (i = 0; i < osd->line_count; i++)
      ol_osd_window_update_lyric_surface (osd, i);
  }
  ol_osd_window_queue_reshape (osd);
  gtk_widget_queue_draw (widget);
}

static gboolean
ol_osd_window_use_system_drag (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), FALSE);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  return priv->mode == OL_OSD_WINDOW_NORMAL;
}

static gboolean
ol_osd_window_button_press (GtkWidget *widget, GdkEventButton *event)
{
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  if (GTK_WIDGET_CLASS (ol_osd_window_parent_class)->button_press_event &&
      GTK_WIDGET_CLASS (ol_osd_window_parent_class)->button_press_event (widget,
                                                                         event))
      return TRUE;
  ol_log_func ();
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (widget);
  if (event->button == 1 && !priv->locked)
  {
    int edge = ol_osd_window_get_edge_on_point (osd, event->x, event->y);
    if (ol_osd_window_use_system_drag (osd)) {
      switch (edge) {
      case GDK_WINDOW_EDGE_EAST:
      case GDK_WINDOW_EDGE_WEST:
        gtk_window_begin_resize_drag (GTK_WINDOW (widget),
                                      edge,
                                      event->button,
                                      event->x_root,
                                      event->y_root,
                                      event->time);
        break;
      default:
        gtk_window_begin_move_drag (GTK_WINDOW (widget),
                                    event->button,
                                    event->x_root,
                                    event->y_root,
                                    event->time);
      }
    } else {
      priv->old_x = widget->allocation.x;
      priv->old_y = widget->allocation.y;
      priv->old_width = widget->allocation.width;
      priv->mouse_x = event->x_root;
      priv->mouse_y = event->y_root;
      switch (edge) {
      case GDK_WINDOW_EDGE_EAST:
        priv->drag_state = DRAG_EAST;
        break;
      case GDK_WINDOW_EDGE_WEST:
        priv->drag_state = DRAG_WEST;
        break;
      default:
        priv->drag_state = DRAG_MOVE;
        break;
      }
    }
  }
  return FALSE;
}

static void
ol_osd_window_emit_move (OlOsdWindow *osd)
{
  GValue params[1] = {{0}};
  g_value_init (&params[0], G_OBJECT_TYPE (osd));
  g_value_set_object (&params[0], G_OBJECT (osd));
  g_signal_emitv (params,
                  OL_OSD_WINDOW_GET_CLASS (osd)->signals[OSD_MOVED],
                  0,
                  NULL);
}

static void
ol_osd_window_emit_resize (OlOsdWindow *osd)
{
  GValue params[1] = {{0}};
  g_value_init (&params[0], G_OBJECT_TYPE (osd));
  g_value_set_object (&params[0], G_OBJECT (osd));
  g_signal_emitv (params,
                  OL_OSD_WINDOW_GET_CLASS (osd)->signals[OSD_RESIZE],
                  0,
                  NULL);
}

static gboolean
ol_osd_window_button_release (GtkWidget *widget, GdkEventButton *event)
{
  /* ol_log_func (); */
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (widget);
  if (priv->drag_state != DRAG_NONE)
  {
    ol_osd_window_emit_move (osd);
    ol_osd_window_emit_resize (osd);
  }
  priv->drag_state = DRAG_NONE;
  return FALSE;
}

static gboolean
ol_osd_window_configure_event (GtkWindow *window,
                               GdkEventConfigure *event,
                               gpointer user_data)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (window), FALSE);
  GtkWidget *widget = GTK_WIDGET (window);
  OlOsdWindow *osd = OL_OSD_WINDOW (window);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  gboolean size_changed = FALSE;
  widget->allocation.x = event->x;
  widget->allocation.y = event->y;
  if (priv->width != event->width)
  {
    priv->width = event->width;
    size_changed = TRUE;
    if (priv->drag_state == DRAG_NONE)
      ol_osd_window_emit_resize (osd);
  }
  if (size_changed)
  {
    ol_osd_window_update_child_allocation (osd);
    ol_osd_window_reset_shape_pixmap (osd);
  }
  if (priv->raw_x != event->x || priv->raw_y != event->y)
  {
    priv->raw_x = event->x;
    priv->raw_y = event->y;
    if (priv->drag_state == DRAG_NONE)
      ol_osd_window_emit_move (osd);
  }
  return FALSE;
}

static GdkWindowEdge
ol_osd_window_get_edge_on_point (OlOsdWindow *osd,
                                 gint x,
                                 gint y)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), GDK_WINDOW_EDGE_SOUTH);
  GtkWidget *widget = GTK_WIDGET (osd);
  gint width = widget->allocation.width;
  gint height = widget->allocation.height;
  if (y >= 0 && y <= height) {
    if (x >= 0 && x < BORDER_WIDTH)
      return GDK_WINDOW_EDGE_WEST;
    if (x >= width - BORDER_WIDTH && x < width)
      return GDK_WINDOW_EDGE_EAST;
  }
  return -1;
}

static gboolean
ol_osd_window_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
  ol_log_func ();
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (widget);
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  int x = priv->old_x + (event->x_root - priv->mouse_x);
  int y = priv->old_y + (event->y_root - priv->mouse_y);
  int height, width;
  gtk_window_get_size (GTK_WINDOW (osd), &width, &height);
  switch (priv->drag_state)
  {
  case DRAG_MOVE:
    ol_osd_window_move (osd, x, y);
    break;
  case DRAG_EAST:
    ol_osd_window_move_resize (osd, priv->old_x, priv->old_y,
                               priv->old_width + (event->x_root - priv->mouse_x),
                               height,
                               DRAG_EAST);
    break;
  case DRAG_WEST:
    ol_osd_window_move_resize (osd, x, priv->old_y,
                               priv->old_width + priv->old_x - x,
                               height,
                               DRAG_WEST);

    break;
  case DRAG_NONE:
  {
    int edge = ol_osd_window_get_edge_on_point (osd,
                                                event->x,
                                                event->y);
    GdkCursor *cursor = NULL;
    switch (edge) {
    case GDK_WINDOW_EDGE_EAST:
        cursor = gdk_cursor_new (GDK_RIGHT_SIDE);
        break;
    case GDK_WINDOW_EDGE_WEST:
      cursor = gdk_cursor_new (GDK_LEFT_SIDE);
      break;
    }
    gdk_window_set_cursor (widget->window,
                           cursor);
    if (cursor)
      gdk_cursor_unref (cursor);
    break;
  }
  default:
    break;
  }
  return FALSE;
}

static gboolean ol_osd_window_panel_visible (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), FALSE);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  return priv->mode == OL_OSD_WINDOW_NORMAL ||
    (priv->locked == FALSE && priv->mouse_over);
}

static gboolean
ol_osd_window_expose_before (GtkWidget *widget, GdkEventExpose *event)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (widget), FALSE);
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  if (ol_osd_window_panel_visible (osd))
    ol_osd_window_paint (osd);
  return FALSE;
}

static gboolean
ol_osd_window_expose_after (GtkWidget *widget, GdkEventExpose *event)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (widget), FALSE);
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  if (!ol_osd_window_panel_visible (osd))
    ol_osd_window_paint (osd);
  return FALSE;
}

static void
ol_osd_window_paint (OlOsdWindow *osd)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  cairo_t *cr;
  cr = gdk_cairo_create (GTK_WIDGET (osd)->window);
  ol_osd_window_paint_bg (osd, cr);
  ol_osd_window_paint_lyrics (osd, cr);
  if (priv->update_shape)
    ol_osd_window_update_shape (osd);
  cairo_destroy (cr);
}

static gboolean
ol_osd_window_enter_notify (GtkWidget *widget, GdkEventCrossing *event)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (widget), FALSE);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (widget);
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  priv->mouse_over = TRUE;
  ol_osd_window_queue_reshape (osd);
  gtk_widget_queue_draw (widget);
  /* ol_osd_window_update_child_visibility (OL_OSD_WINDOW (widget)); */
  return FALSE;
}

static void
ol_osd_window_check_mouse_leave (OlOsdWindow *osd)
{
  ol_assert (osd != NULL);
  GtkWidget *widget = GTK_WIDGET (osd);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (widget);
  if (!gdk_window_is_visible (widget->window))
    return;
  gint rel_x, rel_y;
  gdk_window_get_pointer (widget->window, &rel_x, &rel_y, NULL);
  GdkRectangle rect;
  rect.x = 0; rect.y = 0;
  rect.width = widget->allocation.width;
  rect.height = widget->allocation.height;
  if ((priv->mode != OL_OSD_WINDOW_DOCK ||
       priv->drag_state == DRAG_NONE) &&
      !_point_in_rect (rel_x, rel_y, &rect))
  {
    priv->mouse_over = FALSE;
    ol_osd_window_queue_reshape (osd);
    gtk_widget_queue_draw (widget);
  }
}

static gboolean
ol_osd_window_leave_notify (GtkWidget *widget, GdkEventCrossing *event)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (widget), FALSE);
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  ol_osd_window_check_mouse_leave (osd);
  return FALSE;
}

static gboolean
ol_osd_window_map_cb (GtkWidget *widget,
                      GdkEvent  *event,
                      gpointer   user_data) {
  ol_assert_ret (OL_IS_OSD_WINDOW (widget), FALSE);
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (widget);
  if (priv->mouse_timer_id == 0)
  {
    priv->mouse_timer_id = g_timeout_add (MOUSE_TIMER_INTERVAL,
                                          (GSourceFunc) ol_osd_window_mouse_timer,
                                          widget);
  }
  ol_osd_window_queue_reshape (osd);
  priv->configure_event = g_signal_connect (G_OBJECT (osd), "configure-event",
                                            G_CALLBACK (ol_osd_window_configure_event), osd);
  return FALSE;
}

static gboolean
ol_osd_window_unmap_cb (GtkWidget *widget,
                      GdkEvent  *event,
                      gpointer   user_data) {
  ol_assert_ret (OL_IS_OSD_WINDOW (widget), FALSE);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (widget);
  if (priv->mouse_timer_id != 0)
  {
    g_source_remove (priv->mouse_timer_id);
    priv->mouse_timer_id = 0;
  }
  if (priv->configure_event > 0) {
    g_signal_handler_disconnect (widget,
                                 priv->configure_event);
    priv->configure_event = 0;
  }
  return FALSE;
}

static void
ol_osd_window_realize_cb (GtkWidget *widget,
                          gpointer   user_data)
{
  ol_assert (OL_IS_OSD_WINDOW (widget));
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  if (priv->composited_signal == 0)
  {
    GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (widget));
    priv->composited_signal = g_signal_connect (screen,
                                                "composited-changed",
                                                G_CALLBACK (ol_osd_window_screen_composited_changed),
                                                osd);
  }
  ol_osd_window_reset_shape_pixmap (osd);
  ol_osd_window_set_input_shape_mask (osd, priv->locked);
}

static void
ol_osd_window_unrealize_cb (GtkWidget *widget,
                            gpointer   user_data)
{
  ol_assert (OL_IS_OSD_WINDOW (widget));
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  if (priv->composited_signal != 0)
  {
    g_signal_handler_disconnect (gtk_widget_get_screen (widget),
                                 priv->composited_signal);
    priv->composited_signal = 0;
  }
}

static void
ol_osd_window_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  widget->allocation = *allocation;

  /* priv->width = allocation->width; */
  if (gtk_widget_get_realized (widget))
  {
    gdk_window_resize (widget->window,
                       widget->allocation.width,
                       widget->allocation.height);
  }
  ol_osd_window_update_child_allocation (osd);
}

static void
ol_osd_window_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  OlOsdWindow *osd = OL_OSD_WINDOW (widget);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  ol_osd_window_update_layout (osd);
  requisition->width = priv->width;
  requisition->height = ol_osd_window_compute_window_height (osd);
}

static void
ol_osd_window_update_layout (OlOsdWindow *osd)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  int old_height = priv->osd_height;
  priv->osd_height = ol_osd_window_compute_osd_height (osd);
  if (priv->osd_height != old_height)
    ol_osd_window_reset_shape_pixmap (osd);
  ol_osd_window_update_child_size_requisition (osd);
}

static gboolean
_point_in_rect (int x, int y, GdkRectangle *rect)
{
  ol_assert_ret (rect != NULL, FALSE);
  return x < rect->x + rect->width &&
    y < rect->y + rect->height &&
    x >= rect->x &&
    y >= rect->y;
}

static gboolean
ol_osd_window_mouse_timer (gpointer data)
{
  /* ol_log_func (); */
  ol_assert_ret (data != NULL, FALSE);
  ol_assert_ret (OL_IS_OSD_WINDOW (data), FALSE);
  OlOsdWindow *osd = OL_OSD_WINDOW (data);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  GtkWidget *widget = GTK_WIDGET (osd);
  if (gtk_widget_get_realized (widget))
  {
    gint rel_x, rel_y;
    gboolean mouse_over = FALSE;
    gdk_window_get_pointer (widget->window, &rel_x, &rel_y, NULL);
    int i;
    for (i = 0; i < osd->line_count; i++)
    {
      if (_point_in_rect (rel_x, rel_y, &osd->lyric_rects[i]))
      {
        mouse_over = TRUE;
        break;
      }
    }
    if (priv->mouse_over_lyrics != mouse_over)
    {
      priv->mouse_over_lyrics = mouse_over;
      gtk_widget_queue_draw (widget);
    }
    if (ol_osd_window_get_mode (osd) == OL_OSD_WINDOW_DOCK)
    {
      GdkRectangle window_rect = {
        .x = 0,
        .y = 0,
        .width = widget->allocation.width,
        .height = widget->allocation.height,
      };
      gboolean mouse_over = _point_in_rect (rel_x, rel_y, &window_rect);
      if (mouse_over != priv->mouse_over)
      {
        priv->mouse_over = mouse_over;
        ol_osd_window_queue_reshape (osd);
        gtk_widget_queue_draw (widget);
      }
    }
    else
    {
      ol_osd_window_check_mouse_leave (osd);
    }
  }
  return TRUE;
}

void
ol_osd_window_set_width (OlOsdWindow *osd, gint width)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  priv->width = width;
  ol_osd_window_queue_resize (osd);
  ol_osd_window_queue_reshape (osd);
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

int
ol_osd_window_get_width (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), 0);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  return priv->width;
}

void
ol_osd_window_get_osd_size (OlOsdWindow *osd, gint *width, gint *height)
{
  /* ol_log_func (); */
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  if (width)
    *width = priv->width - BORDER_WIDTH * 2;
  if (height)
    *height = ol_osd_window_compute_osd_height (osd);
}

/* static void */
/* ol_osd_window_update_child_visibility (OlOsdWindow *osd) */
/* { */
/*   ol_assert (OL_IS_OSD_WINDOW (osd)); */
/*   OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd); */
/*   GtkWidget *child = gtk_bin_get_child (GTK_BIN (osd)); */
/*   if (child == NULL) */
/*     return; */
/*   if (priv->locked) */
/*     gtk_widget_hide (child); */
/*   else */
/*     gtk_widget_show (child); */
/* } */

void
ol_osd_window_set_locked (OlOsdWindow *osd, gboolean locked)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  if (priv->locked == locked)
    return;
  priv->locked = locked;
  if (gtk_widget_get_realized (GTK_WIDGET (osd))) {
    ol_osd_window_set_input_shape_mask (osd, locked);
  }
  ol_osd_window_queue_reshape (osd);
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

static void
ol_osd_window_update_child_size_requisition (OlOsdWindow *osd)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  GtkBin *bin = GTK_BIN (osd);
  if (bin->child && gtk_widget_get_visible (bin->child))
  {
    gtk_widget_size_request (bin->child, &priv->child_requisition);
  }
  else
  {
    priv->child_requisition.width = 0;
    priv->child_requisition.height = 0;
  }
}

static int
ol_osd_window_compute_window_width (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), 0);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  int w = priv->width;
  if (w < priv->child_requisition.width)
    w = priv->child_requisition.width;
  return w;
}

/**
 * Computes the height of the OSD Window
 *
 * The computation relies on the height of OSD text and child widget. It should be
 * called after ol_osd_window_update_child_size_requisition priv->osd_height
 * updated.
 *
 * Please note that the computed height does NOT consider the constrain of screen
 * @param osd
 *
 * @return The height of the widget
 */
static int
ol_osd_window_compute_window_height (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), 0);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  int height = BORDER_WIDTH * 2;
  height += priv->osd_height;
  height += priv->child_requisition.height;
  return height;
}

static int
ol_osd_window_compute_osd_height (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd) && osd->render_context != NULL,
                 DEFAULT_HEIGHT);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  int font_height = ol_osd_render_get_font_height (osd->render_context);

  int height = font_height *
    (osd->line_count + (osd->line_count - 1) * LINE_PADDING) +
    ol_osd_render_get_outline_width (osd->render_context);
  if (ol_osd_window_should_blur (osd))
    height += priv->blur_radius * 2.0;
  return height;
}

static void
ol_osd_window_update_child_allocation (OlOsdWindow *osd)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  GtkWidget *widget = GTK_WIDGET (osd);
  GtkBin *bin = GTK_BIN (osd);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  if (bin->child && gtk_widget_get_visible (bin->child))
  {
    ol_debugf ("set child allocation\n");
    GtkAllocation alloc;
    alloc.x = (widget->allocation.width - priv->child_requisition.width) / 2;
    alloc.width = priv->child_requisition.width;
    alloc.height = priv->child_requisition.height;
    alloc.y = widget->allocation.height
      - BORDER_WIDTH
      - priv->child_requisition.height;
    gtk_widget_size_allocate (bin->child, &alloc);
  }
}

static double
ol_osd_window_compute_lyric_xpos (OlOsdWindow *osd, int line, double percentage)
{
  /* ol_log_func (); */
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  gint w, h;
  int width;
  gboolean smooth = TRUE;
  ol_osd_window_get_osd_size (osd, &w, &h);
  if (priv->active_lyric_surfaces[line] != NULL)
  {
    width = cairo_image_surface_get_width (priv->active_lyric_surfaces[line]);
  }
  else
  {
    width = 0;
  }
  double xpos;
  if (w >= width)
  {
    xpos = (w - width) * osd->line_alignment[line];
  }
  else
  {
    OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
    if (!priv->composited &&
        ol_osd_window_get_mode (osd) == OL_OSD_WINDOW_DOCK)
      smooth = FALSE;
    if (smooth)
    {
      if (percentage * width < w / 2.0)
        xpos = 0;
      else if ((1.0 - percentage) * width < w / 2.0)
        xpos = w - width;
      else
        xpos = w / 2.0 - width * percentage;
    }
    else
    {
      if (percentage * width < w)
      {
        xpos = 0;
      }
      else
      {
        int half_count = (percentage * width - w) / w + 1;
        xpos = -half_count * w;
        if (xpos < w - width)
          xpos = w - width;
      }
      if (xpos != priv->lyric_xpos[line])
      {
        ol_osd_window_queue_reshape (osd);
        priv->lyric_xpos[line] = xpos;
      }
    }
  }
  return xpos;
}

static int
ol_osd_window_compute_lyric_ypos (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), 0);
  int y = BORDER_WIDTH;
  return y;
}

static void
ol_osd_window_paint_lyrics (OlOsdWindow *osd, cairo_t *cr)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  GtkWidget *widget = GTK_WIDGET (osd);
  if (!gdk_window_is_visible (widget->window))
    return;
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  double alpha = 1.0;
  int font_height = ol_osd_render_get_font_height (osd->render_context);
  if (priv->composited && priv->locked && priv->mouse_over_lyrics &&
      osd->translucent_on_mouse_over)
  {
    alpha = 0.3;
  }
  if (!gtk_widget_get_realized (widget))
    gtk_widget_realize (widget);
  gint w, h;
  int width, height;
  ol_osd_window_get_osd_size (osd, &w, &h);
  int line;
  gdouble ypos, xpos;
  ypos = ol_osd_window_compute_lyric_ypos (osd);
  int start, end;
  if (osd->line_count == 1)
  {
    start = osd->current_line;
    end = start + 1;
  }
  else
  {
    start = 0;
    end = OL_OSD_WINDOW_MAX_LINE_COUNT;
  }
  cairo_save (cr);
  cairo_rectangle (cr, BORDER_WIDTH, BORDER_WIDTH, w, h);
  cairo_clip (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  for (line = start; line < end; line++)
  {
    double percentage = osd->percentage[line];
    if (priv->active_lyric_surfaces[line] != NULL &&
        priv->inactive_lyric_surfaces[line])
    {
      width = cairo_image_surface_get_width (priv->active_lyric_surfaces[line]);
      height = cairo_image_surface_get_height (priv->active_lyric_surfaces[line]);
      xpos = ol_osd_window_compute_lyric_xpos (osd, line, osd->percentage[line]);
      xpos += BORDER_WIDTH;
      cairo_pattern_t *text_mask = ol_osd_window_create_text_mask (osd,
                                                                   cr,
                                                                   line,
                                                                   xpos,
                                                                   alpha);
      cairo_save (cr);
      cairo_rectangle (cr, xpos, ypos, (double)width * percentage, height);
      cairo_clip (cr);
      cairo_set_source_surface (cr, priv->active_lyric_surfaces[line], xpos, ypos);
      if (text_mask)
        cairo_mask (cr, text_mask);
      else
        cairo_paint_with_alpha (cr, alpha);
      cairo_restore (cr);
      cairo_save (cr);
      cairo_rectangle (cr,
                       xpos + width * percentage,
                       ypos,
                       (double)width * (1.0 - percentage), height);
      cairo_clip (cr);
      cairo_set_source_surface (cr, priv->inactive_lyric_surfaces[line], xpos, ypos);
      if (text_mask)
        cairo_mask (cr, text_mask);
      else
        cairo_paint_with_alpha (cr, alpha);
      if (text_mask)
        cairo_pattern_destroy (text_mask);
      cairo_restore (cr);
    }
    ypos += font_height * (1 + LINE_PADDING);
  }
  cairo_restore (cr);
}

static cairo_pattern_t *
ol_osd_window_create_text_mask (OlOsdWindow *osd,
                                cairo_t *cr,
                                int line,
                                gdouble text_xpos,
                                gdouble alpha)
{
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  int left, width;
  ol_osd_window_get_osd_size (osd, &width, NULL);
  left = BORDER_WIDTH;
  int text_width = cairo_image_surface_get_width (priv->active_lyric_surfaces[line]);
  int fade_in_size = DEFAULT_FADE_IN_SIZE;
  if (fade_in_size * 2 > width)
    fade_in_size = width / 2;
  if (priv->composited)
  {
    cairo_pattern_t *pattern = cairo_pattern_create_linear (left,
                                                            0.0,
                                                            left + width,
                                                            0.0);
    /* Set fade in on left edge */
    if (text_xpos < left)
    {
      cairo_pattern_add_color_stop_rgba (pattern,
                                         0.0, /* offset */
                                         1.0, 1.0, 1.0, 0.0); /* r,g,b,a */
      gdouble loffset = 0.0;
      if (left - text_xpos < fade_in_size)
        loffset = (gdouble) (left - text_xpos) / (gdouble) width;
      else
        loffset = (gdouble) fade_in_size / (gdouble) width;
      cairo_pattern_add_color_stop_rgba (pattern,
                                         loffset, /* offset */
                                         1.0, 1.0, 1.0, alpha); /* r,g,b,a */
    }
    else
    {
      cairo_pattern_add_color_stop_rgba (pattern,
                                         0.0, /* offset */
                                         1.0, 1.0, 1.0, alpha); /* r,g,b,a */
    }
    /* Set fade out on the right edge */
    if (text_xpos + text_width > left + width)
    {
      gdouble roffset = 0.0;
      if (text_xpos + text_width - (left + width) < fade_in_size)
        roffset = 1.0 - (gdouble) (text_xpos + text_width - (left + width)) / (gdouble) width;
      else
        roffset = 1.0 - (gdouble) (fade_in_size) / (gdouble) width;
      cairo_pattern_add_color_stop_rgba (pattern,
                                         roffset, /* offset */
                                         0.0, 0.0, 0.0, alpha); /* r,g,b,a */
      cairo_pattern_add_color_stop_rgba (pattern,
                                         1.0, /* offset */
                                         0.0, 0.0, 0.0, 0.0); /* r,g,b,a */
    }
    else
    {
      cairo_pattern_add_color_stop_rgba (pattern,
                                         1.0, /* offset */
                                         0.0, 0.0, 0.0, alpha); /* r,g,b,a */
    }
    return pattern;
  }
  return NULL;
}

static void
ol_osd_window_clear_cairo (cairo_t *cr)
{
  ol_assert (cr != NULL);
  cairo_save (cr);
  cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint(cr);
  cairo_restore (cr);
}

void
ol_osd_window_set_percentage (OlOsdWindow *osd, gint line, double percentage)
{
  if (line < 0 || line >= OL_OSD_WINDOW_MAX_LINE_COUNT)
    return;
  ol_assert (OL_IS_OSD_WINDOW (osd));
  if (percentage == osd->percentage[line])
    return;
  double old_percentage = osd->percentage[line];
  osd->percentage[line] = percentage;
  /* update shape if line is too long */
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  if (!priv->composited && percentage != old_percentage)
  {
    int old_x = ol_osd_window_compute_lyric_xpos (osd, line, old_percentage);
    int new_x = ol_osd_window_compute_lyric_xpos (osd, line, percentage);
    if (old_x != new_x)
      priv->update_shape = TRUE;
  }
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

void
ol_osd_window_set_current_percentage (OlOsdWindow *osd, double percentage)
{
  ol_osd_window_set_percentage (osd, osd->current_line, percentage);
}

double
ol_osd_window_get_current_percentage (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), 0.0);
  return osd->percentage[osd->current_line];
}

void
ol_osd_window_set_current_line (OlOsdWindow *osd, gint line)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  ol_assert (line < ol_osd_window_get_line_count (osd));
  osd->current_line = line;
}

gint ol_osd_window_get_current_line (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), 0);
  return osd->current_line;
}

static void
ol_osd_draw_lyric_surface (OlOsdWindow *osd, cairo_surface_t **surface, const char *lyric)
{
  if (*surface != NULL)
  {
    cairo_surface_destroy (*surface);
    *surface = NULL;
  }
  if (!ol_is_string_empty (lyric) && *surface == NULL)
  {
    int w, h;
    if (!gtk_widget_get_realized (GTK_WIDGET (osd)))
      gtk_widget_realize (GTK_WIDGET (osd));
    ol_osd_render_get_pixel_size (osd->render_context,
                                  lyric,
                                  &w, &h);
    *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
    cairo_t *cr = cairo_create (*surface);
    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
    ol_osd_render_paint_text (osd->render_context,
                              cr,
                              lyric,
                              0,
                              0);
    cairo_destroy (cr);
  }
}

static void
ol_osd_window_update_lyric_surface (OlOsdWindow *osd, int line)
{
  int i;
  if (!gtk_widget_get_realized (GTK_WIDGET (osd)))
    return;
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  /* draws the inactive surfaces */
  for (i = 0; i < OL_LINEAR_COLOR_COUNT; i++)
  {
    ol_osd_render_set_linear_color (osd->render_context,
                                    i,
                                    osd->inactive_colors[i]);
  }
  ol_osd_draw_lyric_surface (osd,
                             &priv->inactive_lyric_surfaces[line],
                             osd->lyrics[line]);
  /* draws the active surfaces */
  for (i = 0; i < OL_LINEAR_COLOR_COUNT; i++)
  {
    ol_osd_render_set_linear_color (osd->render_context,
                                    i,
                                    osd->active_colors[i]);
  }
  ol_osd_draw_lyric_surface (osd,
                             &priv->active_lyric_surfaces[line],
                             osd->lyrics[line]);
  ol_osd_window_update_lyric_rect (osd, line);
}

static void
ol_osd_window_update_lyric_rect (OlOsdWindow *osd, int line)
{
  ol_log_func ();
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  int w, h;
  w = h = 0;
  if (priv->active_lyric_surfaces[line] != NULL)
  {
    w = cairo_image_surface_get_width (priv->active_lyric_surfaces[line]);
    h = cairo_image_surface_get_height (priv->active_lyric_surfaces[line]);
  }
  int font_height = ol_osd_render_get_font_height (osd->render_context);
  osd->lyric_rects[line].x = ol_osd_window_compute_lyric_xpos (osd,
                                                               line,
                                                               osd->percentage[line]);
  osd->lyric_rects[line].y = font_height * line * (1 + LINE_PADDING);
  osd->lyric_rects[line].width = w;
  osd->lyric_rects[line].height = h;
}

void
ol_osd_window_set_lyric (OlOsdWindow *osd, gint line, const char *lyric)
{
  ol_debugf ("  lyric:%s\n", lyric);
  ol_assert (OL_IS_OSD_WINDOW (osd));
  ol_assert (line >= 0 && line < OL_OSD_WINDOW_MAX_LINE_COUNT);
  if (osd->lyrics[line] != NULL)
    g_free (osd->lyrics[line]);
  if (lyric != NULL)
  {
    if (g_utf8_strlen (lyric, -1) > MAX_LYRIC_LEN)
      osd->lyrics[line] = g_strndup (lyric,
                                     g_utf8_offset_to_pointer (lyric, MAX_LYRIC_LEN) - lyric);
    else
      osd->lyrics[line] = g_strdup (lyric);
  }
  else
  {
    osd->lyrics[line] = g_strdup ("");
  }
  ol_osd_window_update_lyric_surface (osd, line);
  /* We have to call ol_osd_window_update_shape instead of
     ol_osd_window_queue_reshape here, because there might be empty shape
     before setting lyric, in which case expose event will not be emitted,
     thus the shape will not be updated at all. */
  ol_osd_window_update_shape (osd);
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

void
ol_osd_window_set_line_alignment (OlOsdWindow *osd, gint line, double alignment)
{
  ol_debugf ("%s:%d-%lf\n", __FUNCTION__, line, alignment);
  ol_assert (OL_IS_OSD_WINDOW (osd));
  if (line < 0 || line >= OL_OSD_WINDOW_MAX_LINE_COUNT)
    return;
  if (alignment < 0.0)
    alignment = 0.0;
  else if (alignment > 1.0)
    alignment = 1.0;
  osd->line_alignment[line] = alignment;
  ol_osd_window_update_lyric_rect (osd, line);
  ol_osd_window_queue_reshape (osd);
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

static void
ol_osd_window_reset_shape_pixmap (OlOsdWindow *osd)
{
  /* init shape pixmap */
  ol_assert (OL_IS_OSD_WINDOW (osd));
  if (!gtk_widget_get_realized (GTK_WIDGET (osd)))
    return;
  GtkWidget *widget = GTK_WIDGET (osd);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  gint width = priv->width;
  gint height = ol_osd_window_compute_window_height (osd);
  if (priv->shape_pixmap != NULL)
  {
    gint w, h;
    gdk_drawable_get_size (priv->shape_pixmap, &w, &h);
    if (w == width && h == height)
    {
      return;
    }
    else
    {
      g_object_unref (priv->shape_pixmap);
      priv->shape_pixmap = NULL;
    }
  }
  priv->shape_pixmap = gdk_pixmap_new (widget->window, width, height, 1);
  cairo_t *cr = gdk_cairo_create (priv->shape_pixmap);
  ol_osd_window_clear_cairo (cr);
  cairo_destroy (cr);
  ol_osd_window_queue_reshape (osd);
}

static void
ol_osd_window_queue_reshape (OlOsdWindow *osd)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  priv->update_shape = TRUE;
}

static void
ol_osd_window_update_shape (OlOsdWindow *osd)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  GtkWidget *widget = GTK_WIDGET (osd);
  static gboolean empty_mask = TRUE;
  if (!gtk_widget_get_realized (widget))
    return;
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  if (priv->composited ||
      ol_osd_window_panel_visible (osd))
  {
    if (!empty_mask)
    {
      empty_mask = TRUE;
      gdk_window_shape_combine_mask (widget->window, NULL, 0, 0);
    }
    return;
  }
  empty_mask = FALSE;
  GdkPixmap *shape_mask = priv->shape_pixmap;
  GdkGC *fg_gc = gdk_gc_new (shape_mask);
  GdkColor color;
  color.pixel = 0;
  gdk_gc_set_foreground (fg_gc, &color);
  gdk_gc_set_background (fg_gc, &color);
  cairo_t *cr = gdk_cairo_create (shape_mask);
  ol_osd_window_clear_cairo (cr);
  ol_osd_window_paint_lyrics (osd, cr);
  cairo_destroy (cr);
  gdk_window_shape_combine_mask (widget->window, shape_mask, 0, 0);
  priv->update_shape = FALSE;
  g_object_unref (fg_gc);
}

static void
ol_osd_window_set_input_shape_mask (OlOsdWindow *osd, gboolean disable_input)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  GtkWidget *widget = GTK_WIDGET (osd);
  ol_assert (gtk_widget_get_realized (widget));
  if (disable_input)
  {
    GdkRegion *region = gdk_region_new ();
    gdk_window_input_shape_combine_region (GTK_WIDGET (osd)->window, region, 0, 0);
    gdk_region_destroy (region);
  }
  else
  {
    gdk_window_input_shape_combine_region (GTK_WIDGET (osd)->window, NULL, 0, 0);
  }
}

/**
 * Computes the position and size when constraining the window to be inside a screen
 *
 * @param osd
 * @param x
 * @param y
 * @param width
 * @param height
 * @param drag_state The hint of drag type. If the window is outside the screen,
 *                   drag_type determines how to constrain the window:
 *                   DRAG_MOVE: constrain the window by change position
 *                   DRAG_EAST: constrain the window by shrinking the width
 *                   DRAG_WEST: constrain the window by shrinking the width and
 *                              change the x position so that the right edge of the
 *                              window will not change
 *                   DRAG_NONE: acts as DRAG_MOVE

 */
static void
ol_osd_window_compute_constrain (OlOsdWindow *osd,
                                 int *x,
                                 int *y,
                                 int *width,
                                 int *height,
                                 enum DragState drag_state)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  ol_assert (x != NULL);
  ol_assert (y != NULL);
  ol_assert (width != NULL);
  ol_assert (height != NULL);
  GtkWidget *widget = GTK_WIDGET (osd);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  GdkScreen *screen = gtk_widget_get_screen (widget);
  if (ol_osd_window_get_mode (osd) == OL_OSD_WINDOW_DOCK &&
      screen != NULL)
  {
    int sw = gdk_screen_get_width (screen);
    int sh = gdk_screen_get_height (screen);
    switch (drag_state)
    {
    case DRAG_EAST:
      *x = MAX (0, *x);
      *width = MIN (*width, sw - *x);
      break;
    case DRAG_WEST:
      *width = MIN (*width, *x + *width);
      *x = MAX (0, MIN (*x, priv->old_x + priv->old_width - *width));
      break;
    default:
      *x = MAX (0, MIN (*x, sw - *width));
    }
    if (*y + *height > sh) {
      int minh = ol_osd_window_compute_osd_height (osd) +
        BORDER_WIDTH * 2;
      *height = MAX (minh, sh - *y);
      *y = MAX (0, sh - *height);
    } else {
      *y = MAX (0, *y);
    }
  }
}

static void
ol_osd_window_queue_resize (OlOsdWindow *osd)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  GtkWidget *widget = GTK_WIDGET (osd);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  if (!gtk_widget_get_realized (widget))
    return;
  ol_osd_window_update_layout (osd);
  int h = ol_osd_window_compute_window_height (osd);
  int w = ol_osd_window_compute_window_width (osd);
  ol_osd_window_move_resize (osd,
                             priv->raw_x,
                             priv->raw_y,
                             w,
                             h,
                             DRAG_NONE);
}

/**
 * Move and resize the OSD Window.
 *
 * The coordinations and size are raw values, which means does not honor the size
 * of the child widget.
 *
 * @param osd
 * @param raw_x
 * @param raw_y
 * @param width
 * @param drag_state See long comment of ol_osd_window_compute_constrain
*/
static void
ol_osd_window_move_resize (OlOsdWindow *osd,
                           int x,
                           int y,
                           int width,
                           int height,
                           enum DragState drag_state)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  GtkWidget *widget = GTK_WIDGET (osd);
  gboolean size_changed = FALSE;
  gboolean pos_changed = FALSE;
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  int old_x, old_y, old_w, old_h;
  gtk_window_get_position (GTK_WINDOW (osd), &old_x, &old_y);
  gtk_window_get_size (GTK_WINDOW (osd), &old_w, &old_h);
  ol_osd_window_compute_constrain (osd, &x, &y, &width, &height, drag_state);
  if (height != old_h ||
      width != old_w)
    size_changed = TRUE;
  if (priv->raw_x != x || priv->raw_y != y ||
      old_x != x || old_y != y)
    pos_changed = TRUE;
  priv->raw_x = x;
  priv->raw_y = y;
  if (pos_changed)
  {
    gtk_window_move (GTK_WINDOW (osd), x, y);
  }
  if (size_changed || pos_changed)
  {
    /* gtk_window_resize and ol_osd_window_queue_resize don't work,
       so we have to specify allocation explicitly*/
    /* GtkAllocation newalloc = { */
    /*   .x = 0, */
    /*   .y = 0, */
    /*   .width = width, */
    /*   .height = height, */
    /* }; */
    /* gtk_widget_size_allocate (widget, &newalloc); */
    gtk_window_resize (GTK_WINDOW (osd), width, height);
    /* ol_osd_window_queue_resize (widget); */
    ol_osd_window_queue_reshape (osd);
    gtk_widget_queue_draw (widget);
  }
}

void
ol_osd_window_move (OlOsdWindow *osd, int x, int y)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  ol_osd_window_move_resize (osd, x, y,
                             ol_osd_window_compute_window_width (osd),
                             ol_osd_window_compute_window_height (osd),
                             DRAG_MOVE);
}

void
ol_osd_window_get_pos (OlOsdWindow *osd, int *x, int *y)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  if (x)
    *x = priv->raw_x;
  if (y)
    *y = priv->raw_y;
}

static void
ol_osd_window_class_init (OlOsdWindowClass *klass)
{
  ol_log_func ();
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  object_class = (GtkObjectClass*)klass;
  widget_class = (GtkWidgetClass*)klass;
  ol_osd_window_parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = ol_osd_window_set_property;
  gobject_class->get_property = ol_osd_window_get_property;

  object_class->destroy = ol_osd_window_destroy;

  widget_class->size_allocate = ol_osd_window_size_allocate;
  widget_class->size_request = ol_osd_window_size_request;
  widget_class->enter_notify_event = ol_osd_window_enter_notify;
  widget_class->leave_notify_event = ol_osd_window_leave_notify;

  /* set up properties */
  g_object_class_install_property (gobject_class,
                                   PROP_LOCKED,
                                   g_param_spec_boolean ("locked",
                                                         ("Whether the it is possible to move the OSD window"),
                                                         ("If TRUE, there will be a window displayed under the"
                                                          "OSD indicating that it can be moved, and so it is"),
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_WRITABLE));
  /* install signals */
  klass->signals[OSD_MOVED] =
    g_signal_newv ("moved",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  NULL,         /* closure */
                  NULL,         /* accumulator */
                  NULL,         /* accumulator data */
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0,
                  NULL);
  klass->signals[OSD_RESIZE] =
    g_signal_newv ("resize",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  NULL,         /* closure */
                  NULL,         /* accumulator */
                  NULL,         /* accumulator data */
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0,
                  NULL);
}

static void
ol_osd_window_set_property (GObject *object, guint prop_id,
                            const GValue *value, GParamSpec *pspec)
{
  OlOsdWindow *osd = OL_OSD_WINDOW (object);
  switch (prop_id)
  {
  case PROP_LOCKED:
    ol_assert (G_VALUE_HOLDS_BOOLEAN (value));
    ol_osd_window_set_locked (osd, g_value_get_boolean (value));
    break;
  }
}

static void
ol_osd_window_get_property (GObject *object, guint prop_id,
                            GValue *value, GParamSpec *pspec)
{
  OlOsdWindow *osd = OL_OSD_WINDOW (object);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  switch (prop_id)
  {
  case PROP_LOCKED:
    g_value_set_boolean (value, priv->locked);
    break;
  }
}

static void
ol_osd_window_init (OlOsdWindow *osd)
{
  /* Allocate Private data structure */
  (OL_OSD_WINDOW(osd))->priv = \
      (OlOsdWindowPrivate *) g_malloc0(sizeof(OlOsdWindowPrivate));
  /* If correctly allocated, initialize parameters */
  if((OL_OSD_WINDOW(osd))->priv != NULL)
  {
    ol_log_func ();
    GtkWindow *window = GTK_WINDOW (osd);
    OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE(osd);

    window->type = GTK_WINDOW_TOPLEVEL;
    GValue allow_shrink = {0};
    g_value_init (&allow_shrink, G_TYPE_BOOLEAN);
    g_value_set_boolean (&allow_shrink, TRUE);
    g_object_set_property (G_OBJECT (window), "allow-shrink", &allow_shrink);
    g_value_unset (&allow_shrink);

    gtk_window_set_decorated (window, FALSE);
    gtk_widget_set_app_paintable (GTK_WIDGET (osd), TRUE);
    osd->current_line = 0;
    osd->bg_pixbuf = NULL;
    osd->line_count = 1;
    int i;
    for (i = 0; i < OL_OSD_WINDOW_MAX_LINE_COUNT; i++)
    {
      osd->lyrics[i] = NULL;
      osd->line_alignment[i] = 0.5;
      osd->percentage[i] = 0.0;
      priv->active_lyric_surfaces[i] = NULL;
      priv->inactive_lyric_surfaces[i] = NULL;
      osd->lyric_rects[i].x = 0;
      osd->lyric_rects[i].y = 0;
      osd->lyric_rects[i].width = 0;
      osd->lyric_rects[i].height = 0;
    }
    for (i = 0; i < OL_LINEAR_COLOR_COUNT; i++)
    {
      osd->active_colors[i] = DEFAULT_ACTIVE_COLORS[i];
      osd->inactive_colors[i] = DEFAULT_INACTIVE_COLORS[i];
    }
    osd->render_context = ol_osd_render_context_new ();
    osd->translucent_on_mouse_over = FALSE;
    /* initilaize private data */
    priv->shape_pixmap = NULL;
    priv->width = DEFAULT_WIDTH;
    priv->locked = TRUE;
    priv->composited = FALSE;
    priv->visible = FALSE;
    priv->mouse_timer_id = 0;
    priv->configure_event = 0;
    priv->child_requisition.width = 0;
    priv->child_requisition.height = 0;
    priv->mouse_over_lyrics = FALSE;
    priv->composited_signal = 0;
    priv->raw_x = 0;
    priv->raw_y = 0;
    priv->drag_state = DRAG_NONE;
    priv->update_shape = FALSE;
    priv->blur_radius = 0.0;
    /* ol_osd_window_set_mode (osd, OL_OSD_WINDOW_DOCK); */
    ol_osd_window_set_mode (osd, OL_OSD_WINDOW_NORMAL);
    ol_osd_window_update_colormap (osd);
    ol_osd_window_screen_composited_changed (gtk_widget_get_screen (GTK_WIDGET (osd)),
                                             osd);
    gtk_widget_add_events(GTK_WIDGET(osd),
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);
    g_signal_connect (G_OBJECT (osd), "button-press-event",
                      G_CALLBACK (ol_osd_window_button_press), osd);
    g_signal_connect (G_OBJECT (osd), "button-release-event",
                      G_CALLBACK (ol_osd_window_button_release), osd);
    g_signal_connect (G_OBJECT (osd), "motion-notify-event",
                      G_CALLBACK (ol_osd_window_motion_notify), osd);
    g_signal_connect (G_OBJECT (osd), "expose-event",
                      G_CALLBACK (ol_osd_window_expose_before), osd);
    g_signal_connect_after (G_OBJECT (osd), "expose-event",
                            G_CALLBACK (ol_osd_window_expose_after), osd);
    g_signal_connect (G_OBJECT (osd), "map-event",
                      G_CALLBACK (ol_osd_window_map_cb), osd);
    g_signal_connect (G_OBJECT (osd), "unmap-event",
                      G_CALLBACK (ol_osd_window_unmap_cb), osd);
    g_signal_connect (G_OBJECT (osd), "realize",
                      G_CALLBACK (ol_osd_window_realize_cb), osd);
    g_signal_connect (G_OBJECT (osd), "unrealize",
                      G_CALLBACK (ol_osd_window_unrealize_cb), osd);
  }
}

static void
ol_osd_window_dispose(GObject *object)
{
    OlOsdWindow *self = (OlOsdWindow *)object;
    OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE(self);
    /* Check if not NULL! To avoid calling dispose multiple times */
    if(priv != NULL)
    {
        /* Deallocate contents of the private data, if any */
        /* Deallocate private data structure */
        g_free(priv);
        /* And finally set the opaque pointer back to NULL, so that
         *  we don't deallocate it twice. */
        (OL_OSD_WINDOW(self))->priv = NULL;
    }
}

GtkWidget*
ol_osd_window_new ()
{
  ol_log_func ();
  OlOsdWindow *osd;
  osd = g_object_new (ol_osd_window_get_type (), NULL);
  return GTK_WIDGET (osd);
}

static void
ol_osd_window_destroy (GtkObject *object)
{
  ol_log_func ();
  ol_assert (OL_IS_OSD_WINDOW (object));
  OlOsdWindow *osd = OL_OSD_WINDOW (object);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  int i;
  if (priv->mouse_timer_id > 0)
  {
    g_source_remove (priv->mouse_timer_id);
    priv->mouse_timer_id = 0;
  }
  for (i = 0; i < OL_OSD_WINDOW_MAX_LINE_COUNT; i++)
  {
    if (osd->lyrics[i] != NULL)
    {
      g_free (osd->lyrics[i]);
      osd->lyrics[i] = NULL;
    }
    if (priv->active_lyric_surfaces[i] != NULL)
    {
      cairo_surface_destroy (priv->active_lyric_surfaces[i]);
      priv->active_lyric_surfaces[i] = NULL;
    }
    if (priv->inactive_lyric_surfaces[i] != NULL)
    {
      cairo_surface_destroy (priv->inactive_lyric_surfaces[i]);
      priv->inactive_lyric_surfaces[i] = NULL;
    }
  }
  if (priv->shape_pixmap != NULL)
  {
    g_object_unref (priv->shape_pixmap);
    priv->shape_pixmap = NULL;
  }
  if (osd->render_context != NULL)
  {
    ol_osd_render_context_destroy (osd->render_context);
    osd->render_context = NULL;
  }
  if (osd->bg_pixbuf != NULL)
  {
    g_object_unref (osd->bg_pixbuf);
    osd->bg_pixbuf = NULL;
  }
  GTK_OBJECT_CLASS (ol_osd_window_parent_class)->destroy (object);
}

void
ol_osd_window_set_font_name (OlOsdWindow *osd,
                             const char *font_name)
{
  if (osd == NULL || osd->render_context == NULL || font_name == NULL)
    return;
  ol_osd_render_set_font_name (osd->render_context,
                               font_name);
  int i;
  for (i = 0; i < osd->line_count; i++)
    ol_osd_window_update_lyric_surface (osd, i);
  ol_osd_window_queue_resize (osd);
  ol_osd_window_queue_reshape (osd);
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

char*
ol_osd_window_get_font_name (OlOsdWindow *osd)
{
  if (osd == NULL || osd->render_context == NULL)
    return NULL;
  return ol_osd_render_get_font_name (osd->render_context);
}

void
ol_osd_window_set_outline_width (OlOsdWindow *osd,
                                 const int width)
{
  ol_assert (osd != NULL);
  ol_assert (osd->render_context != NULL);
  ol_osd_render_set_outline_width (osd->render_context, width);
  int i;
  for (i = 0; i < osd->line_count; i++)
    ol_osd_window_update_lyric_surface (osd, i);
  ol_osd_window_queue_resize (osd);
  ol_osd_window_queue_reshape (osd);
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

int
ol_osd_window_get_outline_width (OlOsdWindow *osd)
{
  ol_assert_ret (osd != NULL, 0);
  ol_assert_ret (osd->render_context != NULL, 0);
  return ol_osd_render_get_outline_width (osd->render_context);

}

void
ol_osd_window_set_active_colors (OlOsdWindow *osd,
                                 OlColor top_color,
                                 OlColor middle_color,
                                 OlColor bottom_color)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  osd->active_colors[0] = top_color;
  osd->active_colors[1] = middle_color;
  osd->active_colors[2] = bottom_color;
  int i;
  for (i = 0; i < osd->line_count; i++)
    ol_osd_window_update_lyric_surface (osd, i);
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

void
ol_osd_window_set_inactive_colors (OlOsdWindow *osd,
                                   OlColor top_color,
                                   OlColor middle_color,
                                   OlColor bottom_color)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  osd->inactive_colors[0] = top_color;
  osd->inactive_colors[1] = middle_color;
  osd->inactive_colors[2] = bottom_color;
  int i;
  for (i = 0; i < osd->line_count; i++)
    ol_osd_window_update_lyric_surface (osd, i);
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

void
ol_osd_window_set_line_count (OlOsdWindow *osd,
                              guint line_count)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  ol_assert (line_count >= 1 && line_count <= 2);
  osd->line_count = line_count;
  ol_osd_window_queue_resize (osd);
  ol_osd_window_queue_reshape (osd);
}

guint
ol_osd_window_get_line_count (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), 1);
  return osd->line_count;
}

void
ol_osd_window_set_translucent_on_mouse_over (OlOsdWindow *osd,
                                             gboolean value)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  osd->translucent_on_mouse_over = value;
}

gboolean
ol_osd_window_get_translucent_on_mouse_over (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), FALSE);
  return osd->translucent_on_mouse_over;
}

void
ol_osd_window_set_bg (OlOsdWindow *osd, GdkPixbuf *bg)
{
  ol_log_func ();
  ol_debugf ("bg: %p\n", bg);
  ol_assert (OL_IS_OSD_WINDOW (osd));
  if (bg != NULL)
    g_object_ref (bg);
  if (osd->bg_pixbuf != NULL)
    g_object_unref (osd->bg_pixbuf);
  osd->bg_pixbuf = bg;
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

void
ol_osd_window_set_mode (OlOsdWindow *osd, enum OlOsdWindowMode mode)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  GtkWindow *window = GTK_WINDOW (osd);
  GtkWidget *widget = GTK_WIDGET (osd);
  if (mode == priv->mode)
    return;
  gboolean realized = gtk_widget_get_realized (widget);
  gboolean mapped = gtk_widget_get_mapped (widget);
  if (mapped)
    gtk_widget_unmap (widget);
  if (realized)
    gtk_widget_unrealize (widget);
  switch (mode)
  {
  case OL_OSD_WINDOW_NORMAL:
    gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_NORMAL);
    window->type = GTK_WINDOW_TOPLEVEL;
    break;
  case OL_OSD_WINDOW_DOCK:
    gtk_window_set_type_hint (window, GDK_WINDOW_TYPE_HINT_DOCK);
    window->type = GTK_WINDOW_POPUP;
    break;
  default:
    ol_errorf ("Invalid OSD Window type %d\n", mode);
    mode = priv->mode;
  }
  priv->mode = mode;
  ol_osd_window_update_render_blur_radius (osd);
  if (realized)
    gtk_widget_realize (widget);
  if (mapped) {
    gtk_widget_map (widget);
    /* We need to specify the position so that OSD window keeps its place. */
    ol_osd_window_queue_resize (osd);
  }
  ol_osd_window_queue_reshape (osd);
}

enum OlOsdWindowMode
ol_osd_window_get_mode (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), OL_OSD_WINDOW_NORMAL);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  return priv->mode;
}

static gboolean
ol_osd_window_should_blur (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), FALSE);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  return priv->composited || priv->mode == OL_OSD_WINDOW_NORMAL;
}

/**
 * Sets the blur radius of renderer if possible.
 */
static void
ol_osd_window_update_render_blur_radius (OlOsdWindow *osd)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  if (ol_osd_window_should_blur (osd))
    ol_osd_render_set_blur_radius (osd->render_context, priv->blur_radius);
  else
    ol_osd_render_set_blur_radius (osd->render_context, 0.0);
}

void
ol_osd_window_set_blur_radius (OlOsdWindow *osd, double radius)
{
  ol_assert (OL_IS_OSD_WINDOW (osd));
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  int i;
  priv->blur_radius = radius;
  ol_osd_window_update_render_blur_radius (osd);
  for (i = 0; i < osd->line_count; i++)
    ol_osd_window_update_lyric_surface (osd, i);
  ol_osd_window_queue_resize (osd);
  gtk_widget_queue_draw (GTK_WIDGET (osd));
}

double
ol_osd_window_get_blur_radius (OlOsdWindow *osd)
{
  ol_assert_ret (OL_IS_OSD_WINDOW (osd), 0.0);
  OlOsdWindowPrivate *priv = OL_OSD_WINDOW_GET_PRIVATE (osd);
  return priv->blur_radius;
}
