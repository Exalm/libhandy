/*
 * Copyright (C) 2019 Alexander Mikhaylenko <exalm7659@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "hdy-paginator.h"

#include "hdy-paginator-box-private.h"
#include "hdy-swipe-tracker-private.h"

#include <math.h>

#define LINE_WIDTH 3
#define LINE_LENGTH 35
#define LINE_SPACING 5
#define LINE_OPACITY 0.45
#define DEFAULT_DURATION 250

/**
 * SECTION:hdy-paginator
 * @short_description: A paginated scrolling widget.
 * @title: HdyPaginator
 *
 * The #HdyPaginator widget can be used to display a set of pages with
 * swipe-based navigation between them and optional indicators.
 *
 * Since: 0.0.11
 */

/**
 * HdyPaginatorIndicatorStyle
 * @HDY_PAGINATOR_INDICATOR_STYLE_NONE: No indicators
 * @HDY_PAGINATOR_INDICATOR_STYLE_LINES: Each page is denoted by a thin and long line, and active view is shown with another line that moves between them
 *
 * These enumeration values describe the possible page indicator styles in a
 * #HdyPaginator widget.
 *
 * New values may be added to this enumeration over time.
 */

struct _HdyPaginator
{
  GtkEventBox parent_instance;

  GtkBox *box;
  GtkBox *empty_box;
  HdyPaginatorBox *scrolling_box;
  GtkDrawingArea *indicators;

  HdySwipeTracker *tracker;

  HdyPaginatorIndicatorStyle indicator_style;
  gboolean center_content;
  GtkOrientation orientation;
  uint animation_duration;
};

G_DEFINE_TYPE_WITH_CODE (HdyPaginator, hdy_paginator, GTK_TYPE_EVENT_BOX,
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL));

enum {
  PROP_0,
  PROP_N_PAGES,
  PROP_POSITION,
  PROP_INTERACTIVE,
  PROP_INDICATOR_STYLE,
  PROP_CENTER_CONTENT,
  PROP_SPACING,
  PROP_ANIMATION_DURATION,

  /* GtkOrientable */
  PROP_ORIENTATION,
  LAST_PROP = PROP_ORIENTATION,
};

static GParamSpec *props[LAST_PROP];

static void
swipe_begin_cb (HdyPaginator    *self,
                double           x,
                double           y,
                HdySwipeTracker *tracker)
{
  double distance, position, closest_point;
  int i, n_pages;
  double *points;

  hdy_paginator_box_stop_animation (self->scrolling_box);

  distance = hdy_paginator_box_get_distance (self->scrolling_box);
  g_object_get (self->scrolling_box,
                "position", &position,
                "n-pages", &n_pages,
                NULL);
  closest_point = CLAMP (round (position), 0, n_pages - 1);

  points = g_new (gdouble, n_pages);
  for (i = 0; i < n_pages; i++)
    points[i] = i;

  hdy_swipe_tracker_confirm_swipe (tracker, distance, points, n_pages,
                                   position, closest_point);
}

static void
swipe_update_cb (HdyPaginator    *self,
                 double           value,
                 HdySwipeTracker *tracker)
{
  hdy_paginator_box_set_position (self->scrolling_box, value);
}

static void
swipe_end_cb (HdyPaginator    *self,
              gint64           duration,
              double           to,
              HdySwipeTracker *tracker)
{
  if (duration == 0) {
    hdy_paginator_box_set_position (self->scrolling_box, to);
    return;
  }

  hdy_paginator_box_animate (self->scrolling_box, to, duration);
}

static void
notify_n_pages_cb (HdyPaginator *self,
                   GParamSpec   *spec,
                   GObject      *object)
{
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_N_PAGES]);
  gtk_widget_queue_draw (GTK_WIDGET (self->indicators));
}

static void
notify_position_cb (HdyPaginator *self,
                    GParamSpec   *spec,
                    GObject      *object)
{
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_POSITION]);
  gtk_widget_queue_draw (GTK_WIDGET (self->indicators));
}

static void
notify_spacing_cb (HdyPaginator *self,
                   GParamSpec   *spec,
                   GObject      *object)
{
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SPACING]);
}

static gboolean
draw_indicators_cb (HdyPaginator *self,
                    cairo_t      *cr,
                    GtkWidget    *widget)
{
  int i, n_pages, widget_length, indicator_length;
  double position, length;
  GtkStyleContext *context;
  GtkStateFlags flags;
  GdkRGBA color;

  g_object_get (self->scrolling_box,
                "position", &position,
                "n-pages", &n_pages,
                NULL);

  if (n_pages < 2)
    return GDK_EVENT_PROPAGATE;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL &&
      gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
    position = n_pages - position - 1;

  context = gtk_widget_get_style_context (widget);
  flags = gtk_widget_get_state_flags (widget);
  gtk_style_context_get_color (context, flags, &color);

  length = (double) LINE_LENGTH / (LINE_LENGTH + LINE_SPACING);
  indicator_length = (LINE_LENGTH + LINE_SPACING) * n_pages - LINE_SPACING;

  if (self->orientation == GTK_ORIENTATION_HORIZONTAL) {
    widget_length = gtk_widget_get_allocated_width (widget);
    cairo_translate (cr, (widget_length - indicator_length) / 2, 0);
    cairo_scale (cr, LINE_LENGTH + LINE_SPACING, LINE_WIDTH);
  } else {
    widget_length = gtk_widget_get_allocated_height (widget);
    cairo_translate (cr, 0, (widget_length - indicator_length) / 2);
    cairo_scale (cr, LINE_WIDTH, LINE_LENGTH + LINE_SPACING);
  }

  cairo_set_source_rgba (cr, color.red, color.green, color.blue,
                         color.alpha * LINE_OPACITY);
  for (i = 0; i < n_pages; i++) {
    if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
      cairo_rectangle (cr, i, 0, length, 1);
    else
      cairo_rectangle (cr, 0, i, 1, length);
    cairo_fill (cr);
  }

  cairo_set_source_rgba (cr, color.red, color.green, color.blue, color.alpha);
  if (self->orientation == GTK_ORIENTATION_HORIZONTAL)
    cairo_rectangle (cr, position, 0, length, 1);
  else
    cairo_rectangle (cr, 0, position, 1, length);
  cairo_fill (cr);

  return GDK_EVENT_PROPAGATE;
}

static void
update_indicators_visibility (HdyPaginator *self)
{
  gboolean show_indicators = (self->indicator_style !=
                              HDY_PAGINATOR_INDICATOR_STYLE_NONE);
  g_object_set (self->indicators, "visible", show_indicators, NULL);
  g_object_set (self->empty_box, "visible",
                show_indicators && self->center_content, NULL);
}

static void
update_orientation (HdyPaginator *self)
{
  GtkOrientation opposite;
  gboolean reversed;

  if (!self->scrolling_box)
    return;

  opposite = (self->orientation == GTK_ORIENTATION_HORIZONTAL) ?
    GTK_ORIENTATION_VERTICAL :
    GTK_ORIENTATION_HORIZONTAL;
  reversed = self->orientation == GTK_ORIENTATION_HORIZONTAL &&
    gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;

  g_object_set (self->scrolling_box, "orientation", self->orientation, NULL);
  g_object_set (self->tracker, "orientation", self->orientation,
                "reversed", reversed, NULL);
  g_object_set (self->box, "orientation", opposite, NULL);

  gtk_widget_queue_draw (GTK_WIDGET (self->indicators));
}

static gboolean
captured_event_cb (HdyPaginator *self,
                   GdkEvent     *event)
{
  return hdy_swipe_tracker_captured_event (self->tracker, event);
}

static void
hdy_paginator_destroy (GtkWidget *widget)
{
  HdyPaginator *self = HDY_PAGINATOR (widget);

  if (self->box) {
    gtk_widget_destroy (GTK_WIDGET (self->box));
    self->box = NULL;
  }

  GTK_WIDGET_CLASS (hdy_paginator_parent_class)->destroy (widget);
}

static void
hdy_paginator_direction_changed (GtkWidget        *widget,
                                 GtkTextDirection  previous_direction)
{
  HdyPaginator *self = HDY_PAGINATOR (widget);

  update_orientation (self);
}

static void
hdy_paginator_add (GtkContainer *container,
                   GtkWidget    *widget)
{
  HdyPaginator *self = HDY_PAGINATOR (container);

  if (self->scrolling_box)
    gtk_container_add (GTK_CONTAINER (self->scrolling_box), widget);
  else
    GTK_CONTAINER_CLASS (hdy_paginator_parent_class)->add (container, widget);
}

static void
hdy_paginator_remove (GtkContainer *container,
                      GtkWidget    *widget)
{
  HdyPaginator *self = HDY_PAGINATOR (container);

  if (self->scrolling_box)
    gtk_container_remove (GTK_CONTAINER (self->scrolling_box), widget);
  else
    GTK_CONTAINER_CLASS (hdy_paginator_parent_class)->remove (container, widget);
}

static void
hdy_paginator_forall (GtkContainer *container,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
  HdyPaginator *self = HDY_PAGINATOR (container);

  if (include_internals)
    (* callback) (GTK_WIDGET (self->box), callback_data);
  else if (self->scrolling_box)
    gtk_container_foreach (GTK_CONTAINER (self->scrolling_box),
                           callback, callback_data);
}

static void
hdy_paginator_constructed (GObject *object)
{
  HdyPaginator *self = (HdyPaginator *)object;

  update_orientation (self);

  G_OBJECT_CLASS (hdy_paginator_parent_class)->constructed (object);
}

static void
hdy_paginator_finalize (GObject *object)
{
  HdyPaginator *self = (HdyPaginator *)object;

  g_signal_handlers_disconnect_by_data (self->scrolling_box, self);
  g_signal_handlers_disconnect_by_data (self->tracker, self);
  g_object_unref (self->tracker);

  g_object_set_data (object, "captured-event-handler", NULL);

  G_OBJECT_CLASS (hdy_paginator_parent_class)->finalize (object);
}

static void
hdy_paginator_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  HdyPaginator *self = HDY_PAGINATOR (object);

  switch (prop_id) {
  case PROP_N_PAGES:
    g_value_set_int (value, hdy_paginator_get_n_pages (self));
    break;

  case PROP_POSITION:
    g_value_set_double (value, hdy_paginator_get_position (self));
    break;

  case PROP_INTERACTIVE:
    g_value_set_boolean (value, hdy_paginator_get_interactive (self));
    break;

  case PROP_INDICATOR_STYLE:
    g_value_set_enum (value, hdy_paginator_get_indicator_style (self));
    break;

  case PROP_CENTER_CONTENT:
    g_value_set_boolean (value, hdy_paginator_get_center_content (self));
    break;

  case PROP_SPACING:
    g_value_set_int (value, hdy_paginator_get_spacing (self));
    break;

  case PROP_ORIENTATION:
    g_value_set_enum (value, self->orientation);
    break;

  case PROP_ANIMATION_DURATION:
    g_value_set_uint (value, hdy_paginator_get_animation_duration (self));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_paginator_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  HdyPaginator *self = HDY_PAGINATOR (object);

  switch (prop_id) {
  case PROP_INTERACTIVE:
    hdy_paginator_set_interactive (self, g_value_get_boolean (value));
    break;

  case PROP_INDICATOR_STYLE:
    hdy_paginator_set_indicator_style (self, g_value_get_enum (value));
    break;

  case PROP_CENTER_CONTENT:
    hdy_paginator_set_center_content (self, g_value_get_boolean (value));
    break;

  case PROP_SPACING:
    hdy_paginator_set_spacing (self, g_value_get_int (value));
    break;

  case PROP_ANIMATION_DURATION:
    hdy_paginator_set_animation_duration (self, g_value_get_uint (value));
    break;

  case PROP_ORIENTATION:
    {
      GtkOrientation orientation = g_value_get_enum (value);
      if (orientation != self->orientation) {
        self->orientation = orientation;
        update_orientation (self);
        g_object_notify (G_OBJECT (self), "orientation");
      }
    }
    g_object_notify (G_OBJECT (self), "orientation");
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
hdy_paginator_class_init (HdyPaginatorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->constructed = hdy_paginator_constructed;
  object_class->finalize = hdy_paginator_finalize;
  object_class->get_property = hdy_paginator_get_property;
  object_class->set_property = hdy_paginator_set_property;
  widget_class->destroy = hdy_paginator_destroy;
  widget_class->direction_changed = hdy_paginator_direction_changed;
  container_class->add = hdy_paginator_add;
  container_class->remove = hdy_paginator_remove;
  container_class->forall = hdy_paginator_forall;

  /**
   * HdyPaginator:n-pages:
   *
   * The number of pages in a #HdyPaginator
   *
   * Since: 0.0.11
   */
  props[PROP_N_PAGES] =
    g_param_spec_int ("n-pages",
                      _("Number of pages"),
                      _("Number of pages"),
                      0,
                      G_MAXINT,
                      0,
                      G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyPaginator:position:
   *
   * Current scrolling position, unitless. 1 matches 1 page. Use
   * hdy_paginator_scroll_to() for changing it.
   *
   * Since: 0.0.11
   */
  props[PROP_POSITION] =
    g_param_spec_double ("position",
                         _("Position"),
                         _("Current scrolling position"),
                         0,
                         G_MAXDOUBLE,
                         0,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyPaginator:interactive:
   *
   * Whether @self can be swiped. This can be used to temporarily disable
   * a #HdyPaginator to only allow swiping in a certain state.
   *
   * Since: 0.0.11
   */
  props[PROP_INTERACTIVE] =
    g_param_spec_boolean ("interactive",
                          _("Interactive"),
                          _("Whether the widget can be swiped"),
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyPaginator:indicator-style:
   *
   * The style of page indicators. Depending on orientation, they are displayed
   * below or besides the pages. If the pages are meant to be centered,
   * #HdyPaginator:center-content can be used to compensate for that.
   *
   * Since: 0.0.11
   */
  props[PROP_INDICATOR_STYLE] =
    g_param_spec_enum ("indicator-style",
                       _("Indicator style"),
                       _("Page indicator style"),
                       HDY_TYPE_PAGINATOR_INDICATOR_STYLE,
                       HDY_PAGINATOR_INDICATOR_STYLE_NONE,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyPaginator:center-content:
   *
   * Whether the #HdyPaginator is centering pages. If
   * #HdyPaginator:indicator-style is @HDY_PAGINATOR_INDICATOR_STYLE_NONE,
   * centering does nothing, otherwise it adds whitespace to the left or above
   * the pages to compensate for the indicators.
   *
   * Since: 0.0.11
   */
  props[PROP_CENTER_CONTENT] =
    g_param_spec_boolean ("center-content",
                          _("Center content"),
                          _("Whether to center pages to compensate for indicators"),
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * HdyPaginator:spacing:
   *
   * Spacing between pages in pixels.
   *
   * Since: 0.0.11
   */
  props[PROP_SPACING] =
    g_param_spec_int ("spacing",
                      _("Spacing"),
                      _("Spacing between pages"),
                      G_MININT,
                      G_MAXINT,
                      0,
                      G_PARAM_READWRITE);

  /**
   * HdyPaginator:animation-duration:
   *
   * Animation duration in milliseconds, used by hdy_paginator_scroll_to().
   *
   * Since: 0.0.11
   */
  props[PROP_ANIMATION_DURATION] =
    g_param_spec_uint ("animation-duration",
                       _("Animation duration"),
                       _("Default animation duration"),
                       0, G_MAXUINT, DEFAULT_DURATION,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_override_property (object_class,
                                    PROP_ORIENTATION,
                                    "orientation");

  g_object_class_install_properties (object_class, LAST_PROP, props);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/handy/ui/hdy-paginator.ui");
  gtk_widget_class_bind_template_child (widget_class, HdyPaginator, box);
  gtk_widget_class_bind_template_child (widget_class, HdyPaginator, empty_box);
  gtk_widget_class_bind_template_child (widget_class, HdyPaginator, scrolling_box);
  gtk_widget_class_bind_template_child (widget_class, HdyPaginator, indicators);
  gtk_widget_class_bind_template_callback (widget_class, draw_indicators_cb);

  gtk_widget_class_set_css_name (widget_class, "hdypaginator");
}

static void
hdy_paginator_init (HdyPaginator *self)
{
  g_type_ensure (HDY_TYPE_PAGINATOR_BOX);
  gtk_widget_init_template (GTK_WIDGET (self));

  self->animation_duration = DEFAULT_DURATION;

  self->tracker = hdy_swipe_tracker_new (GTK_WIDGET (self));
  g_signal_connect_swapped (self->tracker, "begin", G_CALLBACK (swipe_begin_cb), self);
  g_signal_connect_swapped (self->tracker, "update", G_CALLBACK (swipe_update_cb), self);
  g_signal_connect_swapped (self->tracker, "end", G_CALLBACK (swipe_end_cb), self);

  g_signal_connect_swapped (self->scrolling_box, "notify::n-pages", G_CALLBACK (notify_n_pages_cb), self);
  g_signal_connect_swapped (self->scrolling_box, "notify::position", G_CALLBACK (notify_position_cb), self);
  g_signal_connect_swapped (self->scrolling_box, "notify::spacing", G_CALLBACK (notify_spacing_cb), self);

  /*
   * HACK: GTK3 has no other way to get events on capture phase.
   * This is a reimplementation of _gtk_widget_set_captured_event_handler(),
   * which is private. In GTK4 it can be replaced with GtkEventControllerLegacy
   * with capture propagation phase
   */
  g_object_set_data (G_OBJECT (self), "captured-event-handler", captured_event_cb);
}

/**
 * hdy_paginator_new:
 *
 * Create a new #HdyPaginator widget.
 *
 * Returns: The newly created #HdyPaginator widget
 *
 * Since: 0.0.11
 */
HdyPaginator *
hdy_paginator_new (void)
{
  return g_object_new (HDY_TYPE_PAGINATOR, NULL);
}

/**
 * hdy_paginator_prepend:
 * @self: a #HdyPaginator
 * @child: a widget to add
 *
 * Prepends @child to @self
 *
 * Since: 0.0.11
 */
void
hdy_paginator_prepend (HdyPaginator *self,
                       GtkWidget    *widget)
{
  g_return_if_fail (HDY_IS_PAGINATOR (self));

  hdy_paginator_box_insert (self->scrolling_box, widget, 0);
}

/**
 * hdy_paginator_insert:
 * @self: a #HdyPaginator
 * @child: a widget to add
 * @position: the position to insert @child in.
 *
 * Inserts @child into @self at position @position.
 *
 * If position is -1, or larger than the number of pages,
 * @child will be appended to the end.
 *
 * Since: 0.0.11
 */
void
hdy_paginator_insert (HdyPaginator *self,
                      GtkWidget    *widget,
                      int           position)
{
  g_return_if_fail (HDY_IS_PAGINATOR (self));

  hdy_paginator_box_insert (self->scrolling_box, widget, position);
}
/**
 * hdy_paginator_reorder:
 * @self: a #HdyPaginator
 * @child: a widget to add
 * @position: the position to move @child to.
 *
 * Moves @child into position @position.
 *
 * If position is -1, or larger than the number of pages, @child will be moved
 * to the end.
 *
 * Since: 0.0.11
 */
void
hdy_paginator_reorder (HdyPaginator *self,
                       GtkWidget    *child,
                       int           position)
{
  g_return_if_fail (HDY_IS_PAGINATOR (self));
  g_return_if_fail (GTK_IS_WIDGET (child));

  hdy_paginator_box_reorder (self->scrolling_box, child, position);
}

/**
 * hdy_paginator_scroll_to:
 * @self: a #HdyPaginator
 * @widget: a child of @self
 *
 * Scrolls to @widget position with an animation.
 * #HdyPaginator:animation-duration property can be used for controlling the
 * duration.
 *
 * Since: 0.0.11
 */
void
hdy_paginator_scroll_to (HdyPaginator *self,
                         GtkWidget    *widget)
{
  g_return_if_fail (HDY_IS_PAGINATOR (self));

  hdy_paginator_box_scroll_to (self->scrolling_box, widget,
                               self->animation_duration);
}

/**
 * hdy_paginator_scroll_to_full:
 * @self: a #HdyPaginator
 * @widget: a child of @self
 * @duration: animation duration in milliseconds
 *
 * Scrolls to @widget position with an animation.
 *
 * Since: 0.0.11
 */
void
hdy_paginator_scroll_to_full (HdyPaginator *self,
                              GtkWidget    *widget,
                              int64_t       duration)
{
  g_return_if_fail (HDY_IS_PAGINATOR (self));

  hdy_paginator_box_scroll_to (self->scrolling_box, widget,
                               duration);
}

/**
 * hdy_paginator_get_n_pages:
 * @self: a #HdyPaginator
 *
 * Gets the number of pages in @self.
 *
 * Returns: The number of pages in @self
 *
 * Since: 0.0.11
 */
int
hdy_paginator_get_n_pages (HdyPaginator *self)
{
  g_return_val_if_fail (HDY_IS_PAGINATOR (self), 0);

  return hdy_paginator_box_get_n_pages (self->scrolling_box);
}

/**
 * hdy_paginator_get_position:
 * @self: a #HdyPaginator
 *
 * Gets current scroll position in @self. It's unitless, 1 matches 1 page.
 *
 * Returns: The scroll position
 *
 * Since: 0.0.11
 */
double
hdy_paginator_get_position (HdyPaginator *self)
{
  g_return_val_if_fail (HDY_IS_PAGINATOR (self), 0);

  return hdy_paginator_box_get_position (self->scrolling_box);
}

/**
 * hdy_paginator_get_interactive
 * @self: a #HdyPaginator
 *
 * Gets whether @self can be swiped.
 *
 * Returns: %TRUE if @self can be swiped
 *
 * Since: 0.0.11
 */
gboolean
hdy_paginator_get_interactive (HdyPaginator *self)
{
  g_return_val_if_fail (HDY_IS_PAGINATOR (self), FALSE);

  return hdy_swipe_tracker_get_enabled (self->tracker);
}

/**
 * hdy_paginator_set_interactive
 * @self: a #HdyPaginator
 * @interactive: whether @self can be swiped.
 *
 * Sets whether @self can be swiped. This can be used to temporarily disable
 * a #HdyPaginator to only allow swiping in a certain state.
 *
 * Since: 0.0.11
 */
void
hdy_paginator_set_interactive (HdyPaginator *self,
                               gboolean      interactive)
{
  g_return_if_fail (HDY_IS_PAGINATOR (self));

  hdy_swipe_tracker_set_enabled (self->tracker, interactive);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_INTERACTIVE]);
}

/**
 * hdy_paginator_get_indicator_style
 * @self: a #HdyPaginator
 *
 * Gets the current page indicator style.
 *
 * Returns: the current indicator style
 *
 * Since: 0.0.11
 */
HdyPaginatorIndicatorStyle
hdy_paginator_get_indicator_style (HdyPaginator *self)
{
  g_return_val_if_fail (HDY_IS_PAGINATOR (self), FALSE);

  return self->indicator_style;
}

/**
 * hdy_paginator_set_indicator_style
 * @self: a #HdyPaginator
 * @style: indicator style to use
 *
 * Sets style of page indicators. Depending on orientation, they are displayed
 * below or besides the pages. If the pages are meant to be centered,
 * #HdyPaginator:center-content can be used to compensate for that.
 *
 * Since: 0.0.11
 */
void
hdy_paginator_set_indicator_style (HdyPaginator               *self,
                                   HdyPaginatorIndicatorStyle  style)
{
  g_return_if_fail (HDY_IS_PAGINATOR (self));

  if (self->indicator_style == style)
    return;

  self->indicator_style = style;
  update_indicators_visibility (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_INDICATOR_STYLE]);
}

/**
 * hdy_paginator_get_center_content
 * @self: a #HdyPaginator
 *
 * Sets whether @self is centering pages.
 *
 * Returns: %TRUE if @self is centering pages
 *
 * Since: 0.0.11
 */
gboolean
hdy_paginator_get_center_content (HdyPaginator *self)
{
  g_return_val_if_fail (HDY_IS_PAGINATOR (self), FALSE);

  return self->center_content;
}

/**
 * hdy_paginator_set_center_content
 * @self: a #HdyPaginator
 * @center_content: whether @self should center contents
 *
 * Sets whether @self is centering content. If #HdyPaginator:indicator-style is
 * @HDY_PAGINATOR_INDICATOR_STYLE_NONE, centering does nothing, otherwise it
 * adds whitespace to the left or above the pages to compensate for the
 * indicators.
 *
 * Since: 0.0.11
 */
void
hdy_paginator_set_center_content (HdyPaginator *self,
                                  gboolean      center_content)
{
  g_return_if_fail (HDY_IS_PAGINATOR (self));

  if (self->center_content == center_content)
    return;

  self->center_content = center_content;
  update_indicators_visibility (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_CENTER_CONTENT]);
}

/**
 * hdy_paginator_get_spacing:
 * @self: a #HdyPaginator
 *
 * Gets spacing between pages in pixels.
 *
 * Returns: Spacing between pages
 *
 * Since: 0.0.11
 */
int
hdy_paginator_get_spacing (HdyPaginator *self)
{
  g_return_val_if_fail (HDY_IS_PAGINATOR (self), 0);

  return hdy_paginator_box_get_spacing (self->scrolling_box);
}

/**
 * hdy_paginator_set_spacing:
 * @self: a #HdyPaginator
 * @spacing: the new spacing value
 *
 * Sets spacing between pages in pixels.
 *
 * Since: 0.0.11
 */
void
hdy_paginator_set_spacing (HdyPaginator *self, int spacing)
{
  g_return_if_fail (HDY_IS_PAGINATOR (self));

  g_object_freeze_notify (G_OBJECT (self));
  hdy_paginator_box_set_spacing (self->scrolling_box, spacing);
  g_object_thaw_notify (G_OBJECT (self));

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SPACING]);
}

/**
 * hdy_paginator_get_animation_duration:
 * @self: a #HdyPaginator
 *
 * Gets animation duration used by hdy_paginator_scroll_to().
 *
 * Returns: Animation duration in milliseconds
 *
 * Since: 0.0.11
 */
uint
hdy_paginator_get_animation_duration (HdyPaginator *self)
{
  g_return_val_if_fail (HDY_IS_PAGINATOR (self), 0);

  return self->animation_duration;
}

/**
 * hdy_paginator_set_animation_duration:
 * @self: a #HdyPaginator
 * @duration: animation duration in milliseconds
 *
 * Sets animation duration used by hdy_paginator_scroll_to().
 *
 * Since: 0.0.11
 */
void
hdy_paginator_set_animation_duration (HdyPaginator *self,
                                      uint          duration)
{
  g_return_if_fail (HDY_IS_PAGINATOR (self));

  if (self->animation_duration == duration)
    return;

  self->animation_duration = duration;

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ANIMATION_DURATION]);
}
