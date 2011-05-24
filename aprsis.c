// network.c
// connect to APRS-IS server
// Copyright 2011 Gordonjcp MM0YEQ
// GPL V3 applies


#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include <sys/socket.h>
#include <string.h>
#include <netdb.h> 
#include <errno.h>

#include "aprsis.h"
#include "mapviewer.h"

static gboolean reconnect;
static guint aprs1;
GIOChannel *aprsis_io;

aprsis_ctx *aprsis_new(const char *host, const char *port, const char *user, const char *pass) {
	aprsis_ctx *ctx = calloc(1, sizeof(aprsis_ctx));

	ctx->sockfd = -1;
	ctx->host = strdup(host);
	ctx->port = strdup(port);
	ctx->user = strdup(user);
	ctx->pass = strdup(pass);

	return ctx;
}

int aprsis_connect(aprsis_ctx *ctx) {
	// connect to an APRS-IS server
	// return 0 on success
	
	struct addrinfo server;
	gint err;

	// somewhere to put the result of the lookup
	struct addrinfo *res;
	struct addrinfo hints;

	// clear off any hints, set up for TCP/IPv4
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// get a list of addresses
	err = getaddrinfo(ctx->host, ctx->port, &hints, &res);
	if (err != 0)   {
		g_error("error in getaddrinfo: %s\n", gai_strerror(err));
		return 1;
	} 

	char hostname[NI_MAXHOST] = "";

	// loop down the list, and try to connect
	for (; res = res->ai_next; res != NULL) {
		// get the name
		err = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0); 
		if (err) {
			g_error("error in getnameinfo: %s", gai_strerror(err));
		}

		// set up a socket, and attempt to connect
		g_message("trying hostname: %s %d %d", hostname, res->ai_socktype, res->ai_protocol);
		ctx->sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		err = connect(ctx->sockfd, res->ai_addr, res->ai_addrlen);
		if (err < 0) {
			g_message("can't connect - %s",strerror(errno));
		}
	};
	free(res);
	// FIXME make errors work properly, this is ugly
	if (!err) {
		return 0;
	} else {
		return -1;
	}
}

int aprsis_login(aprsis_ctx *ctx) {
	// wait for prompt, send filter message
	char buf[256];
	int n;


	// note that this doesn't *actually* check what the prompt is
	n = read(ctx->sockfd, buf, 256);
	if (n<0) {
		error("couldn't read from socket");
	}
	g_message("got\t%s",buf);

	sprintf(buf, APRSIS_LOGIN"\n", ctx->user, ctx->pass);
	g_message("sending\t%s", buf);
	write(ctx->sockfd, buf, strlen(buf));
	n = read(ctx->sockfd, buf, 256);
	if (n<0) {
		error("couldn't read from socket");
	}
	g_message("got\t%s",buf);
	
	return 0;
}

void aprsis_set_filter(aprsis_ctx *ctx, double latitude, double longitude, int radius) {
	// sets a filter given latitude, longitude and radius
	ctx->latitude = latitude;
	ctx->longitude = longitude;
	ctx->radius = radius;

	if (ctx->sockfd != -1) {
		char buf[64];
		snprintf(buf, sizeof(buf), "#filter r/%.0f/%.0f/%d\n", latitude, longitude, radius);
		g_message("Sending filter: %s", buf);
		write(ctx->sockfd, buf, strlen(buf));
	}
}

void aprsis_set_filter_string(aprsis_ctx *ctx, char *filter) {
	// send a filter string, for more complex filters
	if (ctx->sockfd != -1) {
		char buf[64];
		snprintf(buf, sizeof(buf), "#filter %s\n", filter);
		g_message("Sending filter: %s", buf);
		write(ctx->sockfd, buf, strlen(buf));
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

	if (condition & G_IO_HUP)
		g_error ("Read end of pipe died!");   // FIXME - handle this more gracefully

	if (condition & G_IO_ERR) {
		g_message ("IO error");
		return FALSE;
	}
		
	ret = g_io_channel_read_line (gio, &msg, &len, NULL, &err);
	if (ret == G_IO_STATUS_ERROR)  g_message("Error reading: %s\n", err->message);
	if (ret == G_IO_STATUS_EOF) {
		g_message("EOF (server disconnected)\n");
		return FALSE; // shut down the callback, for now 
	}
	
	
	if (msg[0] == '#') {
		printf("can ignore comment message: %s\n", msg);
	} else {
		printf ("\n------------------------------------------\nRead %u bytes: %s", len, msg);
		process_packet(msg);
	}

	g_free(msg);
	return TRUE;
}


static void *start_aprsis_thread(void *ptr) {
    GError *error = NULL;
	aprsis_ctx *ctx = ptr;
	
	g_message("connecting to %s", ctx->host);
	if (aprsis_connect(ctx)) {
		g_error("failed to connect");
	}

	g_message("logging in...");
	aprsis_login(ctx);
	aprsis_set_filter(ctx, 55, -4, 600);
	//aprsis_set_filter_string(ctx, "p/M/G/2"); // callsigns beginning with G, M or 2 - UK callsigns, normally
	//aprsis_set_filter_string(ctx, "p/HB9"); // Swiss callsigns

	aprsis_io = g_io_channel_unix_new (ctx->sockfd);
    g_io_channel_set_encoding(aprsis_io, NULL, &error);
    if (!g_io_add_watch (aprsis_io, G_IO_IN | G_IO_ERR | G_IO_HUP, aprsis_got_packet, NULL))
        g_error ("Cannot add watch on GIOChannel!");
}

void start_aprsis(aprsis_ctx *ctx) {
	// prepare the APRS-IS connection thread
	reconnect = FALSE;  // don't keep trying if we've already tried to start one
	// FIXME I should finish the "don't start two threads" code
	// remove the IO channel and watch
	if (aprs1) {
		g_source_remove(aprs1);
		aprs1=0;
	}
	if (aprsis_io) {
		g_io_channel_unref (aprsis_io);
		aprsis_io = NULL;
	}
	g_thread_create(&start_aprsis_thread, ctx, FALSE, NULL);
}

/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
