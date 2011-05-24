// station.c

#include <gtk/gtk.h>
#include <string.h>
#include <fap.h>
#include <osm-gps-map.h>

#include "station.h"

extern GdkPixbuf *g_star_image;
extern cairo_surface_t *g_symbol_image;
extern cairo_surface_t *g_symbol_image2;

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

static void
convert_alpha (guchar *dest_data,
               int     dest_stride,
               guchar *src_data,
               int     src_stride,
               int     src_x,
               int     src_y,
               int     width,
               int     height)
{
  int x, y;

  src_data += src_stride * src_y + src_x * 4;

  for (y = 0; y < height; y++) {
    guint32 *src = (guint32 *) src_data;

    for (x = 0; x < width; x++) {
      guint alpha = src[x] >> 24;

      if (alpha == 0)
        {
          dest_data[x * 4 + 0] = 0;
          dest_data[x * 4 + 1] = 0;
          dest_data[x * 4 + 2] = 0;
        }
      else
        {
          dest_data[x * 4 + 0] = (((src[x] & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
          dest_data[x * 4 + 1] = (((src[x] & 0x00ff00) >>  8) * 255 + alpha / 2) / alpha;
          dest_data[x * 4 + 2] = (((src[x] & 0x0000ff) >>  0) * 255 + alpha / 2) / alpha;
        }
      dest_data[x * 4 + 3] = alpha;
    }

    src_data += src_stride;
    dest_data += dest_stride;
  }
}



static GdkPixbuf *aprsmap_get_symbol(fap_packet_t *packet) {
	// return the symbol pixbuf

	guint width=72, height=18;

	gdouble xo, yo;
	guint c;
	GdkPixbuf *pix;
	GdkPixbuf *symbol;
	cairo_text_extents_t extent;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_content_t content;
	GdkPixbuf *dest;


	if (packet->symbol_table && packet->symbol_code) {
		printf("Symbol: '%c%c'\n", packet->symbol_table, packet->symbol_code);

		surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
		cr = cairo_create(surface);

		// draw background
		cairo_set_source_rgba(cr, 1, 1, 1, .5);
		//cairo_rectangle(cr, 18 , 0, width-18, height);
		//cairo_clip(cr);
		cairo_rectangle(cr, 0, 0, width, height);
		cairo_clip(cr);
		cairo_paint(cr);
		
		
	    c = packet->symbol_code-32;
   		yo = (gdouble)((c*16)%256);
   		xo = (gdouble)(c &0xf0);
		if (packet->symbol_table == '\\') {
   			//symbol = gdk_pixbuf_new_subpixbuf(g_symbol_image2, xo, yo, 16, 16);
   			cairo_set_source_surface (cr, g_symbol_image2, 1-xo, 1-yo);

		} else {
//			symbol = gdk_pixbuf_new_subpixbuf(g_symbol_image, xo, yo, 16, 16);
   			cairo_set_source_surface (cr, g_symbol_image, 1-xo, 1-yo);
		}
		cairo_rectangle (cr, 1, 1, 16, 16);
		cairo_fill (cr);

    	cairo_set_font_size(cr, 10);
    	cairo_move_to(cr, 20, 13);
    	cairo_set_source_rgba(cr, 0, 0, 0, 1);
    	cairo_show_text(cr,packet->src_callsign);
    	
    	
		cairo_surface_flush(surface);

		content = cairo_surface_get_content(surface);
		pix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);

    convert_alpha (gdk_pixbuf_get_pixels (pix),
                   gdk_pixbuf_get_rowstride (pix),
                   cairo_image_surface_get_data (surface),
                   cairo_image_surface_get_stride (surface),
                   0, 0,
                   width, height);

		cairo_surface_destroy(surface);
		cairo_destroy(cr);
		
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
			g_object_set (station->image, "x-align", 0.0f, NULL); 						
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
				g_object_set (station->image, "x-align", 0.0f, NULL); 			
			}
		} else printf("it hasn't moved\n");
		

	
	}
	
	fap_free(packet);
	// need to keep returning "true" for the callback to keep running
	return TRUE;
}

/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
