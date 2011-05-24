// station.h

#ifndef STATION_H
#define STATION_H

#include <gtk/gtk.h>
#include <osm-gps-map.h>


typedef struct {
	gchar *callsign;
	char symbol[2];
	OsmGpsMapImage *image;
	GdkPixbuf *pix;
	double lat;
	double lon;
	double speed;
	double course;
} APRSMapStation;

gboolean process_packet(gchar *msg);

#endif
/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
