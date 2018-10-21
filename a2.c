#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
/* According to earlier standards */
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>


#include "a2.h"
#include "queue.h"

#define MAX_BUFFER_SIZE 10
#define WINDOW_SIZE 4
#define PORT 5555
#define ACK "ACK"
#define GOOD_BYE "GOOD_BYE"
#define TIMEOUT 1000


/*
* Global variables
*
*/

uint8_t head_sequence_number;
bool go_back_n = true;
sem_t send_data;
sem_t timer_start;
bool m_run_timer_thread = true;

void set_non_blocking(int *fd){

    int flags = fcntl(*fd, F_GETFL, 0);
    fcntl(*fd, F_SETFL, flags | O_NONBLOCK);
}

void* start_timer(void *thread_data) {

    int sockfd = *(int *)thread_data;
    uint8_t expected_sequence = 1;
    int retval;
    fd_set readset;
    struct timeval tv;
    
    char data[7];

    set_non_blocking(&sockfd);

    while(m_run_timer_thread){    
        sem_wait(&timer_start);
        int end;
        int start;

        start = clock() * 1000 / CLOCKS_PER_SEC;

        do{
            retval = read(sockfd,data, sizeof(data)); 
            if(retval != EAGAIN){
                char ack[7] = {'A', 'C', 'K'};
                strcat(ack, &expected_sequence);
                if(strcmp(data, ack) == 0){
                    go_back_n = false;
                    expected_sequence ++;
                    break;
                }
            }

            end = clock() * 1000 / CLOCKS_PER_SEC;

        }while((end - start) < TIMEOUT);

        if((end - start) > TIMEOUT){
            go_back_n = true;
        }

        sem_post(&send_data);
    }
    
}

void start_server(cfg_t *config)
{

    int server_fd;
    int len, n;
    int err = 0;
    uint8_t packet[MAX_BUFFER_SIZE + 1];
    unsigned char ip[sizeof(struct in_addr)];
    char buff[MAX_BUFFER_SIZE];
    uint8_t expected_seq = 0;

    struct sockaddr_in server_addr, client_addr;
    config->server = "127.0.0.1";

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, config->server, &(server_addr.sin_addr.s_addr));
    server_addr.sin_port = htons(config->port);

    len = sizeof(client_addr);

    do
    {

        // create sockets

        if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {

            fprintf(stderr, "socket creation failed\n");
            err = -1;
            break;
        }

        //bind the socket
        if (bind(server_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            fprintf(stderr, "socket bind failed\n");
            err = -1;
            break;
        }

        // recieve from the server

        while ((n = recvfrom(server_fd, packet, sizeof(packet), 0, (struct sockaddr *)&client_addr, &len)) > 0)
        {
            // write to the destination file
            printf("recived data %s\n", packet);
            if (strcmp(packet, GOOD_BYE) == 0)
            {
                fprintf(stdout, "got good bye from the client... Server shutting down\n");
                break;
            }

            // check  if the seq number is same as expected then
            // write to the buffer and send ack.
            uint8_t seq = packet[0] - '0';
            printf("seq %d expected_seq %d\n", seq, expected_seq);
            if (seq == expected_seq)
            {
                memcpy(buff, &packet[1], n - 1);
                fwrite(buff, 1, n - 1, config->fstream);
                memset(packet, 0, sizeof(buff));
                memset(buff, 0, sizeof(buff));
                char ack_buffer[5] = {'A', 'C', 'K'};
                ack_buffer[3] = 48 + seq;
                printf("\nACK sending.... %s\n", ack_buffer);
                sendto(server_fd, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
                expected_seq = seq ^ 1;
            }
            else
            {
                char ack_buffer[5] = {'A', 'C', 'K'};
                ack_buffer[3] = 48 + seq;
                printf("\n Server sendning ACK .....%s\n", ack_buffer);
                sendto(server_fd, ack_buffer, sizeof(ack_buffer), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            }
        }

        if (n == 0)
        {
            fprintf(stderr, "Client sending an empty message... Server shutting down\n");
            break;
        }

        if (n < 0)
        {
            fprintf(stderr, "Error recieving from the client %d error num %d\n", n, errno);
            break;
        }

    } while (err != 0);

    // close sockets
    close(server_fd);
}

void start_client(cfg_t *config)
{

    int client_fd;
    int n;
    int err = 0;
    uint8_t *packet = (uint8_t*)malloc(MAX_BUFFER_SIZE + 1);
    struct Queue *packets = createQueue();
    uint8_t seq = 0;
    size_t nread;
    pthread_t timer_thread;

    struct sockaddr_in server_addr, client_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    sem_init(&timer_start, 0, 0);
    sem_init(&send_data, 0, 0);

    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, config->server, &(server_addr.sin_addr));
    server_addr.sin_port = htons(config->port);

    do
    {

        // create sockets

        if ((client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            fprintf(stderr, "socket creation failed");
            err = -1;
            break;
        }

        pthread_create(&timer_thread, NULL, &start_timer,(void *)&client_fd);

        printf("Timer thread created successfully\n");

        if (config->fstream != NULL){

            //first simple read and enque packets
            sprintf(packet, "%d", 1);
            head_sequence_number = 0;
            while(1){
            
                /*
                * Intial packet transfer
                */
                  
                do{
                    memset(packet,0,sizeof(packet));
                    head_sequence_number++;
                    sprintf(packet, "%d", head_sequence_number);
                    nread = fread(packet + strlen(packet), 1, sizeof(packet) -2 , config->fstream);
                    packet[MAX_BUFFER_SIZE] = '\0';  
                    if(nread < 0){
                        //send good bye to server
                        printf("Sending Good_bye !!!\n");
                        sendto(client_fd, "GOOD_BYE", strlen("GOOD_BYE"), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
                        close(client_fd);
                        go_back_n = false;
                        break;
                    }
                    sendto(client_fd, packet, sizeof(packet), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
                    enQueue(packets,packet);
                    if(head_sequence_number == 1 || !go_back_n){
                        
                        sem_post(&timer_start);
                    }
                }while(head_sequence_number < WINDOW_SIZE);
                sem_wait(&send_data);
                
                while(go_back_n){
                    struct QNode *cur = packets->front;
                    while(cur != NULL){
                        sendto(client_fd, cur->buffer, sizeof(cur->buffer), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
                        if(cur == packets->front) {
                            sem_post(&timer_start);
                            sem_wait(&send_data);
                        }
                    }
                }
                deQueue(packets);
                
            }

        }
        

    } while (err != 0);

    m_run_timer_thread = false;
    pthread_join(timer_thread,NULL);
}
