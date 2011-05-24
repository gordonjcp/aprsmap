// station.h
#include <gtk/gtk.h>


typedef struct {
	gchar *callsign;
	char symbol[2];
	OsmGpsMapImage *image;
	GdkPixbuf *pix;
} APRSMapStation;

gboolean process_packet(gchar *msg);

