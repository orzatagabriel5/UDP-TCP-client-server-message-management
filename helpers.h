#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "netinet/tcp.h"


using namespace std;


#ifndef _HELPERS_H
#define _HELPERS_H 1
#define MAX_CLIENTS 500
#define CONTENT_SIZE 1500
#define TOPIC_SIZE 50

#define UNSUBSCRIBE 0
#define SUBSCRIBE 1
#define SEND_ID 2
#define DISCONNECT 3

#define INT 0
#define SHORT_REAL 1 
#define FLOAT 2
#define STRING 3

#define BUFF_SIZE 3200



#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

// #define BUFLEN		256	// dimensiunea maxima a calupului de date
// #define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare

#endif

struct udp_msg {
    char topic[TOPIC_SIZE];
    unsigned char data_type;
    char content[CONTENT_SIZE];
    int port;
    struct in_addr ip_address;
}__attribute__((__packed__));

struct udp_msg_for_tcp {
    char topic[TOPIC_SIZE];
    unsigned char data_type;
    char content[CONTENT_SIZE];
    int port;
    struct in_addr ip_address;
}__attribute__((__packed__));

struct tcp_msg {
    int command_type;
    char id[10];
    int SF;
    char topic[TOPIC_SIZE];
}__attribute__((__packed__));


struct tcp_client {
    int socket_number;
    char id[10];
    int SF;
}__attribute__((__packed__));





