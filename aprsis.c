// network.c
// connect to APRS-IS server
// Copyright 2011 Gordonjcp MM0YEQ
// GPL V3 applies


#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gio/gio.h>

#include <sys/socket.h>
#include <string.h>
#include <netdb.h> 
#include <errno.h>
#include <arpa/inet.h>

#include "aprsis.h"
#include "station.h"
#include "mapviewer.h"

static gboolean connected;
static guint aprs1;
static guint reconnect_timer;
static GIOChannel *aprsis_io;

enum { APRSIS_DISCONNECTED, APRSIS_CONNECTING, APRSIS_CONNECTED };

aprsis_ctx *aprsis_new(const char *host, const char *port, const char *user, const char *pass) {
	aprsis_ctx *ctx = calloc(1, sizeof(aprsis_ctx));

	ctx->sockfd = -1;
	ctx->host = strdup(host);
	ctx->port = strdup(port);
	ctx->user = strdup(user);
	ctx->pass = strdup(pass);

	ctx->log_file = NULL;

	return ctx;
}

void aprsis_set_log(aprsis_ctx *ctx, FILE *file) {
	ctx->log_file = file;
}

int aprsis_read(aprsis_ctx *ctx, char *buf, size_t len) {
	// read a line from the GSocket, and maybe log it to a file
	GError *err;
	int count = g_socket_receive(ctx->skt, buf, len, NULL, &err);

	if (ctx->log_file != NULL) {
		printf("Logging read\n");
		fprintf(ctx->log_file, "< %s\n", buf);
	}
	return count;
}

int aprsis_write(aprsis_ctx *ctx, char *buf, size_t len) {
	// send a line to the GSocket
	GError *err;
	int count = g_socket_send(ctx->skt, buf, len, NULL, &err);

	if (ctx->log_file != NULL) {
		printf("Logging write\n");
		fprintf(ctx->log_file, "> %s\n", buf);
	}

	return count;
}

void aprsis_write_log(aprsis_ctx *ctx, char *buf, size_t len) {
	if (ctx->log_file != NULL) {
		fprintf(ctx->log_file, "< %s\n", buf);
	}
}

GError *aprsis_connect(aprsis_ctx *ctx) {
	// connect to an APRS-IS server
	// return GError
	
	ctx->state = APRSIS_CONNECTING;
	GError *err = NULL;
	GSocketClient *client = g_socket_client_new();
	GSocketConnection *conn = g_socket_client_connect_to_host(client, ctx->host, 14580, NULL, &err); // FIXME needs to convert string to int
	
	if (conn) {
		ctx->skt = g_socket_connection_get_socket(conn);
	}
	if (ctx->skt) {
		ctx->sockfd = g_socket_get_fd(ctx->skt);
	}

	if(err) {
		g_message ("%s", err->message);
		ctx->state = APRSIS_DISCONNECTED;
	}
	return err;
}

int aprsis_login(aprsis_ctx *ctx) {
	// wait for prompt, send filter message
	char buf[256];
	int n;

	// note that this doesn't *actually* check what the prompt is
	bzero(&buf, 256);
	n = aprsis_read(ctx, buf, 256);
	if (n<0) {
		g_error("couldn't read from socket");
	}
	g_message("got: %s",buf);

	sprintf(buf, APRSIS_LOGIN"\n", ctx->user, ctx->pass);
	g_message("sending: %s", buf);
	aprsis_write(ctx, buf, strlen(buf));
	bzero(&buf, 256);
	n = aprsis_read(ctx, buf, 256);
	if (n<0) {
		g_error("couldn't read from socket");
	}
	g_message("got: %s",buf);
	
	return 0;
}

void aprsis_set_filter(aprsis_ctx *ctx, double latitude, double longitude, int radius) {
	// sets a filter given latitude, longitude and radius
	ctx->latitude = latitude;
	ctx->longitude = longitude;
	ctx->radius = radius;

	if (ctx->skt) {
		char buf[64];
		snprintf(buf, sizeof(buf), "#filter r/%.0f/%.0f/%d\n", latitude, longitude, radius);
		g_message("Sending filter: %s", buf);
		aprsis_write(ctx, buf, strlen(buf));
	}
}

void aprsis_set_filter_string(aprsis_ctx *ctx, char *filter) {
	// send a filter string, for more complex filters
	if (ctx->sockfd != -1) {
		char buf[64];
		snprintf(buf, sizeof(buf), "#filter %s\n", filter);
		g_message("Sending filter: %s", buf);
		aprsis_write(ctx, buf, strlen(buf));
	}
}


void aprsis_close(aprsis_ctx *ctx) {
	// close the connection and clean up
	close(ctx->sockfd);
	if (ctx->host != NULL) {
		free(ctx->host);
	}
	
	if (ctx->port != NULL) {
		free(ctx->port);
	}

	if (ctx->user != NULL) {
		free(ctx->user);
	}
	
	if (ctx->pass != NULL) {
		free(ctx->pass);
	}
	free(ctx);
	if (aprs1) {
		g_source_remove(aprs1);
	}
	if (aprsis_io) {
		g_io_channel_unref (aprsis_io);
	}
	
}

static gboolean aprsis_got_packet(GIOChannel *gio, GIOCondition condition, gpointer data) {
	// callback when GIOChannel tells us there's an APRS packet to be handled
	GIOStatus ret;
	GError *err = NULL;
	gchar *msg;
	gsize len;
	aprsis_ctx *ctx = (aprsis_ctx *) data;

	if (condition & G_IO_HUP)
		g_message ("Read end of pipe died!");   // FIXME - handle this more gracefully

	if (condition & G_IO_ERR) {
		g_message ("IO error");
		return FALSE;
	}
		
	ret = g_io_channel_read_line (gio, &msg, &len, NULL, &err);
	if (ret == G_IO_STATUS_ERROR) {
		g_message("Error reading: %s", err->message);
		ctx->state = APRSIS_DISCONNECTED;
		return FALSE;
	}
	if (ret == G_IO_STATUS_EOF) {
		g_message("EOF (server disconnected)");
		ctx->state = APRSIS_DISCONNECTED;
		return FALSE; // shut down the callback, for now 
	}
	
	aprsis_write_log(ctx, msg, len);
	
	if (msg[0] == '#') {
		printf("can ignore comment message: %s\n", msg);
	} else {
		process_packet(msg);
	}

	g_free(msg);
	return TRUE;
}

static gboolean aprsis_reconnect(void *ptr) {
	// called once a second when the timer times out
	aprsis_ctx *ctx = ptr;
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	start_aprsis(ctx);
}

static gboolean
aprsis_io_error(GIOChannel *src, GIOCondition condition, void *ptr)
{
	printf("*** %s(): \n",__PRETTY_FUNCTION__);
	aprsis_ctx *ctx = ptr;
	ctx->state = APRSIS_DISCONNECTED;
	reconnect_timer = g_timeout_add_seconds(5, aprsis_reconnect, ctx);
	return FALSE;
}

static void start_aprsis_thread(void *ptr) {

    GError *error = NULL;
	aprsis_ctx *ctx = ptr;

	if (!reconnect_timer) reconnect_timer = g_timeout_add_seconds(10, aprsis_reconnect, ctx);
	
	if (aprsis_connect(ctx)) {
		g_message("failed to connect");
		return;
	}

	g_message("logging in...");
	aprsis_login(ctx);
	
	g_source_remove (reconnect_timer);
	reconnect_timer = 0;
	
	aprsis_set_filter(ctx, 55, -4, 600);
	//aprsis_set_filter_string(ctx, "p/M/G/2"); // callsigns beginning with G, M or 2 - UK callsigns, normally
	//aprsis_set_filter_string(ctx, "p/HB9"); // Swiss callsigns

	aprsis_io = g_io_channel_unix_new (ctx->sockfd);
    g_io_channel_set_encoding(aprsis_io, NULL, &error);
    if (!g_io_add_watch (aprsis_io, G_IO_IN, aprsis_got_packet, ctx))
        g_error ("Cannot add watch on GIOChannel G_IO_IN");
    
    
    if (!g_io_add_watch (aprsis_io,  G_IO_ERR | G_IO_HUP, aprsis_io_error, ctx))
        g_error ("Cannot add watch on GIOChannel G_IO_IN");

	ctx->state = APRSIS_CONNECTED;
}

void start_aprsis(aprsis_ctx *ctx) {
	// prepare the APRS-IS connection thread

	printf("ctx->state = %d\n", ctx->state);
	if (ctx->state != APRSIS_DISCONNECTED) return;

	// remove the IO channel and watch
	if (aprs1) {
		g_source_remove(aprs1);
		aprs1=0;
	}
	if (reconnect_timer) {
		g_source_remove (reconnect_timer);
		reconnect_timer = 0;
	}
	if (aprsis_io) {
		g_io_channel_unref (aprsis_io);
		aprsis_io = NULL;
	}
	g_thread_create((GThreadFunc) start_aprsis_thread, ctx, FALSE, NULL);
}

/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
