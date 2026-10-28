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

static void ol_image_button_finalize (GObject *object);

G_DEFINE_TYPE (OlImageButton, ol_image_button, GTK_TYPE_BUTTON);

static void ol_image_button_destroy (GtkWidget *object);
static void ol_image_button_size_request (GtkWidget *widget,
                                          GtkRequisition *requisition);
static void ol_image_button_size_allocate (GtkWidget     *widget,
                                           GtkAllocation *allocation);
static gboolean ol_image_button_draw (GtkWidget *widget,
                                        cairo_t *cr);

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
  GTK_WIDGET_CLASS (ol_image_button_parent_class)->size_allocate (widget, allocation);
}

static gboolean
ol_image_button_draw (GtkWidget *widget,
                        cairo_t *cr)
{
  OlImageButton *btn = OL_IMAGE_BUTTON (widget);
  OlImageButtonPriv *priv = OL_IMAGE_BUTTON_GET_PRIVATE (btn);
  if (priv->image == NULL)
  {
    GTK_WIDGET_CLASS (ol_image_button_parent_class)->draw (widget, cr);
  }
  else
  {
    cairo_save (cr);
    int w, h, sw, sh, mx, my, x, y;
    w = gtk_widget_get_allocated_width (widget);
    h = gtk_widget_get_allocated_height (widget);
    mx = w / 2;
    my = h / 2;
    sw = gdk_pixbuf_get_width (priv->image) / SLICE_NUM;
    sh = gdk_pixbuf_get_height (priv->image);
    x = mx - sw / 2; y = my - sh / 2;
    cairo_rectangle (cr, 0, 0,
                     w, h);
    cairo_clip (cr);
    cairo_rectangle (cr, x, y,
                     sw, sh);
    cairo_clip (cr);

    int img_index = STATE_NORMAL;
    GtkStateFlags state = gtk_widget_get_state_flags (widget);
    if (state & GTK_STATE_FLAG_INSENSITIVE)
      img_index = STATE_DISABLED;
    else if (state & GTK_STATE_FLAG_ACTIVE)
      img_index = STATE_PRESSED;
    else if (state & (GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_SELECTED))
      img_index = STATE_ACTIVE;
    gdk_cairo_set_source_pixbuf (cr, priv->image,
                                 x - img_index * sw,
                                 y);
    cairo_paint (cr);
    cairo_restore (cr);
  }
  return FALSE;
}

static void
ol_image_button_get_preferred_width (GtkWidget *widget, gint *minimum, gint *natural)
{
  GtkRequisition size;
  ol_image_button_size_request (widget, &size);
  *minimum = *natural = size.width;
}

static void
ol_image_button_get_preferred_height (GtkWidget *widget, gint *minimum, gint *natural)
{
  GtkRequisition size;
  ol_image_button_size_request (widget, &size);
  *minimum = *natural = size.height;
}

static void
ol_image_button_class_init (OlImageButtonClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = ol_image_button_finalize;
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkWidgetClass *object_class = GTK_WIDGET_CLASS (klass);

  object_class->destroy = ol_image_button_destroy;

  widget_class->get_preferred_width = ol_image_button_get_preferred_width;
  widget_class->get_preferred_height = ol_image_button_get_preferred_height;
  widget_class->size_allocate = ol_image_button_size_allocate;
  widget_class->draw = ol_image_button_draw;
}

static void
ol_image_button_init (OlImageButton *btn)
{
  /* Sprite frames define the button size; theme padding must not shrink it. */
  GtkCssProvider *css = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css,
                                   "button { padding: 0; border-width: 0; min-width: 0; min-height: 0; }",
                                   -1, NULL);
  gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (btn)),
                                  GTK_STYLE_PROVIDER (css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (css);
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
ol_image_button_finalize(GObject *object)
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
  G_OBJECT_CLASS (ol_image_button_parent_class)->finalize (object);
}

static void
ol_image_button_destroy (GtkWidget *object)
{
  OlImageButton *btn = OL_IMAGE_BUTTON (object);
  OlImageButtonPriv *priv = OL_IMAGE_BUTTON_GET_PRIVATE (btn);
  if (priv->image != NULL)
  {
    g_object_unref (priv->image);
    priv->image = NULL;
  }
  GTK_WIDGET_CLASS (ol_image_button_parent_class)->destroy (object);
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
