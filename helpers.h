#define BUFFER_SIZE 1600
#define SUBSCRIBE 9
#define UNSUBSCRIBE 11
#define EXIT 4


typedef struct
{
	char content[1550];
	char id[10];
} tcp;

typedef struct
{
	char topic[51];
	char data_type;
	char payload[1501];
} udp;

typedef struct
{
	char id[10];
	int sockfd;
} client;

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#include <stdint.h>

void process_buffer(char buffer[BUFFER_SIZE], udp *msg) {
    switch (buffer[50]) {
        case 0: {
            msg->data_type = 0;
            uint32_t num;
            memcpy(&num, &(buffer[52]), sizeof(uint32_t));
            memcpy(msg->topic, buffer, 50);

            if (buffer[51] == 1)
                sprintf(msg->payload, "%d", -ntohl(num));
            else
                sprintf(msg->payload, "%d", ntohl(num));

            msg->topic[50] = '\0';
            break;
        }
        case 1: {
            msg->data_type = 1;
            uint16_t num;
            memcpy(&num, &(buffer[51]), sizeof(uint16_t));
            memcpy(msg->topic, buffer, 50);
            num = ntohs(num);
            double num_double = 1.0 * num / 100;
            sprintf(msg->payload, "%.2f", num_double);
            msg->topic[50] = '\0';
            break;
        }
        case 2: {
            msg->data_type = 2;
            uint8_t power = (*(uint8_t *)(&buffer[52] + sizeof(uint32_t)));

            double num = ntohl(*(uint32_t *)&buffer[52]);
            for (int i = 0; i < (int)power; i++)
                num = num / 10;

            memcpy(msg->topic, buffer, 50);
            if (buffer[51] == 1)
                sprintf(msg->payload, "-%f", num);
            else
                sprintf(msg->payload, "%f", num);

            msg->topic[50] = '\0';
            break;
        }
        case 3: {
            msg->data_type = 3;
            memcpy(msg->topic, buffer, 50);
            memcpy(msg->payload, &buffer[51], 1500);
            msg->topic[50] = '\0';
            break;
        }
    }
}



void format_message(udp msg, struct sockaddr_in client_addr, char* buffer) {
	char* type;
    switch(msg.data_type) {
        case 0:
			type = "INT";
            sprintf(buffer, "%s:%d - %s - %s - %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg.topic, type, msg.payload);
            break;
        case 1:
			type = "SHORT_REAL";
            sprintf(buffer, "%s:%d - %s - %s - %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg.topic, type, msg.payload);
            break;
        case 2:
			type = "FLOAT";
            sprintf(buffer, "%s:%d - %s - %s - %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg.topic, type, msg.payload);
            break;
        case 3:
			type = "STRING";
            sprintf(buffer, "%s:%d - %s - %s - %s\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msg.topic, type, msg.payload);
            break;
        default:
            sprintf(buffer, "Unknown message type\n");
    }
}