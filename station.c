// station.c

#include <gtk/gtk.h>
#include <string.h>
#include <fap.h>
#include <osm-gps-map.h>

#include "station.h"

extern GdkPixbuf *g_star_image;
extern GdkPixbuf *g_symbol_image;
extern GdkPixbuf *g_symbol_image2;

extern GHashTable *stations;
extern OsmGpsMap *map;


char *packet_type[] = {

"LOCATION",
"OBJECT",
"ITEM",
"MICE",
"NMEA",
"WX",
"MESSAGE",
"CAPABILITIES",
"STATUS",
"TELEMETRY",
"TELEMETRY_MESSAGE",
"DX_SPOT",
"EXPERIMENTAL"

} ;


static GdkPixbuf *aprsmap_get_symbol(fap_packet_t *packet) {
	// return the symbol pixbuf

	guint xo, yo, c;
	GdkPixbuf *pix;

	if (packet->symbol_table && packet->symbol_code) {
		printf("Symbol: '%c%c'\n", packet->symbol_table, packet->symbol_code);

	    c = packet->symbol_code-32;
   		yo = (c*16)%256;
   		xo = c &0xf0;			
		if (packet->symbol_table == '\\') {
   			pix = gdk_pixbuf_new_subpixbuf(g_symbol_image2, xo, yo, 16, 16);
		} else {
			pix = gdk_pixbuf_new_subpixbuf(g_symbol_image, xo, yo, 16, 16);
		}
		return pix;
	}
	
	// otherwise, nothing		
	return NULL;
}

static gboolean aprsmap_station_moved(fap_packet_t *packet, APRSMapStation *station) {
	// has the station moved?
	if (!packet->latitude) return FALSE; // don't know if it's moved; nothing to tell us it has
	if (*(packet->latitude) != station->lat) return TRUE;
	if (*(packet->longitude) != station->lon) return TRUE;
	return FALSE;
}

gboolean process_packet(gchar *msg) {

	fap_packet_t *packet;
	APRSMapStation *station;

	char errmsg[256]; // ugh
	packet = fap_parseaprs(msg, strlen(msg), 0);
	if (packet->error_code) {
		printf("couldn't decode that...\n");
		fap_explain_error(*packet->error_code, errmsg);
		g_message("%s", errmsg);
		return TRUE;
	}
	//printf("packet type='%s'\n", packet_type[*(packet->type)]);
	//if (packet->latitude) printf("has position\n");

	// have we got this station?  Look it up
	station = g_hash_table_lookup(stations, packet->src_callsign);
	
	if (!station) { // no, create a new one
		station = g_new0(APRSMapStation, 1);
		station->callsign = g_strdup(packet->src_callsign);
		station->pix = aprsmap_get_symbol(packet);
		if (station->pix) {
	    	station->image = osm_gps_map_image_add(map,*(packet->latitude), *(packet->longitude), station->pix); 			
		}
		if (packet->latitude) {
			// can't see why it would have lat but not lon, probably a horrible bug in the waiting
			station->lat = *(packet->latitude);
			station->lon = *(packet->longitude);		
		}

		g_hash_table_insert(stations, station->callsign, station);
		printf("inserted station %s\n", station->callsign);
	} else {
		printf("already got station %s\n", station->callsign);
		if (aprsmap_station_moved(packet, station)) {
			printf("it's moved\n");
			if (station->image) {
				osm_gps_map_image_remove(map, station->image);
				station->image = osm_gps_map_image_add(map,*(packet->latitude), *(packet->longitude), station->pix); 			
			}
		} else printf("it hasn't moved\n");
		

	
	}
	
	fap_free(packet);
	// need to keep returning "true" for the callback to keep running
	return TRUE;
}

/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
