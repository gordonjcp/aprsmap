// station.h

#ifndef STATION_H
#define STATION_H

#include <gtk/gtk.h>
#include <osm-gps-map.h>

typedef enum {
	APRS_NOFIX,
	APRS_VALIDFIX
} aprs_fix_t;

typedef struct {
	gchar *callsign;
	gchar symbol[2];
	OsmGpsMapImage *image;
	OsmGpsMapTrack *track;
	GdkPixbuf *pix;
	cairo_surface_t *icon;
	gdouble speed;
	gdouble course;
	gdouble lat, lon;
	aprs_fix_t fix;
} APRSMapStation;

gboolean process_packet(gchar *msg);
//void write_to_db(float latitude, float longitude, float course, char call);
#endif
/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
