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

#include "ol_cell_renderer_button.h"
#include "ol_marshal.h"
#include "ol_debug.h"

static void ol_cell_renderer_button_finalize   (GObject *object);

enum {
  EDITED,
  CLICKED,
  LAST_SIGNAL
};

enum {
  PROP_0,

  PROP_TEXT,
  PROP_STOCK,
  /* PROP_MARKUP, */
  /* PROP_ATTRIBUTES, */
  /* PROP_SINGLE_PARAGRAPH_MODE, */
  /* PROP_WIDTH_CHARS, */
  /* PROP_WRAP_WIDTH, */
  /* PROP_ALIGN, */

  /* /\* Style args *\/ */
  /* PROP_BACKGROUND, */
  /* PROP_FOREGROUND, */
  /* PROP_BACKGROUND_GDK, */
  /* PROP_FOREGROUND_GDK, */
  /* PROP_FONT, */
  /* PROP_FONT_DESC, */
  /* PROP_FAMILY, */
  /* PROP_STYLE, */
  /* PROP_VARIANT, */
  /* PROP_WEIGHT, */
  /* PROP_STRETCH, */
  /* PROP_SIZE, */
  /* PROP_SIZE_POINTS, */
  /* PROP_SCALE, */
  /* PROP_EDITABLE, */
  /* PROP_STRIKETHROUGH, */
  /* PROP_UNDERLINE, */
  /* PROP_RISE, */
  /* PROP_LANGUAGE, */
  /* PROP_ELLIPSIZE, */
  /* PROP_WRAP_MODE, */

  /* /\* Whether-a-style-arg-is-set args *\/ */
  /* PROP_BACKGROUND_SET, */
  /* PROP_FOREGROUND_SET, */
  /* PROP_FAMILY_SET, */
  /* PROP_STYLE_SET, */
  /* PROP_VARIANT_SET, */
  /* PROP_WEIGHT_SET, */
  /* PROP_STRETCH_SET, */
  /* PROP_SIZE_SET, */
  /* PROP_SCALE_SET, */
  /* PROP_EDITABLE_SET, */
  /* PROP_STRIKETHROUGH_SET, */
  /* PROP_UNDERLINE_SET, */
  /* PROP_RISE_SET, */
  /* PROP_LANGUAGE_SET, */
  /* PROP_ELLIPSIZE_SET, */
  /* PROP_ALIGN_SET */
};

static guint cell_renderer_button_signals [LAST_SIGNAL];

typedef struct _OlCellRendererButtonPrivate OlCellRendererButtonPrivate;
struct _OlCellRendererButtonPrivate
{
  gulong focus_out_id;
  PangoLanguage *language;
  PangoEllipsizeMode ellipsize;
  PangoWrapMode wrap_mode;
  PangoAlignment align;

  gulong populate_popup_id;
  gulong entry_menu_popdown_timeout;
  gboolean in_entry_menu;

  gint width_chars;
  gint wrap_width;

  GtkWidget *entry;
};

G_DEFINE_TYPE_WITH_PRIVATE (OlCellRendererButton,
                            ol_cell_renderer_button,
                            GTK_TYPE_CELL_RENDERER)

static void ol_cell_renderer_button_get_property (GObject *object,
                                           guint param_id,
                                           GValue *value,
                                           GParamSpec *pspec);
static void ol_cell_renderer_button_set_property (GObject *object,
                                           guint param_id,
                                           const GValue *value,
                                           GParamSpec *pspec);
static void ol_cell_renderer_button_get_size (GtkCellRenderer *cell,
                                       GtkWidget *widget,
                                       GdkRectangle *cell_area,
                                       gint *x_offset,
                                       gint *y_offset,
                                       gint *width,
                                       gint *height);
static void ol_cell_renderer_button_render (GtkCellRenderer *cell,
                                     GdkWindow *window,
                                     GtkWidget *widget,
                                     GdkRectangle *background_area,
                                     GdkRectangle *cell_area,
                                     GdkRectangle *expose_area,
                                     GtkCellRendererState flags);

static gint ol_cell_renderer_button_activate (GtkCellRenderer *cell,
                                       GdkEvent *event,
                                       GtkWidget *widget,
                                       const gchar *path,
                                       GdkRectangle *background_area,
                                       GdkRectangle *cell_area,
                                       GtkCellRendererState flags);

static PangoLayout* get_layout (OlCellRendererButton *cellbutton,
                                GtkWidget *widget,
                                gboolean will_render,
                                GtkCellRendererState flags);
static void get_size (GtkCellRenderer *cell,
                      GtkWidget *widget,
                      GdkRectangle *cell_area,
                      PangoLayout *layout,
                      gint *x_offset,
                      gint *y_offset,
                      gint *width,
                      gint *height);

static void
ol_cell_renderer_button_init (OlCellRendererButton *cellbutton)
{
  OlCellRendererButtonPrivate *priv;

  priv = ol_cell_renderer_button_get_instance_private (cellbutton);

  GTK_CELL_RENDERER (cellbutton)->xalign = 0.0;
  GTK_CELL_RENDERER (cellbutton)->yalign = 0.5;
  GTK_CELL_RENDERER (cellbutton)->xpad = 2;
  GTK_CELL_RENDERER (cellbutton)->ypad = 2;
  GTK_CELL_RENDERER (cellbutton)->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
  /* cellbutton->font_scale = 1.0; */
  /* cellbutton->fixed_height_rows = -1; */
  cellbutton->font = pango_font_description_new ();
  cellbutton->stock_id = NULL;

  priv->width_chars = -1;
  priv->wrap_width = -1;
  priv->wrap_mode = PANGO_WRAP_CHAR;
  priv->align = PANGO_ALIGN_LEFT;
  /* priv->align_set = FALSE; */
}

static void
ol_cell_renderer_button_class_init (OlCellRendererButtonClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkCellRendererClass *cell_class = GTK_CELL_RENDERER_CLASS (class);

  object_class->finalize = ol_cell_renderer_button_finalize;

  object_class->get_property = ol_cell_renderer_button_get_property;
  object_class->set_property = ol_cell_renderer_button_set_property;

  cell_class->get_size = ol_cell_renderer_button_get_size;
  cell_class->render = ol_cell_renderer_button_render;
  cell_class->activate = ol_cell_renderer_button_activate;

  g_object_class_install_property (object_class,
                                   PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        ("Text"),
                                                        ("Text to render"),
                                                        NULL,
                                                        GTK_PARAM_READWRITE));
  g_object_class_install_property (object_class,
                                   PROP_STOCK,
                                   g_param_spec_string ("stock",
                                                        ("stock"),
                                                        ("Stock name of icon"),
                                                        NULL,
                                                        GTK_PARAM_READWRITE));


  cell_renderer_button_signals [EDITED] =
    g_signal_new (("edited"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (OlCellRendererButtonClass, edited),
                  NULL, NULL,
                  ol_marshal_VOID__STRING_STRING,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  G_TYPE_STRING);
  cell_renderer_button_signals [CLICKED] =
    g_signal_new (("clicked"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (OlCellRendererButtonClass, clicked),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}

static void
ol_cell_renderer_button_finalize (GObject *object)
{
  OlCellRendererButton *cellbutton = OL_CELL_RENDERER_BUTTON (object);

  pango_font_description_free (cellbutton->font);

  g_free (cellbutton->text);

  G_OBJECT_CLASS (ol_cell_renderer_button_parent_class)->finalize (object);
}


static void
ol_cell_renderer_button_get_property (GObject        *object,
                               guint           param_id,
                               GValue         *value,
                               GParamSpec     *pspec)
{
  OlCellRendererButton *cellbutton = OL_CELL_RENDERER_BUTTON (object);

  switch (param_id)
  {
  case PROP_TEXT:
    g_value_set_string (value, cellbutton->text);
    break;
  case PROP_STOCK:
    g_value_set_string (value, cellbutton->stock_id);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    break;
  }
}

static void
ol_cell_renderer_button_set_property (GObject *object,
                               guint param_id,
                               const GValue *value,
                               GParamSpec *pspec)
{
  OlCellRendererButton *cellbutton = OL_CELL_RENDERER_BUTTON (object);

  ol_logf (OL_DEBUG, "%s:%d\n", __FUNCTION__, param_id);

  switch (param_id)
  {
  case PROP_TEXT:
    ol_logf (OL_DEBUG, "  text\n");
    g_free (cellbutton->text);
    cellbutton->text = g_strdup (g_value_get_string (value));
    g_object_notify (object, "text");
    break;
  case PROP_STOCK:
    ol_logf (OL_DEBUG, "  stock\n");
    g_free (cellbutton->text);
    cellbutton->stock_id = g_strdup (g_value_get_string (value));
    g_object_notify (object, "stock");
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    break;
  }
}

GtkCellRenderer *
ol_cell_renderer_button_new (void)
{
  return g_object_new (OL_TYPE_CELL_RENDERER_BUTTON, NULL);
}

static GdkPixbuf*
get_icon (OlCellRendererButton *cellbutton,
          GtkWidget *widget)
{
  GdkPixbuf *icon = NULL;
  ol_logf (OL_DEBUG, "%s\n", __FUNCTION__);
  if (cellbutton->stock_id != NULL)
  {
    ol_logf (OL_DEBUG, "  stock id = %s\n", cellbutton->stock_id);
    icon = gtk_widget_render_icon (widget,
                                   cellbutton->stock_id,
                                   GTK_ICON_SIZE_SMALL_TOOLBAR,
                                   NULL);
  }
  return icon;
}

static void
get_icon_rect (OlCellRendererButton *cellbtn,
               GtkWidget *widget,
               GdkRectangle *cell_area,
               GdkPixbuf *icon,
               int index,
               int *x_offset, int *y_offset, int *width, int *height)
{
  ol_logf (OL_DEBUG, "%s\n", __FUNCTION__);
  if (icon == NULL)
    icon = get_icon (cellbtn, widget);
  else
    g_object_ref (icon);
  gint icon_width, icon_height;
  if (icon == NULL)
  {
    icon_width = 0;
    icon_height = 0;
  }
  else
  {
    icon_width = gdk_pixbuf_get_width (icon);
    icon_height = gdk_pixbuf_get_height (icon);
    if (cell_area)
    {
      if (x_offset)
        *x_offset = cell_area->width - icon_width * (index + 1);
      if (y_offset)
        *y_offset = (cell_area->height - icon_height) / 2;
    }
    g_object_unref (icon);
  }
  if (width)
    *width = icon_width;
  if (height)
    *height = icon_height;
  ol_logf (OL_DEBUG, "  icon size:%dx%d\n", icon_width, icon_height);
}

static void
ol_cell_renderer_button_render (GtkCellRenderer      *cell,
                         GdkDrawable          *window,
                         GtkWidget            *widget,
                         GdkRectangle         *background_area,
                         GdkRectangle         *cell_area,
                         GdkRectangle         *expose_area,
                         GtkCellRendererState  flags)

{
  ol_logf (OL_DEBUG,
           "%s\n"
           "  flags:%d\n",
           __FUNCTION__,
           (int) flags);
  OlCellRendererButton *cellbutton = (OlCellRendererButton *) cell;
  PangoLayout *layout;
  GtkStateType state = 0;
  gint x_offset;
  gint y_offset;

  layout = get_layout (cellbutton, widget, TRUE, flags);
  get_size (cell, widget, cell_area, layout, &x_offset, &y_offset, NULL, NULL);
  /* ol_logf (OL_DEBUG, "  offset: (%d,%d)\n", x_offset, y_offset); */
  if (!cell->sensitive)
  {
    state |= GTK_STATE_INSENSITIVE;
  }
  else if ((flags & GTK_CELL_RENDERER_SELECTED) == GTK_CELL_RENDERER_SELECTED)
  {
    if (gtk_widget_has_focus (widget))
      state |= GTK_STATE_SELECTED;
    else
      state |= GTK_STATE_ACTIVE;
  }
  else if ((flags & GTK_CELL_RENDERER_PRELIT) == GTK_CELL_RENDERER_PRELIT)
  {
    state = GTK_STATE_PRELIGHT;
  }
  else
  {
    if (gtk_widget_get_state (widget) == GTK_STATE_INSENSITIVE)
      state = GTK_STATE_INSENSITIVE;
    else
      state = GTK_STATE_NORMAL;
  }

  pango_layout_set_width (layout,
                          (cell_area->width - x_offset - 2 * cell->xpad) * PANGO_SCALE);

  gtk_paint_layout (widget->style,
                    window,
                    state,
                    TRUE,
                    expose_area,
                    widget,
                    "cellrenderertext",
                    cell_area->x + x_offset + cell->xpad,
                    cell_area->y + y_offset + cell->ypad,
                    layout);
  g_object_unref (layout);
  {
    GdkPixbuf *icon = get_icon (cellbutton, widget);
    if (icon != NULL)
    {
      gint icon_width, icon_height, icon_x, icon_y;
      get_icon_rect (OL_CELL_RENDERER_BUTTON (cell),
                     widget,
                     cell_area,
                     icon,
                     0,
                     &icon_x, &icon_y, &icon_width, &icon_height);
      gdk_draw_pixbuf (window,
                       widget->style->black_gc,
                       icon,
                       0,           /* src_x */
                       0,           /* src_y */
                       cell_area->x + icon_x - cell->xpad,
                       cell_area->y + icon_y,
                       icon_width,
                       icon_height,
                       GDK_RGB_DITHER_NORMAL,
                       0, 0);

      g_object_unref (icon);
    } /* if icon */
  } /* if state */
}

static void
add_attr (PangoAttrList  *attr_list,
          PangoAttribute *attr)
{
  attr->start_index = 0;
  attr->end_index = G_MAXINT;

  pango_attr_list_insert (attr_list, attr);
}

static PangoLayout*
get_layout (OlCellRendererButton *cellbutton,
            GtkWidget           *widget,
            gboolean             will_render,
            GtkCellRendererState flags)
{
  ol_logf (OL_DEBUG, "%s\n", __FUNCTION__);
  PangoAttrList *attr_list;
  PangoLayout *layout;
  OlCellRendererButtonPrivate *priv;

  priv = ol_cell_renderer_button_get_instance_private (cellbutton);

  layout = gtk_widget_create_pango_layout (widget, cellbutton->text);
  attr_list = pango_attr_list_new ();
  pango_layout_set_single_paragraph_mode (layout, TRUE);

  if (will_render)
  {
    add_attr (attr_list, pango_attr_font_desc_new (cellbutton->font));
  }

  pango_layout_set_ellipsize (layout, priv->ellipsize);

  if (priv->wrap_width != -1)
  {
    pango_layout_set_width (layout, priv->wrap_width * PANGO_SCALE);
    /* ol_logf (OL_DEBUG, "  width: %d\n", (int)(priv->wrap_width * PANGO_SCALE)); */
    pango_layout_set_wrap (layout, priv->wrap_mode);
  }
  else
  {
    pango_layout_set_width (layout, -1);
    pango_layout_set_wrap (layout, PANGO_WRAP_CHAR);
  }

  PangoAlignment align;

  if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    align = PANGO_ALIGN_RIGHT;
  else
    align = PANGO_ALIGN_LEFT;

  pango_layout_set_alignment (layout, align);

  pango_layout_set_attributes (layout, attr_list);

  pango_attr_list_unref (attr_list);

  return layout;
}

static void
get_size (GtkCellRenderer *cell,
	  GtkWidget       *widget,
	  GdkRectangle    *cell_area,
	  PangoLayout     *layout,
	  gint            *x_offset,
	  gint            *y_offset,
	  gint            *width,
	  gint            *height)
{
  ol_logf (OL_DEBUG, "%s\n", __FUNCTION__);
  OlCellRendererButton *cellbutton = (OlCellRendererButton *) cell;

  gint btn_width = (cell->xpad) * 2;
  gint btn_height = (cell->ypad) * 2;

  GdkPixbuf *icon = get_icon (cellbutton, widget);
  if (icon != NULL)
  {
    gint icon_width, icon_height, icon_x, icon_y;
    get_icon_rect (OL_CELL_RENDERER_BUTTON (cell),
                   widget,
                   cell_area,
                   icon,
                   0,
                   &icon_x, &icon_y, &icon_width, &icon_height);

    g_object_unref (icon);
    btn_width += icon_width;
    btn_height += icon_height;
  }
  if (btn_height < cell->height)
    btn_height = cell->height;
  gtk_cell_renderer_set_fixed_size (cell,
                                    btn_width,
                                    btn_height);
  if (height)
    *height = btn_height;
  if (width)
    *width = btn_width;
  if (cell_area)
  {
    if (x_offset)
      *x_offset = cell_area->x;
    if (y_offset)
      *y_offset = cell_area->y;
    ol_logf (OL_DEBUG, "  offset: %dx%d\n", cell_area->x, cell_area->y);
  }
  ol_logf (OL_DEBUG, "  size: %dx%d\n", btn_width, btn_height);
}


static void
ol_cell_renderer_button_get_size (GtkCellRenderer *cell,
                           GtkWidget       *widget,
                           GdkRectangle    *cell_area,
                           gint            *x_offset,
                           gint            *y_offset,
                           gint            *width,
                           gint            *height)
{
  get_size (cell, widget, cell_area, NULL,
	    x_offset, y_offset, width, height);
}

static gint
ol_cell_renderer_button_activate (GtkCellRenderer      *cell,
                           GdkEvent             *event,
                           GtkWidget            *widget,
                           const gchar          *path,
                           GdkRectangle         *background_area,
                           GdkRectangle         *cell_area,
                           GtkCellRendererState  flags)
{
  ol_logf (OL_DEBUG,
           "%s\n"
           "  event type:%d\n",
           __FUNCTION__,
           event->type);
  ol_logf (OL_DEBUG,
           "  bg area: (%d,%d) %dx%d\n"
           "  cell area: (%d,%d) %dx%d\n",
           background_area->x, background_area->y,
           background_area->width, background_area->height,
           cell_area->x, cell_area->y,
           cell_area->width, cell_area->height);
  if (event->type == GDK_BUTTON_PRESS)
  {
    GdkEventButton button = event->button;
    ol_logf (OL_DEBUG,
             "  pointer (%d,%d)\n",
             (int)button.x, (int)button.y);
    if (button.x >= cell_area->x && button.x < cell_area->x + cell_area->width &&
        button.y >= cell_area->y && button.y < cell_area->y + cell_area->height)
    {
      ol_logf (OL_DEBUG, "  clicked\n");
      g_signal_emit (cell, cell_renderer_button_signals[CLICKED], 0, path);
    }
  }
  return FALSE;
}
