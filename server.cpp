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

void usage(char *file)
{
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}
// afisarea mesajelor de eroare
void check_errors(bool condition, const char *description)
{
    if (condition)
    {
        fprintf(stderr, "%s\n", description);
        exit(0);
    }
}
// parseaza structura udp si o transforma in tcp dupa specificatiile din enunt
void create_tcp_message_from_udp(udp_msg_t *received_from_udp, tcp_msg_t *send_to_tcp)
{
    strncpy(send_to_tcp->topic, received_from_udp->topic, 50);
    send_to_tcp->topic[50] = 0;

    long long int_value;
    double real_value;
    float float_value;

    switch (received_from_udp->type)
    {
    case 0: // s-a primit un INT
        strncpy(send_to_tcp->type, "INT", 4);
        int_value = htonl(*(uint32_t *)(received_from_udp->content + 1));
        if (received_from_udp->content[0])
            int_value *= -1;
        sprintf(send_to_tcp->content, "%lld", int_value);
        break;
    case 1: // s-a primit un SHORT REAL
        strncpy(send_to_tcp->type, "SHORT_REAL", 11);
        real_value = ntohs(*(uint16_t *)(received_from_udp->content));
        real_value /= 100;
        sprintf(send_to_tcp->content, "%.2f", real_value);
        break;
    case 2:
        // s-a primit un FLOAT
        strncpy(send_to_tcp->type, "FLOAT", 6);
        float_value = htonl(*(uint32_t *)(received_from_udp->content + 1));
        float_value /= pow(10, received_from_udp->content[5]);
        if (received_from_udp->content[0])
            float_value *= -1;
        sprintf(send_to_tcp->content, "%lf", float_value);
        break;

    default:
        // s-a primit un STRING
        strcpy(send_to_tcp->type, "STRING");
        strcpy(send_to_tcp->content, received_from_udp->content);
        break;
    }
}
int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int sock_tcp, new_sock_tcp, port_num, sock_udp;
    int fdmax; // valoare maxima fd din multimea read_fds
    int flagDelay = 1;
    int n, i, ret;

    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr, udp_addr;
    socklen_t clilen;

    client_t curr_client;

    fd_set read_fds; // multimea de citire folosita in select()
    fd_set tmp_fds;  // multime folosita temporar

    udp_msg_t *udp_msg;
    tcp_msg_t tcp_msg;
    msg_t *serv_msg;

    if (argc < 2)
    {
        usage(argv[0]);
    }

    // se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    sock_udp = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(sock_udp < 0, "Unable to create UDP socket");

    sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sock_tcp < 0, "Unable to create TCP socket");

    port_num = atoi(argv[1]);
    DIE(port_num < 1024, "Incorrect port number");

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_num);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    memset((char *)&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(port_num);
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sock_tcp, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "Unable to bind TCP socket");

    ret = bind(sock_udp, (struct sockaddr *)&udp_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "Unable to bind UDP socket");

    ret = listen(sock_tcp, MAX_CLIENTS);
    DIE(ret < 0, "Unable to listen on the TCP socket");

    // se adauga noul socket de pe care se asculta conexiuni in multimea read_fds
    FD_SET(sock_tcp, &read_fds);
    FD_SET(sock_udp, &read_fds);
    FD_SET(0, &read_fds);
    fdmax = sock_tcp;

    // dezactivam algoritumul lui Neagle
    setsockopt(sock_tcp, IPPROTO_TCP, TCP_NODELAY, &flagDelay, sizeof(int));

    unordered_map<int, client_t> activeClients;
    unordered_map<string, client_t> allClients;

    while (1)
    {
        tmp_fds = read_fds;
        memset(buffer, 0, BUFLEN);
        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "Unable to select");

        for (i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &tmp_fds))
            {
                if (i == 0)
                { // se primeste o comanda de la stdin
                    fgets(buffer, BUFLEN - 1, stdin);
                    if (strncmp(buffer, "exit\n", 5) == 0)
                    {
                        for (i = 0; i <= fdmax; i++)
                        {
                            close(i);
                            close(sock_udp);
                            close(sock_tcp);
                        }
                        return 0;
                    }
                    else
                    {
                        printf("The accepted command is 'exit'\n");
                    }
                }
                else if (i == sock_udp)
                {
                    char *buffer_udp = (char *)malloc(BUFLEN * sizeof(char));
                    socklen_t size_udp = sizeof(struct sockaddr_in);
                    memset(buffer_udp, 0, BUFLEN);
                    ret = recvfrom(sock_udp, buffer_udp, BUFLEN, 0,
                                   (struct sockaddr *)&udp_addr, &size_udp);
                    DIE(ret < 0, "Nothing received from UDP socket");

                    tcp_msg.udp_port = htons(udp_addr.sin_port);
                    strcpy(tcp_msg.ip, inet_ntoa(udp_addr.sin_addr));

                    udp_msg = (udp_msg_t *)buffer_udp;
                    check_errors(udp_msg->type > 3, "Data Type should be between [0..3]");
                    create_tcp_message_from_udp(udp_msg, &tcp_msg);

                    // adaug mesajele fostilor clienti cu sf=1 in buffered messages a fiecarui client
                    for (auto &client : allClients)
                    {
                        if (client.second.status == false)
                        {
                            for (unsigned int j = 0; j < client.second.topicsInfo.size(); j++)
                            {
                                if (client.second.topicsInfo[j].SF == 1 &&
                                    strcmp(client.second.topicsInfo[j].name, tcp_msg.topic) == 0)
                                {
                                    client.second.topicsInfo[j].bufferedMessages.push_back(tcp_msg);
                                }
                            }
                        }
                    }
                    // transmit mesajul tuturor clientilor activi abonat la topic
                    for (auto client : activeClients)
                    {
                        vector<string>::iterator it =
                            find(client.second.topics.begin(), client.second.topics.end(), tcp_msg.topic);
                        if (it != client.second.topics.end())
                        {
                            n = send(client.first, (char *)&tcp_msg, sizeof(tcp_msg_t), 0);
                            DIE(n < 0, "send");
                        }
                    }
                }
                else if (i == sock_tcp)
                {
                    // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
                    // pe care serverul o accepta
                    clilen = sizeof(cli_addr);
                    new_sock_tcp = accept(sock_tcp, (struct sockaddr *)&cli_addr, &clilen);
                    DIE(new_sock_tcp < 0, "accept");

                    setsockopt(new_sock_tcp, IPPROTO_TCP, TCP_NODELAY, (char *)&flagDelay, sizeof(int));
                    // se adauga noul socket intors de accept() la multimea descriptorilor de citire
                    FD_SET(new_sock_tcp, &read_fds);

                    n = recv(new_sock_tcp, buffer, BUFLEN, 0);
                    DIE(n < 0, "No client ID received");

                    bool different_clients = true;
                    bool previously_connected = false;

                    // verific daca clientul cu acest id se afla in lista de clienti conectati acum
                    for (auto client : activeClients)
                    {
                        if (strcmp(client.second.id, buffer) == 0)
                        {
                            printf("Client %s already connected.\n", buffer);
                            different_clients = false;
                            close(new_sock_tcp);
                        }
                    }
                    // daca clientul e valid
                    if (different_clients)
                    {
                        if (new_sock_tcp > fdmax)
                        {
                            fdmax = new_sock_tcp;
                        }
                        strcpy(curr_client.id, buffer);
                        curr_client.topics.clear();
                        curr_client.status = true;

                        // clientul a mai fost conectat inainte cu acelasi id
                        if (allClients.find(buffer) != allClients.end())
                        {
                            previously_connected = true;

                            // adaug in topicurile clientului curent topicurile la care a fost abonat inainte
                            // de dezactivare
                            curr_client.topics.insert(curr_client.topics.end(),
                                                      allClients[buffer].topics.begin(),
                                                      allClients[buffer].topics.end());
                            allClients[buffer].status = true;

                            // trimit mesajele bufferate catre clientul reconectat
                            for (auto topic : allClients[buffer].topicsInfo)
                            {
                                if (!topic.bufferedMessages.empty())
                                {
                                    for (auto message : topic.bufferedMessages)
                                    {
                                        n = send(new_sock_tcp, (char *)&message, sizeof(tcp_msg_t), 0);
                                        DIE(n < 0, "send");
                                    }
                                }
                            }
                        }

                        activeClients.insert({new_sock_tcp, curr_client});
                        // creez lista tuturor clientilor conectati vreodata in aplicatie
                        if (!previously_connected)
                        {
                            allClients.insert({curr_client.id, curr_client});
                        }

                        printf("New client %s connected from %s:%hu.\n", curr_client.id,
                               inet_ntoa(cli_addr.sin_addr), htons(cli_addr.sin_port));
                    }
                }
                else
                {
                    // s-au primit date pe unul din socketii de client,
                    // asa ca serverul trebuie sa le receptioneze
                    memset(buffer, 0, BUFLEN);

                    n = recv(i, buffer, sizeof(buffer), 0);
                    DIE(n < 0, "recv");

                    if (n == 0)
                    {
                        // conexiunea s-a inchis
                        printf("Client %s disconnected.\n", activeClients[i].id);
                        FD_CLR(i, &read_fds);
                        allClients[activeClients[i].id].status = false;
                        activeClients.erase(i);
                        close(i);
                    }
                    else
                    {   // primim date valide de la subscriber
                        // facem subscribe la topic
                        serv_msg = (msg_t *)buffer;
                        if (serv_msg->command_type == 's')
                        {
                            char mesaj[] = "Subscribed to topic.";
                            n = send(i, mesaj, strlen(mesaj), 0);
                            // adaugam topicul in lista de topicuri string si topicsInfo
                            // atat a clientului din lista de clienti activi cat si in cea cu toti clientii
                            if (activeClients.find(i) != activeClients.end())
                            {
                                activeClients[i].topics.push_back(serv_msg->topic);
                                topic_t newTopic;
                                strcpy(newTopic.name, serv_msg->topic);
                                newTopic.SF = serv_msg->SF;
                                activeClients[i].topicsInfo.push_back(newTopic);
                                allClients[activeClients[i].id].topics.push_back(serv_msg->topic);
                                allClients[activeClients[i].id].topicsInfo.push_back(newTopic);
                            }
                        }
                        else if (serv_msg->command_type == 'u')
                        { // facem unsubscribe la topic
                            char mesaj[] = "Unsubscribed from topic.";
                            n = send(i, mesaj, strlen(mesaj), 0);

                            if (activeClients.find(i) != activeClients.end())
                            {
                                // stergem topicul din vectorul de stringuri topics din activeClients
                                for (unsigned int j = 0; j < activeClients[i].topics.size(); j++)
                                {
                                    for (unsigned int j = 0; j < activeClients[i].topics.size(); ++j)
                                    {
                                        if (string(serv_msg->topic).compare(activeClients[i].topics[j] + "\n") == 0)
                                        {
                                            activeClients[i].topics.erase(activeClients[i].topics.begin() + j);
                                            j--;
                                        }
                                    }
                                }
                                // stergem topicul din vectorul de topicuri topicsInfo din activeClients
                                for (unsigned int j = 0; j < activeClients[i].topicsInfo.size(); j++)
                                {
                                    for (unsigned int j = 0; j < activeClients[i].topicsInfo.size(); ++j)
                                    {
                                        if (string(serv_msg->topic).compare(string(activeClients[i].topicsInfo[j].name) + "\n") == 0)
                                        {
                                            activeClients[i].topicsInfo.erase(activeClients[i].topicsInfo.begin() + j);
                                            j--;
                                        }
                                    }
                                }
                                // stergem topicul din vectorul de stringuri topics din allClients
                                for (unsigned int j = 0; j < allClients[activeClients[i].id].topics.size(); j++)
                                {
                                    for (unsigned int j = 0; j < allClients[activeClients[i].id].topics.size(); ++j)
                                    {
                                        if (string(serv_msg->topic).compare(allClients[activeClients[i].id].topics[j] + "\n") == 0)
                                        {
                                            allClients[activeClients[i].id].topics.erase(allClients[activeClients[i].id].topics.begin() + j);
                                            j--;
                                        }
                                    }
                                }
                                // stergem topicul din vectorul de topicuri topicsInfo din allClients
                                for (unsigned int j = 0; j < allClients[activeClients[i].id].topicsInfo.size(); j++)
                                {
                                    for (unsigned int j = 0; j < allClients[activeClients[i].id].topicsInfo.size(); ++j)
                                    {
                                        if (string(serv_msg->topic).compare(string(allClients[activeClients[i].id].topicsInfo[j].name) + "\n") == 0)
                                        {
                                            allClients[activeClients[i].id].topicsInfo.erase(allClients[activeClients[i].id].topicsInfo.begin() + j);
                                            j--;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    close(sock_tcp);
    close(sock_udp);

    return 0;
}
