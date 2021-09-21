#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <bits/stdc++.h>
using namespace std;

void usage(char *file) {
    fprintf(stderr, "Usage: %s <ID_CLIENT> <IP_Server> <Port_Server>\n", file);
    exit(0);
}

void check_errors(bool condition, const char* description) {
    if (condition) {
        fprintf(stderr,"%s\n", description);
        exit(0);
    }
}
int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd, n, ret, flag = 1;
    tcp_msg_t* recv_msg;
    msg_t sent_msg;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	int fdMax;
	fd_set readFd;
	fd_set tempFd;

	FD_ZERO(&tempFd); // se goleste multimea de descriptori
	FD_ZERO(&readFd);

	if (argc < 4) { // ne asiguram ca avem cate argumente trebuie
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);  // initializez socketul tcp
	DIE(sockfd < 0, "socket");

    FD_SET(sockfd, &readFd);
	fdMax = sockfd;
	FD_SET(0, &readFd);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");



	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "Unnable to connect to the server");

    // dezactivez alg. lui Neagle
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    // trimit ID CLIENT catre server ca mesaj TCP
    n = send(sockfd, argv[1], strlen(argv[1]), 0);
    DIE(n < 0, "send");

	while (1) {
  		tempFd = readFd; 

		ret = select(fdMax + 1, &tempFd, NULL, NULL, NULL);
		DIE(ret < 0, "Unable to select");

		if (FD_ISSET(0, &tempFd)) {
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			if (strcmp(buffer, "exit\n") == 0) {
				close(sockfd);
				return 0;
			}
            // primul token vedem daca e subscribe sau unsubscribe
            char *token = strtok(buffer, " "); 

            if (strncmp(token, "subscribe", 9) == 0) {
                token = strtok(NULL, " ");  // ia acum valoarea topicului
                check_errors(token == NULL, "There is no Topic in the statement");
                strcpy(sent_msg.topic, token);
                
                token = strtok(NULL, " ");  // ia acum valoarea tipului
                check_errors(token == NULL, "There is no SF flag in the statement");
                int value_SF = atoi(token);
                check_errors((value_SF != 0 && value_SF != 1), "Value of SF should be 0 or 1");
                sent_msg.SF = value_SF;
                sent_msg.command_type = 's';

                n = send(sockfd, (char*) &sent_msg, sizeof(sent_msg), 0);
			    DIE(n < 0, "SEND FAILED");

                memset(buffer, 0, BUFLEN); 
                // primeste mesajul de Subribed to topic
                // n = recv(sockfd, buffer, sizeof(buffer), 0);
                // printf("%s\n", buffer);

            } else if (strncmp(token, "unsubscribe", 11) == 0) {
                // unsubscribe
                token = strtok(NULL, " ");  // ia acum valoarea topicului
                check_errors(token == NULL, "There is no Topic in the statement");
                strcpy(sent_msg.topic, token);
                sent_msg.command_type = 'u';

                n = send(sockfd, (char*) &sent_msg, sizeof(sent_msg), 0);
			    DIE(n < 0, "SEND FAILED");
                
                memset(buffer, 0, BUFLEN);
                // primeste mesajul de Unsubscribe from topic
                // n = recv(sockfd, buffer, sizeof(buffer), 0);
                // printf("%s\n", buffer);
            } else {  // daca nu e niciuna dintre comenzile astea, aruncam eroare
                check_errors(true, "Accepted commands: subscribe/unsubscribe, exit\n");
            }
			
		} 
         // s-a primit un mesaj de la server (adica de la un client UDP al acestuia)
        if (FD_ISSET(sockfd, &tempFd)) { 
            memset(buffer, 0, BUFLEN);
            n = recv(sockfd, buffer, sizeof(tcp_msg_t), 0);
            if (n == 0) {
                close(sockfd);
				return 0;    // serverul a inchis conexiunea cu clientul curent
            }
            recv_msg = (tcp_msg_t*)buffer;
            DIE(n < 0, "recv");
            printf("%s:%hu - %s - %s - %s\n", recv_msg->ip, recv_msg->udp_port,
                            recv_msg->topic, recv_msg->type, recv_msg->content);
        }
	}

	close(sockfd);

	return 0;
}
