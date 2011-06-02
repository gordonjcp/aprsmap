/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/* vim:set et sw=4 ts=4 cino=t0,(0: */
/*
 * main.c
 * Copyright (C) John Stowers 2008 <john.stowers@gmail.com>
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <fap.h>
#include <osm-gps-map.h>

#include "aprsis.h"
#include "station.h"

OsmGpsMap *map;
GtkEntry *latent;
GtkEntry *lonent;
GtkEntry *rangeent;
GtkWidget *popup;
GtkComboBox *server;

GdkPixbuf *g_star_image = NULL;
cairo_surface_t *g_symbol_image = NULL;
cairo_surface_t *g_symbol_image2 = NULL;
OsmGpsMapImage *g_last_image = NULL;

GHashTable *stations;

//aprs details structure - enables passing of variables between the properties pop up and the main program

typedef struct _aprs_details{

double lat;
double lon;
int range;
aprsis_ctx *ctx;

} aprs_details;

aprs_details *aprs_details_new(double lat,double lon,int range,aprsis_ctx *ctx);

//function that lets us define the values in the aprs_details
aprs_details *aprs_details_new(double lat,double lon,int range,aprsis_ctx *ctx)	{

aprs_details *details = calloc(1, sizeof(aprs_details));

	details->lat = lat;
	details->lon = lon;
	details->range = range;
	details->ctx = ctx;

return details;
}

static OsmGpsMapSource_t opt_map_provider = OSM_GPS_MAP_SOURCE_OPENSTREETMAP;
static gboolean opt_friendly_cache = FALSE;
static gboolean opt_no_cache = FALSE;
static gboolean opt_debug = FALSE;
static char *opt_cache_base_dir = NULL;
static GOptionEntry entries[] =
{
  { "friendly-cache", 'f', 0, G_OPTION_ARG_NONE, &opt_friendly_cache, "Store maps using friendly cache style (source name)", NULL },
  { "no-cache", 'n', 0, G_OPTION_ARG_NONE, &opt_no_cache, "Disable cache", NULL },
  { "cache-basedir", 'b', 0, G_OPTION_ARG_FILENAME, &opt_cache_base_dir, "Cache basedir", NULL },
  { "debug", 'd', 0, G_OPTION_ARG_NONE, &opt_debug, "Enable debugging", NULL },
  { "map", 'm', 0, G_OPTION_ARG_INT, &opt_map_provider, "Map source", "N" },
  { NULL }
};



static gboolean
on_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    OsmGpsMapPoint coord;
    float lat, lon;
    OsmGpsMap *map = OSM_GPS_MAP(widget);
    return FALSE;
}

static gboolean
on_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    float lat,lon;
    OsmGpsMap *map = OSM_GPS_MAP(widget);

    g_object_get(map, "latitude", &lat, "longitude", &lon, NULL);
    gchar *msg = g_strdup_printf("%f,%f",lat,lon);
    g_free(msg);

    return FALSE;
}

static gboolean
on_zoom_in_clicked_event (GtkWidget *widget, gpointer user_data)
{
    int zoom;
    OsmGpsMap *map = OSM_GPS_MAP(user_data);
    g_object_get(map, "zoom", &zoom, NULL);
    osm_gps_map_set_zoom(map, zoom+1);
    return FALSE;
}

static gboolean
on_zoom_out_clicked_event (GtkWidget *widget, gpointer user_data)
{
    int zoom;
    OsmGpsMap *map = OSM_GPS_MAP(user_data);
    g_object_get(map, "zoom", &zoom, NULL);
    osm_gps_map_set_zoom(map, zoom-1);
    return FALSE;
}

static gboolean
on_home_clicked_event (GtkWidget *widget, gpointer user_data)
{
    OsmGpsMap *map = OSM_GPS_MAP(user_data);
	//change this bitch up
    osm_gps_map_set_center_and_zoom(map,55.00,-4.0,5);
    return FALSE;
}
static gboolean
on_properties_clicked_event (GtkWidget *widget, gpointer user_data)
{    
	gtk_window_present( GTK_WINDOW( popup ) );
	return FALSE;
}
static gboolean
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
static gboolean
on_properties_hide_event (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_hide(	GTK_WIDGET( popup ) );
	return FALSE;
}

static void
on_close (GtkWidget *widget, gpointer user_data)
{
    gtk_widget_destroy(widget);
    gtk_main_quit();
}


static void
usage (GOptionContext *context)
{
    int i;

    puts(g_option_context_get_help(context, TRUE, NULL));

    printf("Valid map sources:\n");
    for(i=OSM_GPS_MAP_SOURCE_NULL; i <= OSM_GPS_MAP_SOURCE_LAST; i++)
    {
        const char *name = osm_gps_map_source_get_friendly_name(i);
        const char *uri = osm_gps_map_source_get_repo_uri(i);
        if (uri != NULL)
            printf("\t%d:\t%s\n",i,name);
    }
}

int
main (int argc, char **argv)
{
    GtkBuilder *builder;
    GtkWidget *widget;
    GtkAccelGroup *ag;

    OsmGpsMapLayer *osd;
    const char *repo_uri;
    char *cachedir, *cachebasedir;
    GError *error = NULL;
    GOptionContext *context;
	GIOChannel *gio_read;

	//aprsis_ctx *ctx = aprsis_new("euro.aprs2.net", "14580", "aprsmap", "-1");
	aprsis_ctx *ctx = aprsis_new("localhost", "14580", "aprsmap", "-1");
	
	//set variables properties->lat, properties->lon, properties->range, properties->ctx
	aprs_details *properties = aprs_details_new(55.00,-4.00,600,ctx); 

	//aprsis_set_filter(properties->ctx,55.00,-4.50,300);   

    g_thread_init(NULL);
    gtk_init (&argc, &argv);

    // initialise APRS parser
    fap_init();

	// connect to APRS_IS server
	start_aprsis(ctx);

    context = g_option_context_new ("- Map browser");
    g_option_context_set_help_enabled(context, FALSE);
    g_option_context_add_main_entries (context, entries, NULL);

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        usage(context);
        return 1;
    }

    /* Only use the repo_uri to check if the user has supplied a
    valid map source ID */
    repo_uri = osm_gps_map_source_get_repo_uri(opt_map_provider);
    if ( repo_uri == NULL ) {
        usage(context);
        return 2;
    }

    cachebasedir = osm_gps_map_get_default_cache_directory();

    if (opt_cache_base_dir && g_file_test(opt_cache_base_dir, G_FILE_TEST_IS_DIR)) {
        cachedir = g_strdup(OSM_GPS_MAP_CACHE_AUTO);
        cachebasedir = g_strdup(opt_cache_base_dir);
    } else if (opt_friendly_cache) {
        cachedir = g_strdup(OSM_GPS_MAP_CACHE_FRIENDLY);
    } else if (opt_no_cache) {
        cachedir = g_strdup(OSM_GPS_MAP_CACHE_DISABLED);
    } else {
        cachedir = g_strdup(OSM_GPS_MAP_CACHE_AUTO);
    }

    if (opt_debug)
        gdk_window_set_debug_updates(TRUE);


    g_debug("Map Cache Dir: %s", cachedir);
    g_debug("Map Provider: %s (%d)", osm_gps_map_source_get_friendly_name(opt_map_provider), opt_map_provider);

    map = g_object_new (OSM_TYPE_GPS_MAP,
                        "map-source",opt_map_provider,
                        "tile-cache",cachedir,
                        "tile-cache-base", cachebasedir,
                        "proxy-uri",g_getenv("http_proxy"),
                        NULL);

    osd = g_object_new (OSM_TYPE_GPS_MAP_OSD,
                        "show-scale",TRUE,
                        "show-coordinates",TRUE,
                        NULL);
    osm_gps_map_layer_add(OSM_GPS_MAP(map), osd);
    g_object_unref(G_OBJECT(osd));

    g_free(cachedir);
    g_free(cachebasedir);

    //Enable keyboard   navigation
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_FULLSCREEN, GDK_F11);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_UP, GDK_Up);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_DOWN, GDK_Down);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_LEFT, GDK_Left);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_RIGHT, GDK_Right);

    //Build the UI
    g_symbol_image = cairo_image_surface_create_from_png("allicons.png"); //, &error);
    g_symbol_image2 = cairo_image_surface_create_from_png("allicon2.png"); //, &error);
    	
	stations = g_hash_table_new(g_str_hash, g_str_equal);


    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "mapviewer.ui", &error);
    if (error)
        g_error ("ERROR: %s\n", error->message);

    gtk_box_pack_start (
                GTK_BOX(gtk_builder_get_object(builder, "map_box")),
                GTK_WIDGET(map), TRUE, TRUE, 0);
  
    // centre on UK, because I'm UK-centric
    osm_gps_map_set_center_and_zoom(map, 55.00,-4.00, 5);

    //Connect to signals
    g_signal_connect (
                gtk_builder_get_object(builder, "zoom_in_button"), "clicked",
                G_CALLBACK (on_zoom_in_clicked_event), (gpointer) map);
    g_signal_connect (
                gtk_builder_get_object(builder, "zoom_out_button"), "clicked",
                G_CALLBACK (on_zoom_out_clicked_event), (gpointer) map);
    g_signal_connect (
                gtk_builder_get_object(builder, "home_button"), "clicked",
                G_CALLBACK (on_home_clicked_event), (gpointer) map);
	
	//Show Properties Windows
	g_signal_connect (
				gtk_builder_get_object(builder, "settings_button"), "clicked",
				G_CALLBACK (on_properties_clicked_event), (gpointer) map);
	//Hide Properties Window
	g_signal_connect (
				gtk_builder_get_object(builder, "closePrefs"), "clicked",
				G_CALLBACK (on_properties_hide_event), (gpointer) map);
	g_signal_connect (
				gtk_builder_get_object(builder, "okPrefs"), "clicked",
				G_CALLBACK (on_properties_ok_clicked), properties);
    g_signal_connect (G_OBJECT (map), "button-release-event",
                G_CALLBACK (on_button_release_event),
                (gpointer) gtk_builder_get_object(builder, "text_entry"));
   /* g_signal_connect (G_OBJECT (map), "notify::tiles-queued",
                G_CALLBACK (on_tiles_queued_changed),
                (gpointer) gtk_builder_get_object(builder, "cache_label")); */

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
    g_signal_connect (widget, "destroy",
                      G_CALLBACK (on_close), (gpointer) map);

		//pulls popup data from mapviewer.ui

	popup = GTK_WIDGET(gtk_builder_get_object(builder, "proppop"));

	//connect mapviewer.ui values to popup window
	gtk_builder_connect_signals(builder, popup);

    //Setup accelerators.
    ag = gtk_accel_group_new();
    gtk_accel_group_connect(ag, GDK_w, GDK_CONTROL_MASK, GTK_ACCEL_MASK,
                    g_cclosure_new(gtk_main_quit, NULL, NULL));
    gtk_accel_group_connect(ag, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_MASK,
                    g_cclosure_new(gtk_main_quit, NULL, NULL));
    gtk_window_add_accel_group(GTK_WINDOW(widget), ag);

	//Set up GTK_ENTRY boxes in the preferences pop up
	latent = GTK_ENTRY(gtk_builder_get_object(builder, "declat"));
	lonent = GTK_ENTRY(gtk_builder_get_object(builder, "declon"));
	rangeent = GTK_ENTRY(gtk_builder_get_object(builder, "range"));
	gtk_entry_set_text(latent, g_strdup_printf("%f",properties->lat));
	gtk_entry_set_text(lonent, g_strdup_printf("%f",properties->lon));
	gtk_entry_set_text(rangeent, g_strdup_printf("%d",properties->range));

	g_object_unref( G_OBJECT( builder ) );

    gtk_widget_show_all (widget);
	
    //g_log_set_handler ("OsmGpsMap", G_LOG_LEVEL_MASK, g_log_default_handler, NULL);
    g_log_set_handler ("OsmGpsMap", G_LOG_LEVEL_MESSAGE, g_log_default_handler, NULL);
    gtk_main ();


    fap_cleanup();
    aprsis_close(ctx);
    return(0);
}

/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
