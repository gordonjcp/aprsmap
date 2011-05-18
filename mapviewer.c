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

OsmGpsMap *map;

GtkWidget *popup;
GtkEntry *latent;
GtkEntry *lonent;


double homelat = 55.0;
double homelon = -4.0;

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

static GdkPixbuf *g_star_image = NULL;
static GdkPixbuf *g_symbol1_image = NULL;
static GdkPixbuf *g_symbol2_image = NULL;
static OsmGpsMapImage *g_last_image = NULL;



void put_image(OsmGpsMap *map, double longitude, double latitude, char table, char code) {
	// plop an ARPS symbol onto the map
	// in the .pngs for FindU, the tables are 16x6 symbols

//	GdkPixmap *symbol = Gdk

}

gboolean gio_got_packet(GIOChannel *gio, GIOCondition condition, gpointer data) {
	GIOStatus ret;
	GError *err = NULL;
	gchar *msg;
	gsize len;
	fap_packet_t *packet;
	char errmsg[256]; // ugh
	
	if (condition & G_IO_HUP)
		g_error ("Read end of pipe died!\n");

	ret = g_io_channel_read_line (gio, &msg, &len, NULL, &err);
	if (ret == G_IO_STATUS_ERROR)
	g_error ("Error reading: %s\n", err->message);

	printf ("\n------------------------------------------\nRead %u bytes: %s\n", len, msg);

	packet = fap_parseaprs(msg, strlen(msg), 0);
	if (packet->error_code) {
		printf("couldn't decode that...\n");
		fap_explain_error(*packet->error_code, errmsg);
		printf("%s", errmsg);
	} else if (packet->src_callsign) {
                printf("Got packet from %s\n", packet->src_callsign);
	}
	if (packet->latitude) {
		printf("%f %f\n", *(packet->latitude), *(packet->longitude));
		printf("Symbol code: %c%c\n", packet->symbol_table,packet->symbol_code);
		if (packet->course) {
		    printf("Course: %d\n", *(packet->course));
		    printf("Speed: %fkm/h\n", *(packet->speed));
			osm_gps_map_image_add(map,*(packet->latitude), *(packet->longitude), g_symbol1_image);
		} else {
			osm_gps_map_image_add(map,*(packet->latitude), *(packet->longitude), g_star_image);
		}

    } else {
		printf("has no position information\n");
	}

	fap_free(packet);

	g_free (msg);
	return TRUE;
}
static gboolean
on_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    OsmGpsMapPoint coord;
    float lat, lon;
    OsmGpsMap *map = OSM_GPS_MAP(widget);
   //OsmGpsMapTrack *othertrack = OSM_GPS_MAP_TRACK(user_data);

/*
    if (event->type == GDK_3BUTTON_PRESS) {
        if (event->button == 1) {
            if (g_last_image)
                osm_gps_map_image_remove (map, g_last_image);
        }
        if (event->button == 3) {
            osm_gps_map_track_remove(map, othertrack);
        }
    }

    if (event->type == GDK_2BUTTON_PRESS) {
        osm_gps_map_convert_screen_to_geographic(map, event->x, event->y, &coord);
        osm_gps_map_point_get_degrees(&coord, &lat, &lon);
        if (event->button == 1) {
            osm_gps_map_gps_add (map,
                                 lat,
                                 lon,
                                 g_random_double_range(0,360));
        }
        if (event->button == 3) {
            osm_gps_map_track_add_point(othertrack, &coord);
        }
    }
*/
	// middle click adds a star, don't really need this either
	/*
    if (event->type == GDK_BUTTON_PRESS) {
        if (event->button == 2) {
        osm_gps_map_convert_screen_to_geographic(map, event->x, event->y, &coord);
        osm_gps_map_point_get_degrees(&coord, &lat, &lon);
            g_last_image = osm_gps_map_image_add (
                                    map,
                                    lat,
                                    lon,
                                    g_star_image);
        }
    }
    */
    return FALSE;
}

static gboolean
on_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    float lat,lon;
    //GtkEntry *entry = GTK_ENTRY(user_data);
    OsmGpsMap *map = OSM_GPS_MAP(widget);

    g_object_get(map, "latitude", &lat, "longitude", &lon, NULL);
    gchar *msg = g_strdup_printf("%f,%f",lat,lon);
    //gtk_entry_set_text(entry, msg);
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
    osm_gps_map_set_center_and_zoom(map,homelat,homelon,5);
    return FALSE;
}
static gboolean
on_properties_clicked_event (GtkWidget *widget, gpointer user_data)
{    
	gtk_window_present( GTK_WINDOW( popup ) );
	return FALSE;
}
static gboolean
on_properties_ok_clicked (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_hide(	GTK_WIDGET( popup ) );
	return FALSE;
}
static gboolean
on_properties_hide_event (GtkWidget *widget, gpointer user_data)
{
	gtk_widget_hide(	GTK_WIDGET( popup ) );
	return FALSE;
}
static gboolean
on_cache_clicked_event (GtkWidget *widget, gpointer user_data)
{
    OsmGpsMap *map = OSM_GPS_MAP(user_data);
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
        int zoom,max_zoom;
        OsmGpsMapPoint pt1, pt2;
        osm_gps_map_get_bbox(map, &pt1, &pt2);
        g_object_get(map, "zoom", &zoom, "max-zoom", &max_zoom, NULL);
        osm_gps_map_download_maps(map, &pt1, &pt2, zoom, max_zoom);
    } else {
        osm_gps_map_download_cancel_all(map);
    }
    return FALSE;
}

static void
on_tiles_queued_changed (OsmGpsMap *image, GParamSpec *pspec, gpointer user_data)
{
    gchar *s;
    int tiles;
    GtkLabel *label = GTK_LABEL(user_data);
    g_object_get(image, "tiles-queued", &tiles, NULL);
    s = g_strdup_printf("%d", tiles);
    gtk_label_set_text(label, s);
    g_free(s);
}

/*static void
on_gps_alpha_changed (GtkAdjustment *adjustment, gpointer user_data)
{
    OsmGpsMap *map = OSM_GPS_MAP(user_data);
    OsmGpsMapTrack *track = osm_gps_map_gps_get_track (map);
    float f = gtk_adjustment_get_value(adjustment);
    g_object_set (track, "alpha", f, NULL);}

static void
on_gps_width_changed (GtkAdjustment *adjustment, gpointer user_data)
{
    OsmGpsMap *map = OSM_GPS_MAP(user_data);
    OsmGpsMapTrack *track = osm_gps_map_gps_get_track (map);
    float f = gtk_adjustment_get_value(adjustment);
    g_object_set (track, "line-width", f, NULL);
} */

static void
on_star_align_changed (GtkAdjustment *adjustment, gpointer user_data)
{
    const char *propname = user_data;
    float f = gtk_adjustment_get_value(adjustment);
    if (g_last_image)
        g_object_set (g_last_image, propname, f, NULL);
}

/*static void
on_gps_color_changed (GtkColorButton *widget, gpointer user_data)
{
    GdkColor c;
    OsmGpsMapTrack *track = OSM_GPS_MAP_TRACK(user_data);
    gtk_color_button_get_color (widget, &c);
    g_object_set(track, "color", &c, NULL);
}*/

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
    //OsmGpsMap *map;
    OsmGpsMapLayer *osd;
    //OsmGpsMapTrack *rightclicktrack;
    const char *repo_uri;
    char *cachedir, *cachebasedir;
    GError *error = NULL;
    GOptionContext *context;
	GIOChannel *gio_read;

<<<<<<< HEAD
	//int sockfd = aprsis_connect(NULL, 10152);
=======
	aprsis_ctx *ctx = aprsis_new("england.aprs2.net", "14580", "aprsmap", "-1");
	aprsis_connect(ctx);
>>>>>>> 8238fc177ddad99187212f16f853aea5e74afff0

    // initialise APRS parser
    fap_init();

    g_thread_init(NULL);
    gtk_init (&argc, &argv);

<<<<<<< HEAD
	int sockfd = aprsis_connect();
	aprsis_login(sockfd);
=======
	aprsis_set_filter(ctx, 55.0, -4.0, 600);
	aprsis_login(ctx);
>>>>>>> 8238fc177ddad99187212f16f853aea5e74afff0

    gio_read = g_io_channel_unix_new (ctx->sockfd);
    g_io_channel_set_encoding(gio_read, NULL, &error);
    if (!g_io_add_watch (gio_read, G_IO_IN | G_IO_HUP, gio_got_packet, NULL))
        g_error ("Cannot add watch on GIOChannel!\n");

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
						"show-zoom",TRUE,
						"show-gps-in-zoom",TRUE,
                        NULL);
    osm_gps_map_layer_add(OSM_GPS_MAP(map), osd);
    g_object_unref(G_OBJECT(osd));

    //Add a second track for right clicks <- Debating whether this is required
    //rightclicktrack = osm_gps_map_track_new();
    //osm_gps_map_track_add(OSM_GPS_MAP(map), rightclicktrack);

    g_free(cachedir);
    g_free(cachebasedir);

    //Enable keyboard   navigation
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_FULLSCREEN, GDK_F11);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_UP, GDK_Up);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_DOWN, GDK_Down);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_LEFT, GDK_Left);
    osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_RIGHT, GDK_Right);

    //Build the UI
    g_star_image = gdk_pixbuf_new_from_file_at_size ("poi.png", 24,24,NULL);
    g_symbol1_image = gdk_pixbuf_new_from_file("campervan.png", &error);
    //g_symbol2_image = gdk_pixbuf_new_from_file("allicon2.png", &error);

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "mapviewer.ui", &error);
    if (error)
        g_error ("ERROR: %s\n", error->message);

    gtk_box_pack_start (
                GTK_BOX(gtk_builder_get_object(builder, "map_box")),
                GTK_WIDGET(map), TRUE, TRUE, 0);
	

    //Init values
    float lw,a;
    GdkColor c;
    //OsmGpsMapTrack *gpstrack = osm_gps_map_gps_get_track (map);
    //g_object_get (gpstrack, "line-width", &lw, "alpha", &a, NULL);
    //osm_gps_map_track_get_color(gpstrack, &c);
    
    // centre on UK, because I'm UK-centric
    osm_gps_map_set_center_and_zoom(map, homelat, homelon, 5);
    
    gtk_adjustment_set_value (
                GTK_ADJUSTMENT(gtk_builder_get_object(builder, "gps_width_adjustment")),
                lw);
    gtk_adjustment_set_value (
                GTK_ADJUSTMENT(gtk_builder_get_object(builder, "gps_alpha_adjustment")),
                a);
    gtk_adjustment_set_value (
                GTK_ADJUSTMENT(gtk_builder_get_object(builder, "star_xalign_adjustment")),
                0.5);
    gtk_adjustment_set_value (
                GTK_ADJUSTMENT(gtk_builder_get_object(builder, "star_yalign_adjustment")),
                0.5);
    gtk_color_button_set_color (
                GTK_COLOR_BUTTON(gtk_builder_get_object(builder, "gps_colorbutton")),
                &c);

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
				G_CALLBACK (on_properties_ok_clicked), (gpointer) map);
    g_signal_connect (
                gtk_builder_get_object(builder, "cache_button"), "clicked",
                G_CALLBACK (on_cache_clicked_event), (gpointer) map);
    /*g_signal_connect (
                gtk_builder_get_object(builder, "gps_alpha_adjustment"), "value-changed",
                G_CALLBACK (on_gps_alpha_changed), (gpointer) map); */
    /*g_signal_connect (
                gtk_builder_get_object(builder, "gps_width_adjustment"), "value-changed",
                G_CALLBACK (on_gps_width_changed), (gpointer) map);*/
    g_signal_connect (
                gtk_builder_get_object(builder, "star_xalign_adjustment"), "value-changed",
                G_CALLBACK (on_star_align_changed), (gpointer) "x-align");
    g_signal_connect (
                gtk_builder_get_object(builder, "star_yalign_adjustment"), "value-changed",
                G_CALLBACK (on_star_align_changed), (gpointer) "y-align");
  /*  g_signal_connect (
                gtk_builder_get_object(builder, "gps_colorbutton"), "color-set",
                G_CALLBACK (on_gps_color_changed), (gpointer) gpstrack);
    g_signal_connect (G_OBJECT (map), "button-press-event",
                G_CALLBACK (on_button_press_event), (gpointer) rightclicktrack); */
    g_signal_connect (G_OBJECT (map), "button-release-event",
                G_CALLBACK (on_button_release_event),
                (gpointer) gtk_builder_get_object(builder, "text_entry"));
    g_signal_connect (G_OBJECT (map), "notify::tiles-queued",
                G_CALLBACK (on_tiles_queued_changed),
                (gpointer) gtk_builder_get_object(builder, "cache_label"));

    widget = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));

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
	gchar *latmsg = g_strdup_printf("%f",homelat);
	gchar *lonmsg = g_strdup_printf("%f",homelon);
	gtk_entry_set_text(latent, latmsg);
	gtk_entry_set_text(lonent, lonmsg);

	g_object_unref( G_OBJECT( builder ) );

    gtk_widget_show_all (widget);
	
    //g_log_set_handler ("OsmGpsMap", G_LOG_LEVEL_MASK, g_log_default_handler, NULL);
    gtk_main ();


    fap_cleanup();
    aprsis_close(ctx);
    return(0);
}

/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
