#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include "helpers.h"

#define BUFFER_SIZE 1600

int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    if (argc < 4) {
        fprintf(stderr, "Usage: %s server_address server_port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "fail socket");
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    DIE(inet_aton(argv[2], &serv_addr.sin_addr) == 0, "fail inet_aton");

    DIE(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0, "connect");

    int i, flag = 1;
    DIE(setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0, "setsockopt");

    tcp msg;
    strcpy(msg.id, argv[1]);
    strcpy(msg.content, argv[1]);
    DIE(send(sockfd, &msg, sizeof(msg), 0) < 0, "fail send");

    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    int fdmax = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
    while (1) {
        fd_set tmp_fds = read_fds;
        DIE(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) < 0, "fail select");
        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            msg = {0};

            fgets(msg.content, BUFFER_SIZE - 50, stdin);
            strcpy(msg.id, argv[1]);

            if (strncmp(msg.content, "subscribe", SUBSCRIBE) == 0) {
                printf("Subscribed to topic.\n");
            }
            else if (strncmp(msg.content, "unsubscribe", UNSUBSCRIBE) == 0) {
                printf("Unsubscribed from topic.\n");
            }
            else if (strncmp(msg.content, "exit", EXIT) == 0) {
                DIE(send(sockfd, &msg, sizeof(tcp), 0) < 0, "fail send");
                close(sockfd);
                break;
            }

            DIE(send(sockfd, &msg, sizeof(tcp), 0) < 0, "fail send");
        } else if (FD_ISSET(sockfd, &tmp_fds)) {
            msg = {0};
            for (i = 0; recv(sockfd, &buffer[i], 1, 0) > 0; i++) {
                if ('E' == buffer[i - 2] && 'N' == buffer[i - 1] && 'D' == buffer[i])
                    break;
            }
            if (recv == 0) {
                close(sockfd);
                return 0;
            }
            i = i - 2;
            buffer[i] = '\0';
            int dim = atoi(buffer);
            int dim1 = 0;
            int ret;
            do {
                memset(buffer, 0, BUFFER_SIZE);
                ret = recv(sockfd, buffer, dim, 0);
                DIE(ret < 0, "fail recv");

                if (strncmp(buffer, "exit", 4) == 0) {
                    close(sockfd);
                    return 0;
                }
                dim1 = dim1 + ret;
                printf("%s", buffer);
            } while (ret > 0 && dim != dim1);
        }
    }
    close(sockfd);
    return 0;
}
