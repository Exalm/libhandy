/*
 * Copyright (C) 2017 Purism SPC
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include <glib/gi18n.h>

#include "hdy-dialer.h"
#include "hdy-dialer-button.h"
#include "hdy-dialer-cycle-button.h"
#include "hdy-string-utf8.h"

/**
 * SECTION:hdy-dialer
 * @short_description: A keypad for dialing numbers
 * @Title: HdyDialer
 *
 * The #HdyDialer widget is a keypad for entering numbers such as phone numbers
 * or PIN codes.
 */

typedef struct
{
  HdyDialerButton *btn_0, *btn_1, *btn_2, *btn_3, *btn_4, *btn_5, *btn_6, *btn_7, *btn_8, *btn_9;
  HdyDialerCycleButton *btn_hash, *btn_star, *cycle_btn;
  GtkButton *btn_submit, *btn_del;
  GString *number;
} HdyDialerPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (HdyDialer, hdy_dialer, GTK_TYPE_GRID)

enum {
  PROP_0,
  PROP_NUMBER,
  PROP_LAST_PROP,
};
static GParamSpec *props[PROP_LAST_PROP];

enum {
  SIGNAL_SUBMITTED,
  SIGNAL_LAST_SIGNAL,
};
static guint signals [SIGNAL_LAST_SIGNAL];

static void
stop_cycle_mode (HdyDialer *self)
{
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);

  if (priv->cycle_btn) {
    hdy_dialer_cycle_button_stop_cycle (priv->cycle_btn);
    priv->cycle_btn = NULL;
  }
}

static void
digit_button_clicked (HdyDialer       *self,
                      HdyDialerButton *btn)
{
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);
  int d;

  g_return_if_fail (HDY_IS_DIALER (self));
  g_return_if_fail (HDY_IS_DIALER_BUTTON (btn));

  stop_cycle_mode (self);

  d = hdy_dialer_button_get_digit (btn);
  g_string_append_printf (priv->number, "%d", d);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NUMBER]);
}

static void
cycle_button_clicked (HdyDialer            *self,
                      HdyDialerCycleButton *btn)
{
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);
  gunichar symbol;

  g_return_if_fail (HDY_IS_DIALER (self));
  g_return_if_fail (HDY_IS_DIALER_BUTTON (btn));

  if (priv->cycle_btn != btn) {
    stop_cycle_mode (self);
    priv->cycle_btn = btn;
  } else if (priv->number->len && hdy_dialer_cycle_button_is_cycling (btn)) {
    hdy_string_utf8_truncate (priv->number, hdy_string_utf8_len (priv->number)-1);
  }

  symbol = hdy_dialer_cycle_button_get_current_symbol (btn);
  g_string_append_unichar (priv->number, symbol);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NUMBER]);
}

static void
cycle_start (HdyDialer            *self,
             HdyDialerCycleButton *btn)
{
  /* FIXME: change cursor */
}

static void
cycle_end (HdyDialer            *self,
           HdyDialerCycleButton *btn)
{
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);

  /* reset cycle_btn so pressing it again produces a new character */
  if (priv->cycle_btn == btn) {
    priv->cycle_btn = NULL;
    /* FIXME: change cursor */
  }
}

static void
submit_button_clicked (HdyDialer *self,
                     GtkButton *btn)
{
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);

  g_return_if_fail (HDY_IS_DIALER (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));

  stop_cycle_mode (self);

  g_signal_emit (self, signals[SIGNAL_SUBMITTED], 0, priv->number->str);
}

static void
del_button_clicked (HdyDialer *self,
                    GtkButton *btn)
{
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);

  g_return_if_fail (HDY_IS_DIALER (self));
  g_return_if_fail (GTK_IS_BUTTON (btn));

  stop_cycle_mode (self);

  if (!priv->number->len)
    return;

  hdy_string_utf8_truncate (priv->number, hdy_string_utf8_len (priv->number)-1);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NUMBER]);
}


static void
hdy_dialer_finalize (GObject *object)
{
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (HDY_DIALER (object));

  g_string_free (priv->number, TRUE);

  G_OBJECT_CLASS (hdy_dialer_parent_class)->finalize (object);
}

static void
hdy_dialer_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  HdyDialer *self = HDY_DIALER (object);
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);

  switch (property_id) {
  case PROP_NUMBER:
    g_string_overwrite (priv->number, 0, g_value_get_string (value));
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
hdy_dialer_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  HdyDialer *self = HDY_DIALER (object);
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);

  switch (property_id) {
  case PROP_NUMBER:
    g_value_set_string (value, priv->number->str);
    break;

  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
hdy_dialer_class_init (HdyDialerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = hdy_dialer_finalize;

  object_class->set_property = hdy_dialer_set_property;
  object_class->get_property = hdy_dialer_get_property;

  props[PROP_NUMBER] =
    g_param_spec_string ("number",
                         _("Number"),
                         _("The phone number to dial"),
                         "",
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, PROP_LAST_PROP, props);

  /**
   * HdyDialer::submitted:
   * @self: The #HdyDialer instance.
   * @number: The number at the time of activation.
   *
   * This signal is emitted when the dialer's 'dial' button is activated.
   * Connect to this signal to perform to get notified when the user
   * wants to submit the dialed number.
   */
  signals[SIGNAL_SUBMITTED] =
    g_signal_new ("submitted",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (HdyDialerClass, submitted),
                  NULL, NULL, NULL,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRING);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/sm/puri/handy/dialer/ui/hdy-dialer.ui");
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_0);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_1);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_2);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_3);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_4);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_5);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_6);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_7);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_8);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_9);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_hash);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_star);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_submit);
  gtk_widget_class_bind_template_child_private (widget_class, HdyDialer, btn_del);
}

/**
 * hdy_dialer_new:
 *
 * Create a new #HdyDialer widget.
 *
 * Returns: the newly created #HdyDialer widget
 *
 */
GtkWidget *hdy_dialer_new (void)
{
  return g_object_new (HDY_TYPE_DIALER, NULL);
}

static void
hdy_dialer_init (HdyDialer *self)
{
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);
  GtkWidget *image;

  gtk_widget_init_template (GTK_WIDGET (self));
  g_signal_connect_object (priv->btn_0,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->btn_1,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->btn_2,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->btn_3,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->btn_4,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->btn_5,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->btn_6,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->btn_7,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->btn_8,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->btn_9,
                           "clicked",
                           G_CALLBACK (digit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);

  g_object_connect (priv->btn_star,
                    "swapped-signal::clicked", G_CALLBACK (cycle_button_clicked), self,
                    "swapped-signal::cycle-start", G_CALLBACK (cycle_start), self,
                    "swapped-signal::cycle-end", G_CALLBACK (cycle_end), self,
                    NULL);

  g_object_connect (priv->btn_hash,
                    "swapped-signal::clicked", G_CALLBACK (cycle_button_clicked), self,
                    "swapped-signal::cycle-start", G_CALLBACK (cycle_start), self,
                    "swapped-signal::cycle-end", G_CALLBACK (cycle_end), self,
                    NULL);

  g_signal_connect_object (priv->btn_submit,
                           "clicked",
                           G_CALLBACK (submit_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (priv->btn_del,
                           "clicked",
                           G_CALLBACK (del_button_clicked),
                           self,
                           G_CONNECT_SWAPPED);

  /* In GTK+4 we can just use the icon-name property */
  image = gtk_image_new_from_icon_name ("edit-clear-symbolic",
                                        GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image (priv->btn_del, image);

  gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                    "/sm/puri/handy/icons");
  image = gtk_image_new_from_icon_name ("phone-dial-symbolic",
                                        GTK_ICON_SIZE_BUTTON * 1.3);
  gtk_button_set_image (priv->btn_submit, image);

  priv->number = g_string_new (NULL);
  priv->cycle_btn = NULL;
}

/**
 * hdy_dialer_get_number:
 * @self: a #HdyDialer
 *
 * Get the currently displayed number.
 *
 * Returns: (transfer none): the current number in the display
 */
const char*
hdy_dialer_get_number (HdyDialer *self)
{
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);
  return priv->number->str;
}

/**
 * hdy_dialer_set_number:
 * @self: a #HdyDialer
 * @newnumber: (transfer none): the number to set
 *
 * Set the currently displayed number.
 *
 */
void
hdy_dialer_set_number (HdyDialer  *self,
                       const char *newnumber)
{
  HdyDialerPrivate *priv = hdy_dialer_get_instance_private (self);

  g_string_overwrite (priv->number, 0, newnumber);
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_NUMBER]);
}