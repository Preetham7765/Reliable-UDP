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
#include "a2.h"

#define MAX_BUFFER_SIZE 10
#define PORT 5555
#define ACK "ACK"
#define GOOD_BYE "GOOD_BYE" 


void start_server(cfg_t *config) {

    int server_fd;
    int len, n;
    int err = 0;
    uint8_t packet[MAX_BUFFER_SIZE +1];
    unsigned char ip[sizeof(struct in_addr)];
    char buff[MAX_BUFFER_SIZE];
    uint8_t expected_seq = 0;


    struct sockaddr_in server_addr, client_addr;
    config->server = "10.10.2.10";

    memset(&server_addr, 0 , sizeof(server_addr));
    memset(&client_addr, 0 , sizeof(client_addr));

    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, config->server, &(server_addr.sin_addr.s_addr));
    server_addr.sin_port = htons(config->port);
    
    len = sizeof(client_addr);

    do{

        // create sockets

        if((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){

            fprintf(stderr, "socket creation failed\n");
            err = -1;
            break;
        }


        //bind the socket
        if(bind(server_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
            fprintf(stderr, "socket bind failed\n");
            err = -1;
            break;
        }


        // recieve from the server

        while(( n = recvfrom(server_fd, packet, sizeof(packet), 0, (struct sockaddr *)&client_addr,&len)) > 0) {
            // write to the destination file
             printf("recived data %s\n", packet);
            if(strcmp(packet, GOOD_BYE) == 0){
                fprintf(stdout, "got good bye from the client... Server shutting down\n");
                break;
            }

            // check  if the seq number is same as expected then 
            // write to the buffer and send ack.
            uint8_t seq = packet[0] - '0';
            printf("seq %d expected_seq %d\n", seq, expected_seq);
            if(seq == expected_seq){
                memcpy(buff, &packet[1], n-1);
                fwrite(buff, 1 , n-1, config->fstream);
                memset(packet,0,sizeof(buff));
                memset(buff,0, sizeof(buff));
                char ack_buffer[5] = {'A', 'C', 'K'};
                ack_buffer[3] = 48 + seq;
                printf("\nACK sending.... %s\n", ack_buffer);
                sendto(server_fd, ack_buffer, sizeof(ack_buffer),0,(struct sockaddr *)&client_addr, sizeof(client_addr));
                expected_seq = seq ^ 1;
            } 
            else {
                char ack_buffer[5] = {'A', 'C', 'K'};
                ack_buffer[3] = 48 + seq;
		printf("\n Server sendning ACK .....%s\n",ack_buffer);
                sendto(server_fd, ack_buffer, sizeof(ack_buffer),0,(struct sockaddr *)&client_addr, sizeof(client_addr));
            }
            
        }

        if( n == 0){
            fprintf(stderr, "Client sending an empty message... Server shutting down\n");
            break;
        }

        if(n < 0){
            fprintf(stderr, "Error recieving from the client %d error num %d\n", n, errno);
            break;
        }    
        

    }while(err != 0);

    // close sockets
    close(server_fd);


}

void start_client(cfg_t *config) {

    int client_fd;
    int n;
    int err=0;
    uint8_t packet[MAX_BUFFER_SIZE +1];
    char buff[MAX_BUFFER_SIZE];
    uint8_t seq=0;
    size_t nread;
    fd_set rfds;
    struct timeval tv;


    struct sockaddr_in server_addr, client_addr;

    memset(&server_addr, 0 , sizeof(server_addr));
    memset(&client_addr, 0 , sizeof(client_addr));

    server_addr.sin_family = AF_INET;  
    inet_pton(AF_INET, config->server, &(server_addr.sin_addr));
    server_addr.sin_port = htons(config->port);

    FD_ZERO(&rfds);

    do{

        // create sockets

        if((client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
            fprintf(stderr, "socket creation failed");
            err = -1;
            break;
        }
        
        // read 1023 bytes from the file and send

        // memcpy(buff,&seq,1);
        // buff[0] = (char)seq;
        // packet[0] = seq;
        sprintf(packet, "%d", seq);
        printf("sending packet preetham %s\n", packet);
        if(config->fstream != NULL){
            
            while ((nread = fread(buff , 1, sizeof(buff), config->fstream)) > 0){
                int retval = 0;
                do {
                    
                    memcpy(&packet[1], &buff[0], nread);
		    printf("sending data %s\n",packet);
                    n = sendto(client_fd, &packet, nread+1, 0, (struct sockaddr*)&server_addr,sizeof(server_addr));
                    if(n < 0){
                        fprintf(stderr, "Error sending to the server\n");
                    }
                    // start time wait for the ACK
                    //1. use select and wait for that time. u need to add to fdset  

		    FD_SET(client_fd, &rfds);
		    tv.tv_sec = 0;
		    tv.tv_usec = 25000;
                    retval = select(client_fd+1, &rfds, NULL, NULL, &tv); 
		    printf("select return value %d\n", retval);
		    if(retval !=0){
			char ack[5];
                    	read(client_fd, ack,sizeof(ack));
                    	char expected_ack[5] = {'A', 'C', 'K'};
                    	expected_ack[3] = 48 + seq;
                    	printf("\nACK got ack %s and expected.... %s\n", ack, expected_ack);  
                    if(strcmp(ack, expected_ack) != 0){
                        fprintf(stderr, "Unexpected data recieved from the server %s\n", ack);
                        retval=0;
                    }
		  }
		}while(retval == 0);
                seq  = seq ^ 1;
                memset(packet,0,sizeof(buff));
                memset(buff,0, sizeof(buff));
                sprintf(packet, "%d", seq);
            }
        }
        //send good bye to server
	printf("Sending Good_bye !!!\n");
        sendto(client_fd, "GOOD_BYE", strlen("GOOD_BYE"), 0, (struct sockaddr*)&server_addr,sizeof(server_addr));

    }while(err != 0);

    // create sockets   

    close(client_fd);


}
