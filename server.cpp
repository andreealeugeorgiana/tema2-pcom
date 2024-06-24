#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <netinet/tcp.h>
#include "helpers.h"
using namespace std;

int main(int argc, char *argv[])
{
    vector<vector<client>> subscribers;
    vector<client> connected;
    vector<string> topics;

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int sockfd_udp, sockfd_tcp, newsockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serv_addr, client_addr;
    int n, i;
    socklen_t client_len = sizeof(struct sockaddr_in);

    fd_set read_fds;
    FD_ZERO(&read_fds);
    fd_set tmp_fds;
    FD_ZERO(&tmp_fds);

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s server_port\n", argv[0]);
        exit(0);
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    DIE(atoi(argv[1]) == 0, "fail atoi");
    serv_addr.sin_port = htons(atoi(argv[1]));
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // socket UDP
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockfd_udp < 0, "fail socket");
    DIE(bind(sockfd_udp, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0, "fail bind");

    // socket TCP
    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd_tcp < 0, "fail socket");
    int flag = 1;
    setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    DIE(bind(sockfd_tcp, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) < 0, "bind");
    DIE(listen(sockfd_tcp, 2000) < 0, "fail listen");

    FD_SET(0, &read_fds);
    FD_SET(sockfd_udp, &read_fds);
    FD_SET(sockfd_tcp, &read_fds);

    int fdmax = max(sockfd_udp, sockfd_tcp);
    client_len = sizeof(client_addr);
    

    while (1) {
        tmp_fds = read_fds;
        DIE(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) < 0, "fail select");
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == sockfd_tcp) {
                    newsockfd = accept(sockfd_tcp, (struct sockaddr *)&client_addr, (socklen_t *)&client_len);
                    DIE(newsockfd < 0, "fail accept");

                    memset(buffer, 0, BUFFER_SIZE);
                    tcp message;
                    DIE(recv(newsockfd, &message, sizeof(message), 0) < 0, "fail recv");
                    bool on = false;

                    for (int i = 0; i < connected.size(); i++)
                        if (strcmp((connected.at(i)).id, message.id) == 0) {
                            tcp message;
                            strcpy(message.content, "exit");
                            printf("Client %s already connected.\n", message.id);
                            sprintf(buffer, "%dEND", (int)strlen(message.content));
                            DIE(send(newsockfd, buffer, strlen(buffer), 0) < 0, "fail send");
                            DIE(send(newsockfd, &message, strlen(message.content) + 1, 0) < 0, "fail send");
                            on = true;
                            break;
                        }
                    if (on == false) {
                        FD_SET(newsockfd, &read_fds);
                        fdmax = max(fdmax, newsockfd);
                        setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
                        client subscriber;
                        printf("New client %s connected from %s:%d.\n", message.id, inet_ntoa(client_addr.sin_addr), ntohs(serv_addr.sin_port));
                        strcpy(subscriber.id, message.id);
                        subscriber.sockfd = newsockfd;
                        connected.push_back(subscriber);
                    }
                } else if (i == sockfd_udp) {
                    udp msg;
                    memset(msg.payload, 0, 1501);

                    memset(buffer, 0, BUFFER_SIZE);
                    DIE(recvfrom(i, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_len) < 0, "fail recv");
                    setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

                    process_buffer(buffer, &msg);
                    memset(buffer, 0, BUFFER_SIZE);
                    format_message(msg, client_addr, buffer);

                    tcp msg_tcp;
                    strcpy(msg_tcp.content, buffer);

                    char buffer_aux[BUFFER_SIZE] = {0};
                    sprintf(buffer_aux, "%dEND", (int)strlen(buffer));
                    for (int i = 0; i < topics.size(); i++)
                        if (topics[i] == msg.topic)
                            for (int j = 0; j < subscribers[i].size(); j++)
                                for (int k = 0; k < connected.size(); k++)
                                    if (connected[k].sockfd == subscribers[i][j].sockfd) {
                                        DIE(send(subscribers[i][j].sockfd, buffer_aux, strlen(buffer_aux), 0) < 0, "fail send");
                                        DIE(send(subscribers[i][j].sockfd, &msg_tcp, strlen(buffer), 0) < 0, "fail send");
                                    }
                } else if (i == 0 && strncmp(fgets(buffer, BUFFER_SIZE - 1, stdin), "exit\n", 5) == 0) {
                        tcp message;
                        strcpy(message.content, "exit");
                        int i = 1;						
                        sprintf(buffer, "%dENDexit\n", (int)(strlen(message.content) + 1));
                        while (i <= fdmax) {
                            if (FD_ISSET(i, &read_fds))
                                if (i != sockfd_tcp && i != sockfd_udp) {
                                    sprintf(buffer, "%dEND", (int)strlen(message.content));
                                    DIE(send(i, buffer, strlen(buffer), 0) < 0, "fail send");
                                    DIE(send(i, &message, strlen(message.content) + 1, 0) < 0, "fail send");
                                }
                            i++;
                        }
                        close(sockfd_tcp);
                        close(sockfd_udp);
                        return 0;
                } else {
                    tcp msg_tcp;
                    DIE(recv(i, &msg_tcp, sizeof(tcp), 0) < 0, "fail recv");
                    char topic[50];
                    char command[11];
                    if (strncmp(msg_tcp.content, "subscribe", SUBSCRIBE) == 0) {

                        sscanf(msg_tcp.content, "%s %s", command, topic);
                        int j = 0;
                        bool found = false;
                        client subscriber;
                        subscriber.sockfd = i;
                        strcpy(subscriber.id, msg_tcp.id);
                        while (j < (int)topics.size()) {
                            if (topics[j] == topic) {
                                subscribers[j].push_back(subscriber);
                                found = true;
                                break;
                            }
                            j++;
                        }
                        if (found == true) {
                            continue;
                        }
                        vector<client> aux;
                        aux.push_back(subscriber);
                        topics.push_back(topic);
                        subscribers.push_back(aux);
                    } else if (strncmp(msg_tcp.content, "exit", EXIT) == 0) {
                        printf("Client %s disconnected.\n", msg_tcp.id);
                        for (int j = 0; j < connected.size(); j++)
                            if (connected[j].sockfd == i)
                                connected.erase(connected.begin() + j);

                        FD_CLR(i, &read_fds);
                    } else
                        continue;
                }
            }
        }
    }

    // inchid socket-ii
    close(sockfd_udp);
    close(sockfd_tcp);
    return 0;
}