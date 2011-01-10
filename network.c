// network.c
// connect to APRS-IS server
// Copyright 2011 Gordonjcp MM0YEQ
// GPL V3 applies

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h> 
#include <errno.h>


static int sockfd;

int aprsis_connect() {
	struct addrinfo server;
	// FIXME grim hardcoded values
	const char *host = "rotate.aprs2.net";
	const char *port = "10152";
	int err;

	// somewhere to put the result of the lookup
	struct addrinfo *res;
	struct addrinfo hints;

	// clear off any hints, set up for TCP/IPv4
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	// get a list of addresses
	err = getaddrinfo(host, port, NULL, &res);
	if (err != 0)   {
		printf("error in getaddrinfo: %s\n", gai_strerror(err));
		return EXIT_FAILURE;
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
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		err = connect(sockfd, res->ai_addr, res->ai_addrlen);
		if (err < 0) {
			printf("can't connect - %s\n",strerror(errno));
			res = res->ai_next;
		}
	} while (err);
	    char buf[256];
	    int n;

	sprintf(buf, "user mm0yeq pass -1 vers aprsmap 0.0 filter r/55/-4/600\n");
	write(sockfd, buf, 256);
	while(1) {
	
	  n = read(sockfd, buf, 256);
    if (n < 0) 
      error("ERROR reading from socket");
    printf("Echo from server: %s", buf);
	}
}



/* vim: set noexpandtab ai ts=4 sw=4 tw=4: */
