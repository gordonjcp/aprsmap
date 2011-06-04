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
#include "station.h"
#include "mapviewer.h"

static gboolean reconnect;
static guint aprs1;
static GIOChannel *aprsis_io;

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

int aprsis_set_log(aprsis_ctx *ctx, FILE *file) {
	ctx->log_file = file;
}

int aprsis_read(aprsis_ctx *ctx, char *buf, size_t len) {
	int count = read(ctx->sockfd, buf, len);

	if (ctx->log_file != NULL) {
		printf("Logging read\n");
		fprintf(ctx->log_file, "< %s\n", buf);
	}

	return count;
}

int aprsis_write(aprsis_ctx *ctx, char *buf, size_t len) {
	int count = write(ctx->sockfd, buf, len);

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
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	//hints.ai_protocol = IPPROTO_TCP;

	char ipstr[INET6_ADDRSTRLEN];

	// get a list of addresses
	err = getaddrinfo(ctx->host, ctx->port, &hints, &res);
	if (err != 0)   {
		g_error("error in getaddrinfo: %s", gai_strerror(err));
		return 1;
	} 


	// loop down the list, and try to connect
	for (; res != NULL ; res = res->ai_next) {
		// get the name
	    char hostname[NI_MAXHOST] = "";
		void *addr;
		void *ipver;
		err = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0); 
		if (err != 0) {
			g_error("error in getnameinfo: %s", gai_strerror(err));
			continue;
		}
		// MOAR DEBUG
		if (res->ai_family == AF_INET) {
		    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		} else { 
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv6";
		}
		inet_ntop(res->ai_family, addr, ipstr, sizeof ipstr);
		g_message("trying: %s (%s) over %s", hostname, ipstr, ipver);

		// set up a socket, and attempt to connect
		ctx->sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		err = connect(ctx->sockfd, res->ai_addr, res->ai_addrlen);
		if (err != 0) {
			g_message("can't connect to %s - %s",hostname, strerror(errno));
			continue;
		}
		break;
	};
	freeaddrinfo(res);
	// FIXME make errors work properly, this is ugly
	
	/* No idea if this is the right thing to do, but here goes...
	if (!err) {
		return 0;
	} else {
		return -1;
	} */
	return (err);
}

int aprsis_login(aprsis_ctx *ctx) {
	// wait for prompt, send filter message
	char buf[256];
	int n;


	// note that this doesn't *actually* check what the prompt is
	n = aprsis_read(ctx, buf, 256);
	if (n<0) {
		error("couldn't read from socket");
	}
	g_message("got: %s",buf);

	sprintf(buf, APRSIS_LOGIN"\n", ctx->user, ctx->pass);
	g_message("sending: %s", buf);
	aprsis_write(ctx, buf, strlen(buf));
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

	if (ctx->sockfd != -1) {
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
		g_error ("Read end of pipe died!");   // FIXME - handle this more gracefully

	if (condition & G_IO_ERR) {
		g_message ("IO error");
		return FALSE;
	}
		
	ret = g_io_channel_read_line (gio, &msg, &len, NULL, &err);
	if (ret == G_IO_STATUS_ERROR)  g_message("Error reading: %s", err->message);
	if (ret == G_IO_STATUS_EOF) {
		g_message("EOF (server disconnected)");
		return FALSE; // shut down the callback, for now 
	}
	
	aprsis_write_log(ctx, msg, len);
	
	if (msg[0] == '#') {
		printf("can ignore comment message: %s\n", msg);
	} else {
		printf ("\n------------------------------------------\nRead %u bytes: %s\n", (unsigned int) len, msg);
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
    if (!g_io_add_watch (aprsis_io, G_IO_IN | G_IO_ERR | G_IO_HUP, aprsis_got_packet, ctx))
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
