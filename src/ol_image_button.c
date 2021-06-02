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
#include "ol_image_button.h"
#include "ol_debug.h"

#define DEFAULT_SIZE 16
enum ImageIndex {
  STATE_NORMAL = 0,
  STATE_ACTIVE,
  STATE_PRESSED,
  STATE_DISABLED,
  SLICE_NUM,
};
#define OL_IMAGE_BUTTON_GET_PRIVATE(obj) \
    ((OlImageButtonPriv *)((OL_IMAGE_BUTTON(obj))->priv))

typedef struct _OlImageButtonPriv OlImageButtonPriv;
struct _OlImageButtonPriv
{
  GdkPixbuf *image;
};

G_DEFINE_TYPE_WITH_PRIVATE (OlImageButton, ol_image_button, GTK_TYPE_BUTTON);

static void ol_image_button_destroy (GtkObject *object);
static void ol_image_button_size_request (GtkWidget *widget,
                                          GtkRequisition *requisition);
static void ol_image_button_size_allocate (GtkWidget     *widget,
                                           GtkAllocation *allocation);
static gboolean ol_image_button_expose (GtkWidget *widget,
                                        GdkEventExpose *event);

static void
ol_image_button_size_request (GtkWidget *widget,
                              GtkRequisition *requisition)
{
  ol_log_func ();
  OlImageButton *btn = OL_IMAGE_BUTTON (widget);
  OlImageButtonPriv *priv = OL_IMAGE_BUTTON_GET_PRIVATE (btn);
  if (priv->image == NULL)
  {
    requisition->width = DEFAULT_SIZE;
    requisition->height = DEFAULT_SIZE;
  }
  else
  {
    requisition->width = gdk_pixbuf_get_width (priv->image) / SLICE_NUM;
    requisition->height = gdk_pixbuf_get_height (priv->image);
  }
  ol_debugf ("request: %d x %d\n", requisition->width, requisition->height);
}

static void
ol_image_button_size_allocate (GtkWidget     *widget,
                               GtkAllocation *allocation)
{
  ol_log_func ();
  ol_debugf ("allocation: (%d, %d) %d x %d\n",
             allocation->x, allocation->y,
             allocation->width, allocation->height);
  GtkButton *button = GTK_BUTTON (widget);
  widget->allocation = *allocation;

  if (gtk_widget_get_realized (widget))
    gdk_window_move_resize (button->event_window,
			    widget->allocation.x,
			    widget->allocation.y,
			    widget->allocation.width,
			    widget->allocation.height);
}

static gboolean
ol_image_button_expose (GtkWidget *widget,
                        GdkEventExpose *event)
{
  OlImageButton *btn = OL_IMAGE_BUTTON (widget);
  OlImageButtonPriv *priv = OL_IMAGE_BUTTON_GET_PRIVATE (btn);
  if (priv->image == NULL)
  {
    GTK_WIDGET_CLASS (ol_image_button_parent_class)->expose_event (widget, event);
  }
  else
  {
    cairo_t *cr = gdk_cairo_create (event->window);
    int w, h, sw, sh, mx, my, x, y;
    w = event->area.width; h = event->area.height;
    mx = event->area.x + event->area.width / 2;
    my = event->area.y + event->area.height / 2;
    sw = gdk_pixbuf_get_width (priv->image) / SLICE_NUM;
    sh = gdk_pixbuf_get_height (priv->image);
    x = mx - sw / 2; y = my - sh / 2;
    /* ol_debugf ("cut: (%d, %d) %d x %d\n", */
    /*            event->area.x, event->area.y, */
    /*            w, h); */
    cairo_rectangle (cr, event->area.x, event->area.y,
                     w, h);
    cairo_clip (cr);
    /* ol_debugf ("cut: (%d, %d) %d x %d\n", */
    /*            x, y, */
    /*            sw, sh); */
    cairo_rectangle (cr, x, y,
                     sw, sh);
    cairo_clip (cr);

    int img_index = STATE_NORMAL;
    GtkStateType state = gtk_widget_get_state (widget);
    if (state == GTK_STATE_ACTIVE)
      img_index = STATE_PRESSED;
    else if (state == GTK_STATE_PRELIGHT || state == GTK_STATE_SELECTED)
      img_index = STATE_ACTIVE;
    else if (state == GTK_STATE_INSENSITIVE)
      img_index = STATE_DISABLED;
    gdk_cairo_set_source_pixbuf (cr, priv->image,
                                 x - img_index * sw,
                                 y);
    cairo_paint (cr);
    cairo_destroy (cr);
  }
  return FALSE;
}

static void
ol_image_button_class_init (OlImageButtonClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkObjectClass *object_class = GTK_OBJECT_CLASS (klass);

  object_class->destroy = ol_image_button_destroy;

  widget_class->size_request = ol_image_button_size_request;
  widget_class->size_allocate = ol_image_button_size_allocate;
  widget_class->expose_event = ol_image_button_expose;
}

static void
ol_image_button_init (OlImageButton *btn)
{
  /* Allocate Private data structure */
  (OL_IMAGE_BUTTON(btn))->priv = \
      (OlImageButtonPriv *) g_malloc0(sizeof(OlImageButtonPriv));
  /* If correctly allocated, initialize parameters */
  if((OL_IMAGE_BUTTON(btn))->priv != NULL)
  {
    OlImageButtonPriv *priv = OL_IMAGE_BUTTON_GET_PRIVATE (btn);
    priv->image = NULL;
  }
}

static void
ol_image_button_dispose(GObject *object)
{
    OlImageButton *self = (OlImageButton *)object;
    OlImageButtonPriv *priv = OL_IMAGE_BUTTON_GET_PRIVATE(self);
    /* Check if not NULL! To avoid calling dispose multiple times */
    if(priv != NULL)
    {
        /* Deallocate contents of the private data, if any */
        /* Deallocate private data structure */
        g_free(priv);
        /* And finally set the opaque pointer back to NULL, so that
         *  we don't deallocate it twice. */
        (OL_IMAGE_BUTTON(self))->priv = NULL;
    }
}

static void
ol_image_button_destroy (GtkObject *object)
{
  OlImageButton *btn = OL_IMAGE_BUTTON (object);
  OlImageButtonPriv *priv = OL_IMAGE_BUTTON_GET_PRIVATE (btn);
  if (priv->image != NULL)
  {
    g_object_unref (priv->image);
    priv->image = NULL;
  }
  GTK_OBJECT_CLASS (ol_image_button_parent_class)->destroy (object);
}

GtkWidget *
ol_image_button_new (void)
{
  OlImageButton *button;
  button = g_object_new (ol_image_button_get_type (), NULL);
  return GTK_WIDGET (button);
}

void
ol_image_button_set_pixbuf (OlImageButton *btn, GdkPixbuf *image)
{
  ol_log_func ();
  ol_assert (OL_IS_IMAGE_BUTTON (btn));
  ol_assert (image == NULL || GDK_IS_PIXBUF (image));
  OlImageButtonPriv *priv = OL_IMAGE_BUTTON_GET_PRIVATE (btn);
  if (priv->image != NULL)
  {
    g_object_unref (priv->image);
  }
  priv->image = image;
  gtk_widget_queue_resize (GTK_WIDGET (btn));
  gtk_widget_queue_draw (GTK_WIDGET (btn));
}
