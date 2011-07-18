// station.c

#include <gtk/gtk.h>
#include <string.h>
#include <fap.h>
#include <osm-gps-map.h>
#include <sqlite3.h>
#include <math.h>

#include "station.h"

extern GdkPixbuf *g_star_image;
extern cairo_surface_t *g_symbol_image;
extern cairo_surface_t *g_symbol_image2;

extern GHashTable *stations;
extern OsmGpsMap *map;
extern rc;
extern *zErrMsg = 0;
extern sqlite3 *db;
// workaround for libfap bug
/// The magic constant.
#define PI 3.14159265
/// Degrees to radians.
#define DEG2RAD(x) (x/360*2*PI)
/// Radians to degrees.
#define RAD2DEG(x) (x*(180/PI))


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

static void aprsmap_set_icon(fap_packet_t *packet, APRSMapStation *station) {
	// get the icon given a packet to get the symbol from
	cairo_t *cr;
	guint c;
	gdouble xo, yo;
	gdouble angle;
	
	if (!station->icon) {
		station->icon = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 22, 22);
	}
	
	cr = cairo_create(station->icon);

	// calculate the symbol image
	// this fails on symbols that aren't on the \ or / table
	// they default to the / table, probably wrongly ;-)
    c = packet->symbol_code-32;
	yo = (gdouble)((c*16)%256);
	xo = (gdouble)(c &0xf0);

	if (station->course) {
	  	cairo_translate(cr, 8, 8);
	  	if (station->course > 180) {
	  		cairo_scale(cr, -1, 1);
		  	angle = 270.0f - station->course;
		} else {
			angle = station->course - 90.0f;
		}
	  	cairo_rotate(cr, DEG2RAD(angle));
	   	cairo_translate(cr, -8, -8);
	}
	
	if (packet->symbol_table == '\\') {
   		cairo_set_source_surface (cr, g_symbol_image2, 1-xo, 1-yo);

	} else {
   		cairo_set_source_surface (cr, g_symbol_image, 1-xo, 1-yo);
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_rectangle (cr, 1, 1, 16, 16);
	//cairo_clip(cr);

	cairo_fill (cr);
	cairo_surface_flush(station->icon);
	cairo_destroy(cr);	
}

static void aprsmap_get_label(fap_packet_t *packet, APRSMapStation *station) {
	// return the symbol pixbuf

	guint width=90, height=22;

	gdouble xo, yo;
	guint c;
	cairo_t *cr;
	cairo_text_extents_t extent;
	cairo_surface_t *surface;
	cairo_content_t content;
	GdkPixbuf *dest;

	if (packet->symbol_table && packet->symbol_code) {
		surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
		cr = cairo_create(surface);

		// get callsign/name size
		cairo_select_font_face(cr, "Sans",
			CAIRO_FONT_SLANT_NORMAL,
			CAIRO_FONT_WEIGHT_NORMAL);
    	cairo_set_font_size(cr, 12);
		cairo_text_extents(cr, station->callsign, &extent);

		// draw background
		cairo_arc(cr, 6.5f, 6.5f, 6,  3.14, 4.71);
		cairo_arc(cr, 24+extent.width-6.5f, 6.5f, 6,  4.71, 6.28);
		cairo_arc(cr, 24+extent.width-6.5f, height-6.5f, 6, 0, 1.57);
		cairo_arc(cr, 6.5f, height-6.5f, 6, 1.57, 3.14);
		cairo_close_path(cr);

		cairo_set_source_rgba(cr, 1, 1, 1, .75);
		cairo_set_source_rgba(cr, .85, .85, .85, .75);
		cairo_fill_preserve(cr);

		//cairo_set_source_rgba(cr, .5, .5, .5, 1);
		cairo_set_source_rgba(cr, 1, 1, 1, 1);
		cairo_set_line_width(cr, 2.0f);
		cairo_stroke(cr);

		// draw the icon
		aprsmap_set_icon(packet, station);
		cairo_set_source_surface(cr, station->icon, 2, 2);
		cairo_paint(cr);
		
		// draw the callsign
    	cairo_move_to(cr, 20, height-1-(height-extent.height)/2);
    	cairo_set_source_rgba(cr, 0, 0, 0, 1);
    	cairo_show_text(cr, station->callsign);

		// munge it into a pixbuf for OsmGpsMapImage
		cairo_surface_flush(surface);
		content = cairo_surface_get_content(surface);
		if (!station->pix) station->pix = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);

    convert_alpha (gdk_pixbuf_get_pixels (station->pix),
                   gdk_pixbuf_get_rowstride (station->pix),
                   cairo_image_surface_get_data (surface),
                   cairo_image_surface_get_stride (surface),
                   0, 0,
                   width, height);

		cairo_surface_destroy(surface);
		cairo_destroy(cr);
	}
}

static gboolean aprsmap_station_moved(fap_packet_t *packet, APRSMapStation *station) {
	// has the station moved?
	if (!packet->latitude) return FALSE; // don't know if it's moved; nothing to tell us it has
	if (station->fix == APRS_NOFIX) return TRUE; // if it's got a latitude we now have a fix

	// if there was a previous fix and this packet contains a position, compare
	if (*(packet->latitude) != station->lat) return TRUE;
	if (*(packet->longitude) != station->lon) return TRUE;
	return FALSE;
}

static APRSMapStation* get_station(fap_packet_t *packet) {
	// return either a new station, or an existing one
	char name[10];
	APRSMapStation *station;
	
	// objects and items are sent from a callsign, but have a name
	// which is passed as part of the payload
	bzero(&name, 10);
	switch (*(packet->type)) {
		case fapOBJECT:
		case fapITEM:
			strncpy(name, packet->object_or_item_name, 9);
			break;
		default:
			strncpy(name, packet->src_callsign, 9);
		break;
	}
	station = g_hash_table_lookup(stations, name);
	if (!station) {
//		printf("new station %s\n", name);
		station = g_new0(APRSMapStation, 1);
		station->callsign = g_strdup(name);
	}
	return station;
}

double gjcp_direction(double lon0, double lat0, double lon1, double lat1)
{
	double direction;

	/* Convert degrees into radians. */
	lon0 = DEG2RAD(lon0);
	lat0 = DEG2RAD(lat0);
	lon1 = DEG2RAD(lon1);
	lat1 = DEG2RAD(lat1);

	/* Direction from Aviation Formulary V1.42 by Ed Williams by way of
	 * http://mathforum.org/library/drmath/view/55417.html */
	direction = atan2(sin(lon1-lon0)*cos(lat1), cos(lat0)*sin(lat1)-sin(lat0)*cos(lat1)*cos(lon1-lon0));

	if ( direction < 0 )
	{
		/* Make direction positive. */
		direction += 2 * PI;
	}
	return RAD2DEG(direction);
}



static void position_station(APRSMapStation *station, fap_packet_t *packet) {
	// deal with position packets
	OsmGpsMapPoint pt;
	if (station->fix == APRS_VALIDFIX) {
//		printf("co-ordinates: %f %f\n", station->lat, station->lon);
		if ((station->lat != *(packet->latitude)) || (station->lon != *(packet->longitude))) {
//			printf("it moved\n");
			// not all APRS packets contain speed and course
			// we can work it out though
			station->course = gjcp_direction(station->lon, station->lat, *(packet->longitude), *(packet->latitude));
			station->lat = *(packet->latitude);
			station->lon = *(packet->longitude);
	char zlat[10]; char zlon[10]; char zcourse[10];
 	int n, m, o,rc;
	n=sprintf(zlat,"%f",station->lat);   
	m=sprintf(zlon,"%f",station->lon);
	o=sprintf(zcourse, "%f", station->course);
	char *zSQL = sqlite3_mprintf("INSERT INTO call_data (call, object, course, lon, lat) VALUES (%Q,%Q,%Q,%Q,%Q)",packet->src_callsign,packet->object_or_item_name,zcourse,zlon,zlat);
	rc = sqlite3_exec(db, zSQL, 0, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
	sqlite3_free(zSQL);
			// we may need to create a track, then
			if (!station->track) {
				station->track = osm_gps_map_track_new();
				osm_gps_map_point_set_degrees (&pt, station->lat, station->lon);
				osm_gps_map_track_add_point(station->track, &pt);
			    osm_gps_map_track_add(OSM_GPS_MAP(map), station->track);
			} else {
				// already got a track
				osm_gps_map_image_remove(map, station->image);
				osm_gps_map_point_set_degrees (&pt, station->lat, station->lon);
				osm_gps_map_track_add_point(station->track, &pt);
				aprsmap_get_label(packet, station);
				station->image = osm_gps_map_image_add(map, station->lat, station->lon, station->pix);
				g_object_set (station->image, "x-align", 0.0f, NULL); 
			}
		}
	
	} else {
//		printf("first position packet received for this station\n");
		if (packet->latitude) {
			station->lat = *(packet->latitude);
			station->lon = *(packet->longitude);
			if (packet->course) station->course = *(packet->course);
			station->fix = APRS_VALIDFIX;
			aprsmap_get_label(packet, station);
			if (station->pix) {
	    		station->image = osm_gps_map_image_add(map,*(packet->latitude), *(packet->longitude), station->pix); 
				g_object_set (station->image, "x-align", 0.0f, NULL); 						
			} else {
				g_error("not really an error, just checking to see if we ever get a posit with no symbol");
			}
		} else {
			g_message("got a position packet, with no valid position. ignoring.");
		}
	}

	char zlat[10]; char zlon[10]; char zcourse[10];
 	int n, m, o,rc;
	n=sprintf(zlat,"%f",station->lat);   
	m=sprintf(zlon,"%f",station->lon);
	o=sprintf(zcourse, "%f", station->course);
	char *zSQL = sqlite3_mprintf("INSERT INTO call_data (call, object, course, lon, lat) VALUES (%Q,%Q,%Q,%Q,%Q)",packet->src_callsign,packet->object_or_item_name,zcourse,zlon,zlat);
	rc = sqlite3_exec(db, zSQL, 0, 0, &zErrMsg);
	if( rc!=SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
}
}
gboolean process_packet(gchar *msg) {
	// process an incoming packet, and call a suitable function
	fap_packet_t *packet;
	fap_packet_type_t type;
	APRSMapStation *station;
	char errmsg[256];
	char name[10];

	packet = fap_parseaprs(msg, strlen(msg), 0);
	if (packet->error_code) {
		fap_explain_error(*packet->error_code, errmsg);
		g_message("%s", errmsg);
		return TRUE;
	}
	
	type = *(packet->type);
	printf("packet type is %s\n", packet_type[type]);

	// see if we have a record of this, or not
	station = get_station(packet);

	switch(type) {
		case fapOBJECT:
		case fapITEM:
		case fapLOCATION:
			position_station(station, packet);
			break;
		default:
			printf("unhandled\n");
			break;
	}
	g_hash_table_replace(stations, station->callsign, station);
}
/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
