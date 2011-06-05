#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <fap.h>
#include <osm-gps-map.h>

#include "callbacks.h"



G_MODULE_EXPORT gboolean
on_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{	
	if(event->type == GDK_2BUTTON_PRESS) {
	int zoom;
    	g_object_get(map, "zoom", &zoom, NULL);
    	osm_gps_map_set_zoom(map, zoom+1);
}
   	 return FALSE;
}
/* 
	Unconnected callbacks that will likely become useful in time. 
G_MODULE_EXPORT gboolean
on_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    float lat,lon;
    OsmGpsMap *map = OSM_GPS_MAP(widget);

    g_object_get(map, "latitude", &lat, "longitude", &lon, NULL);
    gchar *msg = g_strdup_printf("%f,%f",lat,lon);
    g_free(msg);

    return FALSE;
} */

G_MODULE_EXPORT gboolean
on_set_home_activate_event (GtkWidget *widget, aprs_details *properties)
{
	float lat,lon;
	
	g_object_get(map, "latitude", &lat, "longitude", &lon, NULL);
	
	properties->lat = lat;
	properties->lon = lon;
	
	//set filter around home area
	aprsis_set_filter(properties->ctx, properties->lat,properties->lon,properties->range);
	//update text boxes in properties
	gtk_entry_set_text(latent, g_strdup_printf("%f",properties->lat));
	gtk_entry_set_text(lonent, g_strdup_printf("%f",properties->lon));
	gtk_entry_set_text(rangeent, g_strdup_printf("%d",properties->range));
	return FALSE;  
}

G_MODULE_EXPORT gboolean
on_zoom_in_clicked_event (GtkWidget *widget, gpointer user_data)
{
    int zoom;
    g_object_get(map, "zoom", &zoom, NULL);
    osm_gps_map_set_zoom(map, zoom+1);
    return FALSE;
}

G_MODULE_EXPORT gboolean
on_zoom_out_clicked_event (GtkWidget *widget, gpointer user_data)
{
    int zoom;
    g_object_get(map, "zoom", &zoom, NULL);
    osm_gps_map_set_zoom(map, zoom-1);
    return FALSE;
}

G_MODULE_EXPORT gboolean
on_home_clicked_event (GtkWidget *widget, aprs_details *properties)
{
	//change this bitch up
    osm_gps_map_set_center_and_zoom(map,properties->lat,properties->lon,5);
    return FALSE;
}
G_MODULE_EXPORT void
on_about_clicked_event (GtkWidget *widget, gpointer user_data)
{
	gtk_dialog_run (GTK_DIALOG(about) );
	gtk_widget_hide (GTK_WIDGET(about));
}
G_MODULE_EXPORT gpointer
on_properties_clicked_event (GtkWidget *widget, aprs_details *properties)
{    
	gtk_window_present( GTK_WINDOW( popup ) );
	return FALSE;
}
G_MODULE_EXPORT gboolean
on_properties_ok_clicked (GtkWidget *widget, aprs_details *properties)
{
	double oldlat = properties->lat;
	double oldlon = properties->lon;   
	printf("We were: %f%f\n",oldlat,oldlon);
	properties->lat=g_ascii_strtod (gtk_entry_get_text(GTK_ENTRY(latent)),NULL);
	properties->lon=g_ascii_strtod (gtk_entry_get_text(GTK_ENTRY(lonent)), NULL);
	properties->range=g_ascii_strtod (gtk_entry_get_text(GTK_ENTRY(rangeent)), NULL); 
	printf("We are: %f%f\n",properties->lat,properties->lon);  
	//Check Latitude/Longitude entries are correct
	if(properties->lat > 89.9 || properties->lat < -89.9) {
	//printf("Invalid Lat\n");
	double homelat = oldlat; 
	//printf("New Lat:%f\n", homelat);
	gtk_entry_set_text(latent, g_strdup_printf("%f",properties->lat));
	}
	if(properties->lon > 180 || properties->lon < -180) {
	//printf("Invalid Lon\n");
	double homelon = oldlon; 
	//printf("New Lon:%f\n", homelon);
	gtk_entry_set_text(lonent, g_strdup_printf("%f",properties->lon));
	}
	//centre map on new coordinates after widget closed
	osm_gps_map_set_center_and_zoom(map,properties->lat, properties->lon, 5);
	aprsis_set_filter(properties->ctx, properties->lat,properties->lon,properties->range);
	gtk_widget_hide(	GTK_WIDGET( popup ) );
	return FALSE;
}
G_MODULE_EXPORT gboolean
on_properties_hide_event (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_hide(	GTK_WIDGET( popup ) );
	return FALSE;
}

G_MODULE_EXPORT void
on_close (GtkWidget *widget, gpointer user_data)
{
    gtk_widget_destroy(widget);
    gtk_main_quit();
}
