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

gboolean process_packet(gchar *msg) {

	fap_packet_t *packet;
	APRSMapStation *station;

	char errmsg[256]; // ugh
	packet = fap_parseaprs(msg, strlen(msg), 0);
	if (packet->error_code) {
		printf("couldn't decode that...\n");
		fap_explain_error(*packet->error_code, errmsg);
		printf("%s", errmsg);
	} else if (packet->src_callsign) { // can't see why it wouldn't have a source callsign
		station = g_hash_table_lookup(stations, packet->src_callsign);
		if (!station) { // work out what of the packet is valid and store it in the hash
			station = g_new0(APRSMapStation, 1);
			station->callsign = g_strdup(packet->src_callsign);	
			
			if (packet->symbol_table && packet->symbol_code) {
				printf("%02x %02x\n", packet->symbol_table, packet->symbol_code);
			
				guint xo, yo, c;
			    c = packet->symbol_code-32;
	    		yo = (c*16)%256;
	    		xo = c &0xf0;			
				if (packet->symbol_table == '\\') {
	    			station->pix = gdk_pixbuf_new_subpixbuf(g_symbol_image2, xo, yo, 16, 16);
		    	} else {
					station->pix = gdk_pixbuf_new_subpixbuf(g_symbol_image, xo, yo, 16, 16);
		    	}
		    	station->image = osm_gps_map_image_add(map,*(packet->latitude), *(packet->longitude), station->pix); 			
			}
			g_hash_table_insert(stations, station->callsign, station);
			printf("inserted station %s\n", station->callsign);
		} else {
			printf("already got station %s\n", station->callsign);
			if (station->image) {
				osm_gps_map_image_remove(map, station->image);
				station->image = osm_gps_map_image_add(map,*(packet->latitude), *(packet->longitude), station->pix); 			
			}
		}

	fap_free(packet);
	
	return TRUE;
}
}

	/*
		printf("Got packet from %s (%d bytes)\n", packet->src_callsign, strlen(packet->src_callsign));
		if (packet->latitude) {
			//print lat/lon value
	        APRSMapStation *station = g_hash_table_lookup(stations, packet->src_callsign);
		    guint xo, yo, c;
    		if (!station) {
    			c = packet->symbol_code-32;
	    		yo = (c*16)%256;
	    		xo = c &0xf0;
	    		station = g_new0(APRSMapStation, 1);
	    		station->callsign = g_strdup(packet->src_callsign);
				if (packet->symbol_table == '\\') {
	    			station->pix = gdk_pixbuf_new_subpixbuf(g_symbol_image2, xo, yo, 16, 16);
		    	} else {
					station->pix = gdk_pixbuf_new_subpixbuf(g_symbol_image, xo, yo, 16, 16);
		    	}
		    	station->image = osm_gps_map_image_add(map,*(packet->latitude), *(packet->longitude), station->pix);   		
	    		g_hash_table_insert(stations, station->callsign, station);
			} else {
	    		printf("already got %s\n", station->callsign);
			}
			printf("%f %f\n", *(packet->latitude), *(packet->longitude));
	    } else {
			printf("has no position information\n");
	}
	
	
	}
	*/

