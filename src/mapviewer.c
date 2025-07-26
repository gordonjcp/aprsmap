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
#include <sqlite3.h>

#include "aprsis.h"
#include "station.h"
#include "mapviewer.h"
#include "callbacks.h"

OsmGpsMap *map;
GtkEntry *latent;
GtkEntry *lonent;
GtkEntry *rangeent;
GtkWidget *about;
GtkWidget *popup;
GtkComboBox *server;
sqlite3 *db;
int rc;

GdkPixbuf *g_star_image = NULL;
cairo_surface_t *g_symbol_image = NULL;
cairo_surface_t *g_symbol_image2 = NULL;
OsmGpsMapImage *g_last_image = NULL;
GtkStatusbar *statusbar = NULL;

GHashTable *stations;

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
static char *packet_log_file = NULL;
static char *aprsis_server = NULL;
static char *aprsis_port = NULL;

static guint st_ctx;

static GOptionEntry entries[] =
{
	{ "friendly-cache", 'f', 0, G_OPTION_ARG_NONE, &opt_friendly_cache, "Store maps using friendly cache style (source name)", NULL },
	{ "no-cache", 'n', 0, G_OPTION_ARG_NONE, &opt_no_cache, "Disable cache", NULL },
	{ "cache-basedir", 'b', 0, G_OPTION_ARG_FILENAME, &opt_cache_base_dir, "Cache basedir", NULL },
	{ "debug", 'd', 0, G_OPTION_ARG_NONE, &opt_debug, "Enable debugging", NULL },
	{ "map", 'm', 0, G_OPTION_ARG_INT, &opt_map_provider, "Map source", "N" },
	{ "aprsis-server", 's', 0, G_OPTION_ARG_STRING, &aprsis_server, "APRS-IS server", "HOST" },
	{ "aprsis-port", 'p', 0, G_OPTION_ARG_STRING, &aprsis_port, "APRS-IS port number", "PORT" },
	{ "packet-log-file", 'l', 0, G_OPTION_ARG_FILENAME, &packet_log_file, "Log network IO to a file", "FILE" },
  { NULL }
};



static gboolean *aprsmap_clear_status() {
	// remove message from the statusbar when the timer runs out
	gtk_statusbar_pop(statusbar, st_ctx);
	return FALSE;
}


void aprsmap_set_status(gchar *msg) {
	// put new message on the status bar
	gtk_statusbar_pop(statusbar, st_ctx);
	gtk_statusbar_push(statusbar, st_ctx, msg);
	g_timeout_add_seconds(3, (GSourceFunc)aprsmap_clear_status, NULL);
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
	char *zErrMsg = 0;
	//create and/or open sqlite3db
	rc = sqlite3_open("aprs.db", &db);
	if( rc ){
      fprintf(stderr, "Can't open database: %s\n",sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
    }
	//create and/or open table
	rc = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS call_data (call TEXT, object TEXT, course NUMERIC, lat NUMERIC, lon NUMERIC, time NUMERIC)", callback, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
    
    context = g_option_context_new ("- Map browser");
    g_option_context_set_help_enabled(context, FALSE);
    g_option_context_add_main_entries (context, entries, NULL);

    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        usage(context);
        return 1;
    }
	
	if (aprsis_server == NULL) {
		aprsis_server = strdup("euro.aprs2.net");
	}

	if (aprsis_port == 0) {
		aprsis_port = strdup("14580");
	}

	aprsis_ctx *ctx = aprsis_new(aprsis_server, aprsis_port, "aprsmap", "-1");
	//aprsis_ctx *ctx = aprsis_new("localhost", "14580", "aprsmap", "-1");

	//set variables properties->lat, properties->lon, properties->range, properties->ctx
	aprs_details *properties = aprs_details_new(55.00,-4.00,600,ctx); 
	
	if (packet_log_file != NULL) {
		FILE *log = fopen(packet_log_file, "w");
		aprsis_set_log(ctx, log);
	}

	aprsis_set_filter(properties->ctx, properties->lat,properties->lon,properties->range);
    gtk_init (&argc, &argv);

    // initialise APRS parser
    fap_init();

	// connect to APRS_IS server
	start_aprsis(ctx);

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
  
    // centre on the latitude and longitude set in the properties menu 
    osm_gps_map_set_center_and_zoom(map, properties->lat,properties->lon, 5);

    //Connect to signals that need data passed to them
    
    g_signal_connect (
                gtk_builder_get_object(builder, "home_button"), "clicked",
                G_CALLBACK (on_home_clicked_event), properties);
    g_signal_connect (
                gtk_builder_get_object(builder, "homemenuitem"), "activate",
                G_CALLBACK (on_home_clicked_event), properties);
	g_signal_connect (
				gtk_builder_get_object(builder, "sethomemenuitem"), "activate",
				G_CALLBACK (on_set_home_activate_event), properties);
	g_signal_connect (
				gtk_builder_get_object(builder, "okPrefs"), "clicked",
				G_CALLBACK (on_properties_ok_clicked), properties);
	g_signal_connect (
				gtk_builder_get_object(builder, "prefs_button"), "clicked",
				G_CALLBACK (on_properties_clicked_event), properties);
	g_signal_connect (G_OBJECT (map), "button-press-event",
                G_CALLBACK (on_button_press_event), (gpointer) map);

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));

    g_signal_connect (widget, "destroy",
                      G_CALLBACK (on_close), (gpointer) map);

		//pulls popup data from mapviewer.ui

	popup = GTK_WIDGET(gtk_builder_get_object(builder, "proppop"));

	about = GTK_WIDGET(gtk_builder_get_object(builder, "about"));

	//connect mapviewer.ui values to popup window
	gtk_builder_connect_signals(builder, popup);
	gtk_builder_connect_signals(builder, about);

    //Setup accelerators.
    ag = gtk_accel_group_new();
    gtk_accel_group_connect(ag, GDK_w, GDK_CONTROL_MASK, GTK_ACCEL_MASK,
                    g_cclosure_new(gtk_main_quit, NULL, NULL));
    gtk_accel_group_connect(ag, GDK_q, GDK_CONTROL_MASK, GTK_ACCEL_MASK,
                    g_cclosure_new(gtk_main_quit, NULL, NULL));
    gtk_window_add_accel_group(GTK_WINDOW(widget), ag);

	statusbar = GTK_STATUSBAR(gtk_builder_get_object(builder, "statusbar1"));
	st_ctx = gtk_statusbar_get_context_id(statusbar, "connect");
	
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
	aprsis_set_filter(properties->ctx, properties->lat,properties->lon,properties->range);
	printf("Filter Data Set\n");
    gtk_main ();
	
	sqlite3_close(db);

    fap_cleanup();
    aprsis_close(ctx);
	
    return(0);
}

/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
