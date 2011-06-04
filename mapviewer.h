#ifndef MAPVIEWER_H
#define MAPVIEWER_H

gboolean process_packet(gchar *msg);

//aprs details structure - enables passing of variables between the properties pop up and the main program

typedef struct _aprs_details {
    double lat;
    double lon;
    int range;
    aprsis_ctx *ctx;
} aprs_details;

aprs_details *aprs_details_new(double lat,double lon,int range,aprsis_ctx *ctx);

#endif
