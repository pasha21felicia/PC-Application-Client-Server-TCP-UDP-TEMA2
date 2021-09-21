#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <netinet/in.h>
using namespace std;
/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN		1560	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare


struct __attribute__((packed)) tcp_msg_t {
    char ip[16];
    uint16_t udp_port;
    char topic[51];
    char type[20];
    char content[1501];
};

// structura pentru un mesaj primit de la un client UDP
struct __attribute__((packed)) udp_msg_t {
    char topic[50];
    uint8_t type;
    char content[1501];
};

// structura pentru un mesaj transmis la server de catre un client TCP
struct __attribute__((packed)) msg_t {
    char topic[51];
    int SF;
    char command_type;
};


struct topic_t {
    bool SF;
    char name[51];
    vector <tcp_msg_t> bufferedMessages;
};

struct client_t {
    bool status; //false - dezactivat /true - activ
    char id[11];
    vector <string> topics;
    vector <topic_t> topicsInfo;
};


#endif
