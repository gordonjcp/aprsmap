#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "aprsis.h"
#include "mapviewer.h"

extern OsmGpsMap *map;
extern GtkEntry *latent;
extern GtkEntry *lonent;
extern GtkEntry *rangeent;
extern GtkWidget *about;
extern GtkWidget *popup;
extern GtkComboBox *server;

G_MODULE_EXPORT gboolean
on_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data);

G_MODULE_EXPORT gboolean
on_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data);

G_MODULE_EXPORT gboolean
on_zoom_in_clicked_event (GtkWidget *widget, gpointer user_data);

G_MODULE_EXPORT gboolean
on_zoom_out_clicked_event (GtkWidget *widget, gpointer user_data);

G_MODULE_EXPORT gboolean
on_home_clicked_event (GtkWidget *widget, gpointer user_data);

G_MODULE_EXPORT void
on_about_clicked_event (GtkWidget *widget, gpointer user_data);

G_MODULE_EXPORT gpointer
on_properties_clicked_event (GtkWidget *widget, gpointer user_data);

G_MODULE_EXPORT gboolean
on_properties_ok_clicked (GtkWidget *widget, aprs_details *properties);

G_MODULE_EXPORT gboolean
on_properties_hide_event (GtkWidget *widget, gpointer user_data);

G_MODULE_EXPORT void
on_close (GtkWidget *widget, gpointer user_data);

#endif


