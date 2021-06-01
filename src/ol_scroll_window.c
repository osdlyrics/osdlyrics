/* -*- mode: C; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2010  Sarlmol Apple <sarlmolapple@gmail.com>
 * Copyright (C) 2011  Tiger Soldier <tigersoldier@gmail.com>
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
#include <math.h>
#include <glib.h>
#include <pango/pangocairo.h>

#include "ol_scroll_window.h"
#include "ol_intl.h"
#include "ol_marshal.h"
#include "ol_color.h"
#include "ol_debug.h"

#define OL_SCROLL_WINDOW_GET_PRIVATE(obj) \
   ((OlScrollWindowPrivate *)((OL_SCROLL_WINDOW(obj))->priv))

/*************default setting******************/
static const gint DEFAULT_WIDTH = 600;
static const gint DEFAULT_HEIGHT = 600;
static const gint DEFAULT_LINE_COUNT = 20;
static const OlColor DEFAULT_ACTIVE_COLOR = {0.89,0.81,0};
static const OlColor DEFAULT_INACTIVE_COLOR = {0.98,0.92,0.84};
static const OlColor DEFAULT_BG_COLOR = {0,0,0};
static const char *DEFAULT_FONT_NAME = "serif 13";
static const gint DEFAULT_LINE_MARGIN = 7;
static const gint DEFAULT_PADDING_X = 10;
static const gint DEFAULT_PADDING_Y = 5;
static const gint DEFAULT_CORNER_RADIUS = 10;
static const gint DEFAULT_FRAME_WIDTH = 7;
static const double DEFAULT_BG_OPACITY = 0.9;
static const gchar *TOOLTIP_WITH_SEEK = N_ ("Drag to move the window\nHold CTRL to seek");
static const gchar *TOOLTIP_WITHOUT_SEEK = N_ ("Drag to move the window");
/**********************************************/


typedef struct __OlScrollWindowPrivate OlScrollWindowPrivate;
struct __OlScrollWindowPrivate
{
  gint line_count;
  OlColor active_color;            /*Color of playing lyric*/
  OlColor inactive_color;          /*Color of non-playing lyric*/
  OlColor bg_color;
  gchar *font_name;                /*Font string, including family, size, and style*/
  gint line_margin;                /*Margin between two lines*/
  gint padding_x;
  gint padding_y;
  gint frame_width;
  gint corner_radius;
  double bg_opacity;
  char *text;
  gint saved_lrc_y;
  GtkWidget *window_container;
  GtkContainer *toolbar_container;
  enum OlScrollWindowScrollMode scroll_mode;
  gboolean can_seek;
  gboolean seeking;
  gint saved_lyric_id;
  gint saved_seek_offset;
  gint saved_pointer_y;
  gint current_pointer_y;
};

enum {
  SEEK_SIGNAL,
  LAST_SIGNAL,
};

static guint _signals[LAST_SIGNAL] = {0};

static cairo_t* _get_cairo (OlScrollWindow *scroll, GtkWidget *widget);
static PangoLayout* _get_pango (OlScrollWindow *scroll, cairo_t *cr);
static void _paint_bg (OlScrollWindow *scroll, cairo_t *cr);
static void _paint_lyrics (OlScrollWindow *scroll, cairo_t *cr);
static void _paint_text (OlScrollWindow *scroll, cairo_t *cr);
static gint _calc_lrc_ypos (OlScrollWindow *scroll, double percentage);
static void _calc_paint_pos (OlScrollWindow *scroll,
                             gint *lyric_id,
                             gint *lrc_y);

static void ol_scroll_window_init (OlScrollWindow *self);
static void ol_scroll_window_class_init (OlScrollWindowClass *klass);
static void ol_scroll_window_destroy (GtkObject *object);
static gboolean ol_scroll_window_expose (GtkWidget *widget,
                                         GdkEventExpose *event,
                                         gpointer userdata);

static int ol_scroll_window_compute_line_count (OlScrollWindow *scroll);
static int ol_scroll_window_get_font_height (OlScrollWindow *scroll);

static PangoLayout* _get_pango (OlScrollWindow *scroll, cairo_t *cr);

static gboolean ol_scroll_window_button_press (GtkWidget *widget,
                                               GdkEventButton *event);
static gboolean ol_scroll_window_button_release (GtkWidget *widget,
                                               GdkEventButton *event);
static gboolean ol_scroll_window_motion_notify (GtkWidget *widget,
                                                GdkEventMotion *event);
static void ol_scroll_window_begin_move_resize (GtkWidget *widget,
                                                GdkEventButton *event);
static void ol_scroll_window_begin_seek (OlScrollWindow *scroll,
                                         GdkEventButton *event);
static void ol_scroll_window_update_cursor (GtkWidget *widget,
                                            GdkEventMotion *event);
static void ol_scroll_window_seek (OlScrollWindow *scroll,
                                   GdkEventMotion *event);
static void ol_scroll_window_end_seek (OlScrollWindow *scroll);
static void ol_scroll_window_update_tooltip (OlScrollWindow *scroll);

G_DEFINE_TYPE_WITH_PRIVATE (OlScrollWindow, ol_scroll_window, GTK_TYPE_WINDOW);


GtkWidget*
ol_scroll_window_new ()
{
  OlScrollWindow *scroll;
  scroll = g_object_new (ol_scroll_window_get_type (), NULL);
  return GTK_WIDGET (scroll);
}

static void
ol_scroll_window_init (OlScrollWindow *self)
{
  /* Allocate Private data structure */
  (OL_SCROLL_WINDOW(self))->priv = \
      (OlScrollWindowPrivate *) g_malloc0(sizeof(OlScrollWindowPrivate));
  /* If correctly allocated, initialize parameters */
  if((OL_SCROLL_WINDOW(self))->priv != NULL)
  {
    /*basic*/
    self->percentage = 0.0;
    self->whole_lyrics = NULL;
    self->current_lyric_id = -1;
    /*privat data*/
    OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (self);
    priv->line_count = DEFAULT_LINE_COUNT;
    priv->active_color = DEFAULT_ACTIVE_COLOR;
    priv->inactive_color = DEFAULT_INACTIVE_COLOR;
    priv->bg_color = DEFAULT_BG_COLOR;
    priv->font_name = g_strdup (DEFAULT_FONT_NAME);
    priv->line_margin = DEFAULT_LINE_MARGIN;
    priv->padding_x = DEFAULT_PADDING_X;
    priv->padding_y = DEFAULT_PADDING_Y;
    priv->corner_radius = DEFAULT_CORNER_RADIUS;
    priv->bg_opacity = DEFAULT_BG_OPACITY;
    priv->frame_width = DEFAULT_FRAME_WIDTH;
    priv->text = NULL;
    priv->scroll_mode = OL_SCROLL_WINDOW_ALWAYS;
    priv->can_seek = FALSE;
    priv->seeking = FALSE;
    /*set allocation*/
    gtk_window_resize(GTK_WINDOW(self), DEFAULT_WIDTH, DEFAULT_HEIGHT);
    gtk_widget_add_events (GTK_WIDGET (self),
                           GDK_BUTTON_PRESS_MASK |
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_POINTER_MOTION_MASK |
                           GDK_POINTER_MOTION_HINT_MASK |
                           GDK_DESTROY);
    /* Set RGBA Colormap */
    GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (self));
    GdkColormap* colormap = gdk_screen_get_rgba_colormap (screen);
    if (colormap == NULL)
      colormap = gdk_screen_get_rgb_colormap (screen);
    gtk_widget_set_colormap (GTK_WIDGET (self), colormap);
    gtk_window_set_decorated (GTK_WINDOW(self), FALSE);
    gtk_widget_set_app_paintable (GTK_WIDGET (self), TRUE);
    /* We need an additional widget to paint on */
    priv->window_container = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
    gtk_widget_set_redraw_on_allocate(priv->window_container, TRUE);
    gtk_container_add (GTK_CONTAINER (self), priv->window_container);
    /* Set toolbar container */
    priv->toolbar_container = GTK_CONTAINER (gtk_alignment_new (1.0, 0.0, 0.0, 0.0));
    gtk_alignment_set_padding (GTK_ALIGNMENT (priv->toolbar_container),
                               priv->padding_x, priv->padding_x,
                               priv->padding_x, priv->padding_x);
    gtk_container_add (GTK_CONTAINER (priv->window_container),
                       GTK_WIDGET (priv->toolbar_container));
    gtk_widget_show_all (priv->window_container);
    /* Set tooltips */
    ol_scroll_window_update_tooltip (self);
    /* Connect signals */
    g_signal_connect (G_OBJECT (priv->window_container), "expose-event",
                      G_CALLBACK (ol_scroll_window_expose), self);
    g_signal_connect (G_OBJECT (self), "button-press-event",
                      G_CALLBACK (ol_scroll_window_button_press), self);
    g_signal_connect (G_OBJECT (self), "button-release-event",
                      G_CALLBACK (ol_scroll_window_button_release), self);
    g_signal_connect (G_OBJECT (self), "motion-notify-event",
                      G_CALLBACK (ol_scroll_window_motion_notify), self);
  }
}

static void
ol_scroll_window_dispose(GObject *object)
{
    OlScrollWindow *self = (OlScrollWindow *)object;
    OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (self);
    /* Check if not NULL! To avoid calling dispose multiple times */
    if(priv != NULL)
    {
        /* Deallocate contents of the private data, if any */
        /* Deallocate private data structure */
        g_free(priv);
        /* And finally set the opaque pointer back to NULL, so that
         *  we don't deallocate it twice. */
        (OL_SCROLL_WINDOW(self))->priv = NULL;
    }
}

static void
ol_scroll_window_class_init (OlScrollWindowClass *klass)
{
  GObjectClass *gobject_class;
  GtkObjectClass *gtkobject_class;
  gobject_class = G_OBJECT_CLASS (klass);
  gtkobject_class = GTK_OBJECT_CLASS (klass);
  gtkobject_class->destroy = ol_scroll_window_destroy;
  /* install signals */
  _signals[SEEK_SIGNAL] =
    g_signal_new ("seek",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,            /* class_offset */
                  NULL,         /* accumulator */
                  NULL,         /* accumulator data */
                  ol_marshal_VOID__UINT_DOUBLE,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_UINT,
                  G_TYPE_DOUBLE);
}

void
ol_scroll_window_add_toolbar (OlScrollWindow *scroll,
                              GtkWidget *widget)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  gtk_container_add (priv->toolbar_container, widget);
}

 void
ol_scroll_window_remove_toolbar (OlScrollWindow *scroll,
                                 GtkWidget *widget)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  gtk_container_remove (priv->toolbar_container, widget);
}

static void
ol_scroll_window_destroy (GtkObject *object)
{
  OlScrollWindow *scroll = OL_SCROLL_WINDOW (object);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (object);
  if (priv->font_name != NULL)
    g_free (priv->font_name);
  if (priv->text != NULL)
    g_free (priv->text);
  if (scroll->current_lyric_id!= -1)
  {
    scroll->current_lyric_id = -1;
  }
  GTK_OBJECT_CLASS (ol_scroll_window_parent_class)->destroy (object);
}


static gboolean
ol_scroll_window_expose (GtkWidget *widget,
                         GdkEventExpose *event,
                         gpointer userdata)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (userdata), FALSE);
  OlScrollWindow *scroll = OL_SCROLL_WINDOW (userdata);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  cairo_t *cr = _get_cairo (scroll, widget);
  _paint_bg (scroll, cr);
  if (scroll->whole_lyrics != NULL)
    _paint_lyrics (scroll, cr);
  else if (priv->text != NULL)
    _paint_text (scroll, cr);
  cairo_destroy (cr);
  return FALSE;
}

void
ol_scroll_window_set_whole_lyrics (OlScrollWindow *scroll,
                                   GPtrArray *whole_lyrics)
{
  ol_log_func ();
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  if (scroll->whole_lyrics != NULL)
    g_ptr_array_unref (scroll->whole_lyrics);
  scroll->whole_lyrics = whole_lyrics;
  if (whole_lyrics != NULL)
  {
    g_ptr_array_ref (whole_lyrics);
    priv->saved_lrc_y = -1;
  }
  else
  {
    /* We only queue draw when the lyrics are no available.
       Otherwise the progress will go wrong due to out-dated
       progress info*/
    gtk_widget_queue_draw (GTK_WIDGET (scroll));
  }
  ol_scroll_window_update_tooltip (scroll);
  if (priv->seeking)
    ol_scroll_window_end_seek (scroll);
}

static PangoLayout*
_get_pango (OlScrollWindow *scroll, cairo_t *cr)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll), NULL);
  ol_assert_ret (cr != NULL, NULL);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  PangoLayout *layout = pango_cairo_create_layout (cr);
  PangoFontDescription *desc = pango_font_description_from_string (priv->font_name);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);
  return layout;
}

static cairo_t*
_get_cairo (OlScrollWindow *scroll, GtkWidget *widget)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll), NULL);
  ol_assert_ret (GTK_IS_WIDGET (widget), NULL);
  cairo_t *cr;
  cr = gdk_cairo_create (gtk_widget_get_window (widget));
  cairo_save (cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_restore (cr);
  return cr;
}

static double
_get_active_color_ratio (OlScrollWindow *scroll, int line)
{
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  int line_height = ol_scroll_window_get_font_height (scroll) + priv->line_margin;
  gint current_lyric_id, lrc_y;
  _calc_paint_pos (scroll, &current_lyric_id, &lrc_y);
  double ratio = 0.0;
  gdouble percentage = (gdouble) lrc_y / (gdouble) line_height;
  if (line == current_lyric_id)
  {
    ratio = (1.0 - percentage) / 0.1;
    if (ratio > 1.0) ratio = 1.0;
    if (ratio < 0.0) ratio = 0.0;
    return ratio;
  }
  else if (line == current_lyric_id + 1)
  {
    ratio = (percentage - 0.9) / 0.1;
    if (ratio > 1.0) ratio = 1.0;
    if (ratio < 0.0) ratio = 0.0;
  }
  return ratio;
}

static void
_paint_bg (OlScrollWindow *scroll, cairo_t *cr)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  ol_assert (cr != NULL);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  gint width, height;
  gdk_drawable_get_size (gtk_widget_get_window (GTK_WIDGET (scroll)),
                         &width, &height);
  cairo_save (cr);
  cairo_new_path (cr);
  /* Top-left */
  cairo_arc (cr,
             priv->corner_radius, priv->corner_radius,
             priv->corner_radius,
             M_PI, M_PI * 1.5);
  cairo_line_to (cr, width - priv->corner_radius, 0);
  /* Top-right */
  cairo_arc (cr,
             width - priv->corner_radius, priv->corner_radius,
             priv->corner_radius,
             M_PI * 1.5, M_PI * 2);
  cairo_line_to (cr, width, height - priv->corner_radius);
  /* Bottom-right */
  cairo_arc (cr,
             width - priv->corner_radius, height - priv->corner_radius,
             priv->corner_radius,
             0, M_PI * 0.5);
  cairo_line_to (cr, priv->corner_radius, height);
  /* Bottom-left */
  cairo_arc (cr,
             priv->corner_radius, height - priv->corner_radius,
             priv->corner_radius,
             M_PI * 0.5, M_PI);
  cairo_close_path (cr);
  cairo_set_source_rgba (cr,
                         priv->bg_color.r,
                         priv->bg_color.g,
                         priv->bg_color.b,
                         priv->bg_opacity);
  cairo_fill (cr);
  /* cairo_stroke (cr); */
  cairo_restore (cr);
}

static gint
_calc_lrc_ypos (OlScrollWindow *scroll, double percentage)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll), -1);
  GtkWidget *widget = GTK_WIDGET (scroll);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  if (!gtk_widget_get_realized (widget))
    return -1;
  gint line_height;
  line_height = ol_scroll_window_get_font_height (scroll) + priv->line_margin;
  if (priv->scroll_mode == OL_SCROLL_WINDOW_BY_LINES)
  {
    if (percentage < 0.15)
      percentage = percentage / 0.15;
    else
      percentage = 1;
  }
  return line_height * percentage;
}

static void
_calc_paint_pos (OlScrollWindow *scroll,
                 gint *lyric_id,
                 gint *lrc_y)
{
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  if (priv->seeking)
  {
    gint line_height;
    line_height = ol_scroll_window_get_font_height (scroll) + priv->line_margin;
    gint id = priv->saved_lyric_id;
    gint y = priv->saved_seek_offset - priv->current_pointer_y + priv->saved_pointer_y;
    id += y / line_height;
    y %= line_height;
    if (y < 0)
    {
      y += line_height;
      id--;
    }
    if (id < 0)
    {
      id = 0;
      y = 0;
    }
    else if (id >= scroll->whole_lyrics->len)
    {
      id = scroll->whole_lyrics->len - 1;
      y = line_height;
    }
    if (lyric_id)
      *lyric_id = id;
    if (lrc_y)
      *lrc_y = y;
  }
  else
  {
    if (lyric_id)
      *lyric_id = scroll->current_lyric_id;
    priv->saved_lrc_y = _calc_lrc_ypos (scroll, scroll->percentage);
    if (lrc_y)
      *lrc_y = priv->saved_lrc_y;
  }
}

static void
_paint_lyrics (OlScrollWindow *scroll, cairo_t *cr)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  GtkWidget *widget = GTK_WIDGET (scroll);
  ol_assert (gtk_widget_get_realized (widget));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  int line_height = ol_scroll_window_get_font_height (scroll) + priv->line_margin;
  int count = ol_scroll_window_compute_line_count (scroll);
  gint width, height;
  gdk_drawable_get_size (gtk_widget_get_window (GTK_WIDGET (scroll)),
                         &width, &height);

  /* set the font */
  PangoLayout *layout = _get_pango (scroll, cr);
  pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
  pango_layout_set_width (layout, (width - priv->padding_x*2) * PANGO_SCALE);
  pango_layout_set_indent (layout, -20 * PANGO_SCALE);
  /* paint the lyrics*/
  cairo_save (cr);
  cairo_new_path (cr);
  cairo_rectangle (cr,
                   priv->padding_x, 0,
                   width - priv->padding_x * 2,
                   height - priv->padding_y * 2);
  cairo_close_path (cr);
  cairo_clip (cr);
  int i;
  gint current_lyric_id;
  gint lrc_y;
  _calc_paint_pos (scroll,
                   &current_lyric_id,
                   &lrc_y);
  int begin = current_lyric_id - count / 2;
  int end = current_lyric_id + count / 2 + 1;
  int ypos = height / 2  - lrc_y - (count / 2 + 1) * line_height;
  cairo_set_source_rgb(cr,
                       priv->inactive_color.r,
                       priv->inactive_color.g,
                       priv->inactive_color.b);
  if (scroll->whole_lyrics != NULL) {
    for (i = begin; i < end; i++) {
      ypos += line_height;
      if (i < 0) continue;
      if (i >= scroll->whole_lyrics->len)
        break;
      pango_layout_set_text (layout,
                             g_ptr_array_index (scroll->whole_lyrics, i),
                             -1);
      cairo_save (cr);
      double ratio = _get_active_color_ratio (scroll, i);
      double alpha = 1.0;
      if (ypos < line_height / 2.0 + priv->padding_y)
        alpha = 1.0 - (line_height / 2.0 + priv->padding_y - ypos) * 1.0 / line_height * 2;
      else if (ypos > height - line_height * 1.5 - priv->padding_y)
        alpha = (height - line_height - priv->padding_y - ypos) * 1.0 / line_height * 2;
      if (alpha < 0.0) alpha = 0.0;
      cairo_set_source_rgba (cr,
                             priv->active_color.r * ratio +
                             priv->inactive_color.r * (1 - ratio),
                             priv->active_color.g * ratio +
                             priv->inactive_color.g * (1 - ratio),
                             priv->active_color.b * ratio +
                             priv->inactive_color.b * (1 - ratio),
                             alpha);
      cairo_move_to (cr, priv->padding_x, ypos);
      pango_cairo_update_layout (cr, layout);
      pango_cairo_show_layout (cr, layout);
      cairo_restore (cr);

      if (pango_layout_is_wrapped (layout)) {
        // There is more than one line, offset ypos according to the number of
        // additional lines
        ypos += (pango_layout_get_line_count (layout) - 1)
                * ol_scroll_window_get_font_height (scroll);
      }
    }
  }
  g_object_unref (layout);
  cairo_reset_clip (cr);
  cairo_restore (cr);
}

static void
_paint_text (OlScrollWindow *scroll, cairo_t *cr)
{
  ol_log_func ();
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  ol_assert (cr != NULL);
  GtkWidget *widget = GTK_WIDGET (scroll);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  gint width, height;
  PangoRectangle extent;
  PangoLayout *layout;
  gint x, y;
  gdk_drawable_get_size (gtk_widget_get_window (widget),
                         &width, &height);

  /* set the font */
  cairo_save (cr);
  cairo_set_source_rgb (cr,
                        priv->inactive_color.r,
                        priv->inactive_color.g,
                        priv->inactive_color.b);
  layout = _get_pango (scroll, cr);
  pango_layout_set_text (layout, priv->text, -1);
  pango_layout_get_pixel_extents (layout, NULL, &extent);
  pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
  x = (width - extent.width) / 2;
  y = (height - extent.height) / 2;
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  cairo_move_to (cr, x, y);
  pango_cairo_update_layout (cr, layout);
  pango_cairo_show_layout (cr, layout);
}

void
ol_scroll_window_set_progress (OlScrollWindow *scroll,
                               int lyric_id,
                               double percentage)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  gint saved_lyric_id = scroll->current_lyric_id;
  scroll->current_lyric_id = lyric_id;
  scroll->percentage = percentage;
  if (saved_lyric_id != lyric_id ||
      priv->saved_lrc_y != _calc_lrc_ypos (scroll, percentage))
    gtk_widget_queue_draw (GTK_WIDGET (scroll));
}

void
ol_scroll_window_set_text (OlScrollWindow *scroll,
                           const char *text)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  if (priv->text != NULL)
    g_free (priv->text);
  priv->text = g_strdup (text);
  gtk_widget_queue_draw (GTK_WIDGET (scroll));
}

static int
ol_scroll_window_compute_line_count (OlScrollWindow *scroll)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll), 0);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  int font_height = ol_scroll_window_get_font_height (scroll) + priv->line_margin;
  gint width, height;
  gtk_window_get_size (GTK_WINDOW (scroll), &width, &height);
  int line_count = height - priv->padding_y * 2 / font_height;
  return line_count;
}

static int
ol_scroll_window_get_font_height (OlScrollWindow *scroll)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll), 0);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);

  PangoContext *pango_context = gdk_pango_context_get ();
  PangoLayout *pango_layout = pango_layout_new (pango_context);
  PangoFontDescription *font_desc = pango_font_description_from_string (priv->font_name);
  pango_layout_set_font_description (pango_layout, font_desc);

  PangoFontMetrics *metrics = pango_context_get_metrics (pango_context,
                                                         pango_layout_get_font_description (pango_layout), /* font desc */
                                                         NULL); /* languague */
  int height = 0;
  int ascent, descent;
  ascent = pango_font_metrics_get_ascent (metrics);
  descent = pango_font_metrics_get_descent (metrics);
  pango_font_metrics_unref (metrics);

  height += PANGO_PIXELS (ascent + descent);
  pango_font_description_free (font_desc);
  g_object_unref (pango_layout);
  g_object_unref (pango_context);
  return height;
}

int
ol_scroll_window_get_current_lyric_id (OlScrollWindow *scroll)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll), -1);
  return scroll->current_lyric_id;
}

void
ol_scroll_window_set_font_name (OlScrollWindow *scroll,
                                const char *font_name)
{
  if (scroll == NULL || font_name == NULL)
    return;
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  if (priv->font_name != NULL)
    g_free (priv->font_name);
  priv->font_name = g_strdup (font_name);
  gtk_widget_queue_draw (GTK_WIDGET (scroll));
}

const char*
ol_scroll_window_get_font_name (OlScrollWindow *scroll)
{
  if (scroll == NULL)
    return NULL;
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  return priv->font_name;
}

static gboolean
_get_pointer_edge (gint x, gint y,
                   gint width, gint height,
                   gint top, gint bottom,
                   gint left, gint right,
                   GdkWindowEdge *edge)
{
  ol_assert_ret (width > 0 && height > 0, FALSE);
  ol_assert_ret (top >= 0, FALSE);
  ol_assert_ret (bottom >= 0, FALSE);
  ol_assert_ret (left >= 0, FALSE);
  ol_assert_ret (right >= 0, FALSE);
  gboolean ret = TRUE;
  GdkWindowEdge ret_edge = GDK_WINDOW_EDGE_EAST;
  if (x < left)
  {
    if (y < top)
      ret_edge = GDK_WINDOW_EDGE_NORTH_WEST;
    else if (y >= height - bottom)
      ret_edge = GDK_WINDOW_EDGE_SOUTH_WEST;
    else
      ret_edge = GDK_WINDOW_EDGE_WEST;
  }
  else if (x >= width - right)
  {
    if (y < top)
      ret_edge = GDK_WINDOW_EDGE_NORTH_EAST;
    else if (y >= height - bottom)
      ret_edge = GDK_WINDOW_EDGE_SOUTH_EAST;
    else
      ret_edge = GDK_WINDOW_EDGE_EAST;
  }
  else if (y < top)
  {
    ret_edge = GDK_WINDOW_EDGE_NORTH;
  }
  else if (y >= height - bottom)
  {
    ret_edge = GDK_WINDOW_EDGE_SOUTH;
  }
  else
  {
    ret = FALSE;
  }
  if (edge != NULL && ret)
    *edge = ret_edge;
  return ret;
}

static void
ol_scroll_window_begin_move_resize (GtkWidget *widget,
                                    GdkEventButton *event)
{
  gint width, height;
  GtkWindow *window = GTK_WINDOW (widget);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (widget);
  GdkWindowEdge edge;
  gtk_window_get_size (window, &width, &height);
  if (!_get_pointer_edge (event->x, event->y,
                          width, height,
                          priv->frame_width, priv->frame_width,
                          priv->frame_width, priv->frame_width,
                          &edge))
    gtk_window_begin_move_drag (window,
                                event->button,
                                (gint)event->x_root,
                                (gint)event->y_root,
                                event->time);
  else
    gtk_window_begin_resize_drag (window,
                                  edge,
                                  event->button,
                                  (gint)event->x_root,
                                  (gint)event->y_root,
                                  event->time);
}

static void
ol_scroll_window_begin_seek (OlScrollWindow *scroll,
                             GdkEventButton *event)
{
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  priv->seeking = TRUE;
  priv->saved_seek_offset = priv->saved_lrc_y;
  priv->saved_pointer_y = event->y;
  priv->saved_lyric_id = scroll->current_lyric_id;
}

static gboolean
ol_scroll_window_button_press (GtkWidget *widget,
                               GdkEventButton *event)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (widget), FALSE);
  OlScrollWindow *scroll = OL_SCROLL_WINDOW (widget);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  if (event->button == 1) {
    if (event->state & GDK_CONTROL_MASK && scroll->whole_lyrics != NULL &&
        priv->can_seek)
    {
      ol_scroll_window_begin_seek (OL_SCROLL_WINDOW (widget), event);
    }
    else
    {
      ol_scroll_window_begin_move_resize (widget, event);
    }
  }
  return FALSE;
}

static void
ol_scroll_window_end_seek (OlScrollWindow *scroll)
{
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  priv->seeking = FALSE;
}

static gboolean
ol_scroll_window_button_release (GtkWidget *widget,
                                 GdkEventButton *event)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (widget), FALSE);
  OlScrollWindow *scroll = OL_SCROLL_WINDOW (widget);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (widget);
  if (priv->seeking)
  {
    gint current_lyric_id, lrc_y;
    gint line_height = ol_scroll_window_get_font_height (scroll) + priv->line_margin;
    _calc_paint_pos (scroll, &current_lyric_id, &lrc_y);
    gdouble percentage = (gdouble) lrc_y / (gdouble) line_height;
    ol_scroll_window_end_seek (scroll);
    g_signal_emit (scroll,
                   _signals[SEEK_SIGNAL],
                   0,           /* detail */
                   (guint)current_lyric_id,
                   percentage);
  }
  return FALSE;
}

static void
ol_scroll_window_update_cursor (GtkWidget *widget,
                                GdkEventMotion *event)
{
  GdkWindowEdge edge;
  gint width, height;
  gtk_window_get_size (GTK_WINDOW (widget), &width, &height);
  GdkCursor *cursor = NULL;
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (widget);
  if (_get_pointer_edge (event->x, event->y,
                         width, height,
                         priv->frame_width, priv->frame_width,
                         priv->frame_width, priv->frame_width,
                         &edge))
  {
    switch (edge) {
    case GDK_WINDOW_EDGE_EAST:
      cursor = gdk_cursor_new (GDK_RIGHT_SIDE);
      break;
    case GDK_WINDOW_EDGE_WEST:
      cursor = gdk_cursor_new (GDK_LEFT_SIDE);
      break;
    case GDK_WINDOW_EDGE_NORTH:
      cursor = gdk_cursor_new (GDK_TOP_SIDE);
      break;
    case GDK_WINDOW_EDGE_SOUTH:
      cursor = gdk_cursor_new (GDK_BOTTOM_SIDE);
      break;
    case GDK_WINDOW_EDGE_NORTH_EAST:
      cursor = gdk_cursor_new (GDK_TOP_RIGHT_CORNER);
      break;
    case GDK_WINDOW_EDGE_NORTH_WEST:
      cursor = gdk_cursor_new (GDK_TOP_LEFT_CORNER);
      break;
    case GDK_WINDOW_EDGE_SOUTH_EAST:
      cursor = gdk_cursor_new (GDK_BOTTOM_RIGHT_CORNER);
      break;
    case GDK_WINDOW_EDGE_SOUTH_WEST:
      cursor = gdk_cursor_new (GDK_BOTTOM_LEFT_CORNER);
      break;
    }
  }
  gdk_window_set_cursor (widget->window,
                         cursor);
  if (cursor)
    gdk_cursor_unref (cursor);
}

static void
ol_scroll_window_seek (OlScrollWindow *scroll,
                       GdkEventMotion *event)
{
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  priv->current_pointer_y = event->y;
  gtk_widget_queue_draw (GTK_WIDGET (scroll));
}

static gboolean
ol_scroll_window_motion_notify (GtkWidget *widget,
                                GdkEventMotion *event)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (widget), FALSE);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (widget);
  if (priv->seeking)
  {
    ol_scroll_window_seek (OL_SCROLL_WINDOW (widget),
                           event);
  }
  else if (event->state & GDK_CONTROL_MASK)
  {
    gdk_window_set_cursor (widget->window,
                           NULL);
  }
  else
  {
    ol_scroll_window_update_cursor (widget, event);
  }
  return FALSE;
}

void
ol_scroll_window_set_active_color (OlScrollWindow *scroll,
                                   OlColor color)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  priv->active_color = color;
  gtk_widget_queue_draw (GTK_WIDGET (scroll));
}

OlColor
ol_scroll_window_get_active_color (OlScrollWindow *scroll)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll),
                 ol_color_from_string ("#000000"));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  return priv->active_color;
}

void
ol_scroll_window_set_inactive_color (OlScrollWindow *scroll,
                                     OlColor color)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  priv->inactive_color = color;
  gtk_widget_queue_draw (GTK_WIDGET (scroll));
}

OlColor
ol_scroll_window_get_inactive_color (OlScrollWindow *scroll)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll),
                 ol_color_from_string ("#000000"));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  return priv->inactive_color;
}

void
ol_scroll_window_set_bg_color (OlScrollWindow *scroll,
                               OlColor color)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  priv->bg_color = color;
  gtk_widget_queue_draw (GTK_WIDGET (scroll));
}

OlColor
ol_scroll_window_get_bg_color (OlScrollWindow *scroll)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll),
                 ol_color_from_string ("#000000"));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  return priv->bg_color;
}

void
ol_scroll_window_set_bg_opacity (OlScrollWindow *scroll,
                                 double opacity)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  priv->bg_opacity = opacity;
  gtk_widget_queue_draw (GTK_WIDGET (scroll));
}

double
ol_scroll_window_get_bg_opacity (OlScrollWindow *scroll)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll),
                 1.0);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  return priv->bg_opacity;
}

void
ol_scroll_window_set_scroll_mode (OlScrollWindow *scroll,
                                  enum OlScrollWindowScrollMode mode)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  priv->scroll_mode = mode;
  gtk_widget_queue_draw (GTK_WIDGET (scroll));
}

enum OlScrollWindowScrollMode
ol_scroll_window_get_scroll_mode (OlScrollWindow *scroll)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll), OL_SCROLL_WINDOW_ALWAYS);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  return priv->scroll_mode;
}

static void
ol_scroll_window_update_tooltip (OlScrollWindow *scroll)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  if (priv->can_seek && scroll->whole_lyrics != NULL)
    gtk_widget_set_tooltip_text (GTK_WIDGET (scroll), _(TOOLTIP_WITH_SEEK));
  else
    gtk_widget_set_tooltip_text (GTK_WIDGET (scroll), _(TOOLTIP_WITHOUT_SEEK));
}

void
ol_scroll_window_set_can_seek (OlScrollWindow *scroll,
                               gboolean can_seek)
{
  ol_assert (OL_IS_SCROLL_WINDOW (scroll));
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  if (can_seek != priv->can_seek)
  {
    priv->can_seek = can_seek;
    ol_scroll_window_update_tooltip (scroll);
  }
}

gboolean
ol_scroll_window_get_can_seek (OlScrollWindow *scroll)
{
  ol_assert_ret (OL_IS_SCROLL_WINDOW (scroll), FALSE);
  OlScrollWindowPrivate *priv = OL_SCROLL_WINDOW_GET_PRIVATE (scroll);
  return priv->can_seek;
}
