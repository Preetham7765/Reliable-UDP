#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include "a2.h"

#define MAX_BUFFER_SIZE 1024
#define PORT 5555


void start_server(cfg_t *config) {

    int server_fd;
    int len, n;
    int err = 0;
    char buff[MAX_BUFFER_SIZE];


    struct sockaddr_in server_addr, client_addr;

    memset(&server_addr, 0 , sizeof(server_addr));
    memset(&client_addr, 0 , sizeof(client_addr));

    server_addr.sin_family = AF_INET;  
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = PORT;


    do{

        // create sockets

        if((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){

            fprintf(stderr, "socket creation failed");
            err = -1;
            break;
        }


        //bind the socket
        if(bind(server_fd, (const struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
            fprintf(stderr, "socket bind failed");
            err = -1;
            break;
        }


        // recieve from the server

        while(( n = recvfrom(server_fd, &buff, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&client_addr,&len)) > 0) {
            // write to the destination file

            if(strcmp(buff, "GOOD_BYE") == 0){
                fprintf(stdout, "got good bye from the client... Server shutting down\n");
                break;
            }
            fprintf(stdout, "%s", buff);
            fwrite(buff, 1 , n, config->fstream);

            memset(buff,0, sizeof(buff));
        }

        if( n == 0){
            fprintf(stderr, "Client sending an empty message... Server shutting down\n");
            break;
        }

        if(n < 0){
            fprintf(stderr, "Error recieving from the server\n");
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
    char buff[MAX_BUFFER_SIZE];
    size_t nread;


    struct sockaddr_in server_addr, client_addr;

    memset(&server_addr, 0 , sizeof(server_addr));
    memset(&client_addr, 0 , sizeof(client_addr));

    server_addr.sin_family = AF_INET;  
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = PORT;


    do{

        // create sockets

        if((client_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){

            fprintf(stderr, "socket creation failed");
            err = -1;
            break;
        }
        
        // read 1023 bytes from the file and send

        if(config->fstream != NULL){

            while ((nread = fread(buff, 1, sizeof(buff), config->fstream)) > 0){

                n = sendto(client_fd, &buff, nread, 0, (struct sockaddr*)&server_addr,sizeof(server_addr));
                if(n < 0){
                    fprintf(stderr, "Error sending to the server\n");
                }

                // once the data is sent clear the existing buffer.
                memset(buff,0, sizeof(buff));
            }
        }
        //send good bye to server
        sendto(client_fd, "GOOD_BYE", strlen("GOOD_BYE"), 0, (struct sockaddr*)&server_addr,sizeof(server_addr));

    }while(err != 0);

    // create sockets   

    close(client_fd);


}