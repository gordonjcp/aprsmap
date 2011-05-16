// network.c
// connect to APRS-IS server
// Copyright 2011 Gordonjcp MM0YEQ
// GPL V3 applies

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h> 
#include <errno.h>

#include "aprsis.h"

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
	struct addrinfo server;
	// FIXME grim hardcoded values
	//const char *host = "england.aprs2.net";
	//const char *port = "10152";
	int err;

	// somewhere to put the result of the lookup
	struct addrinfo *res;
	struct addrinfo hints;

	// clear off any hints, set up for TCP/IPv4
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// get a list of addresses
	err = getaddrinfo(ctx->host, ctx->port, NULL, &res);
	if (err != 0)   {
		printf("error in getaddrinfo: %s\n", gai_strerror(err));
		return 1;
	}

	// loop down the list, and try to connect
	do {
		char hostname[NI_MAXHOST] = "";
		// get the name, we don't really need this
		err = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0); 
		if (err) {
			printf("error in getnameinfo: %s\n", gai_strerror(err));
		}
		printf("trying hostname: %s\n", hostname);
		
		// set up a socket, and attempt to connect
		//sockfd = socket(AF_INET, SOCK_STREAM, 0);
		ctx->sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		err = connect(ctx->sockfd, res->ai_addr, res->ai_addrlen);
		if (err < 0) {
			printf("can't connect - %s\n",strerror(errno));
			res = res->ai_next;
		}
	} while (err);
	
	return 1;
	/*
	// crappy test code
	    char buf[256];
	    int n;

	aprsis_login(sockfd);

	while(1) {
		memset(&buf, 0, 256);
	  n = read(sockfd, buf, 256);
    if (n < 0) 
      error("ERROR reading from socket");
    printf("%s", buf);
	}
*/
}

int aprsis_login(aprsis_ctx *ctx) {
	// wait for prompt, send filter message
	char buf[256];
	int n;
	
	n = read(ctx->sockfd, buf, 256);
	if (n<0) {
		error("couldn't read from socket");
	}
	// FIXME crappy hardcoded string
	sprintf(buf, APRSIS_LOGIN, ctx->user, ctx->pass);
	write(ctx->sockfd, buf, strlen(buf));

	if (ctx->radius != 0) {
		memset(buf, '\0', sizeof(buf));
		snprintf(buf, sizeof(buf), " filter r/%.0f/%.0f/%d", ctx->latitude, ctx->longitude, ctx->radius);
		write(ctx->sockfd, buf, strlen(buf));
	}

	write(ctx->sockfd, "\n", 1);
	return 0;
}

void aprsis_set_filter(aprsis_ctx *ctx, double latitude, double longitude, int radius) {

	ctx->latitude = latitude;
	ctx->longitude = longitude;
	ctx->radius = radius;

	if (ctx->sockfd != -1) {
		char buf[64];
		snprintf(buf, sizeof(buf), "filter r/%.0f/%.0f/%d\n", latitude, longitude, radius);
		printf("\nSending filter: %s\n", buf);
		write(ctx->sockfd, buf, strlen(buf));
	}
}

void aprsis_close(aprsis_ctx *ctx) {
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
}

/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
