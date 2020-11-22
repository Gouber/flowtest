#include <stdio.h>  
#include <string.h>   //strlen  
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   //close  
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros 
#include <iostream>
#include "Definitions.hpp"
#include <inttypes.h>
#include <vector>
#include <algorithm>


#define PORT 6969
#define TRUE   1  
#define FALSE  0 

using namespace std;

void addSocket(vector<int>& socketsSoFar, int socketid){
    printf("Before inside addSocket: \n");
    for(int i: socketsSoFar){
        printf("%d,",i);
    } 
    printf("\n");
    
    socketsSoFar.push_back(socketid);
    printf("After addSocket: \n");
    for(int i: socketsSoFar){
        printf("%d,",i);
    } 
    printf("\n");
}
void cleanup(vector<int>& socketsSoFar, vector<int>& socketid){
    if(socketid.size() == 0){
        printf("Nothing to clean");
        return;
    }
    printf("Before inside cleanup: \n");
    for(int i: socketsSoFar){
        printf("%d,",i);
    } 
    printf("\n");
    for(int deletenow: socketid){
        auto it = find(socketsSoFar.begin(), socketsSoFar.end(), deletenow);
        socketsSoFar.erase(it);
    }
    printf("After cleanup: \n");
    for(int i: socketsSoFar){
        printf("%d,",i);
    } 
    printf("\n");
}


int main(){
    int opt = TRUE;

    vector<int> socketsSoFar;
    //number of sockets currently connected

    int master_socket, addrlen, new_socket, max_clients = 30, activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;
    char buffer[16];
    //set of socket descriptors - these are the ones we are going to check
    fd_set readfds;
    //init all client sockets to 0 - aka we are not checking 'em
    //create a master socket
    if( (master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0 ){
        perror("socket init failed");
        exit(EXIT_FAILURE);
    }
    
    //apparently this is good habit - but it works without - can remove at end if it still works (?)
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0 ){
        perror("setsockopts failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //binding it to localhost - sort of kicks off the server
    if(::bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d", PORT);

    if(listen(master_socket, 3) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    puts("Waiting for connections");

    while(true){
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;

        for(int socketInsideSoFar : socketsSoFar){
            FD_SET(socketInsideSoFar, &readfds);
            if(socketInsideSoFar > max_sd)
                max_sd = socketInsideSoFar;
        }

        //this "blocks" in the sense that it waits for one of them to do something
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        //we can assume after this, activity has something for us - either an error, either something useful

        if((activity < 0) && (errno != EINTR)){
            printf("select error");
        }
        if(FD_ISSET(master_socket, &readfds)){
            //if it is the master, then clearly we are getting a new connection - we can add it to the sockets available
            if( (new_socket = accept(master_socket, (struct sockaddr* ) &address, (socklen_t*)&addrlen )) < 0 ){
                perror("accept failed");
                exit(EXIT_FAILURE);
            }
            printf("New connection, socket fd is %d, ip is : %s, port : %d \n", new_socket,inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            // if(send(new_socket, message, strlen(message), 0) != strlen(message)){
            //     perror("send failed");
            // }
            // puts("Welcome message sent successfully");
            addSocket(socketsSoFar, new_socket);
        }
        vector<int> toDeleteLater;
        for(i = 0; i < socketsSoFar.size(); i++){
            sd = socketsSoFar[i];

            if(FD_ISSET(sd, &readfds)){
                //did this socket say anything?
                
                if((valread = read(sd,buffer,16)) == 0){
                    //disconnected
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Host disconnected, ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    toDeleteLater.push_back(sd);
                }else{
                    //Let's assume for now that input is always correct
                    Header* s = (struct Header *) &buffer;
                    int payload = s->payloadSize;
                    switch (payload)
                    {
                    case 35:{
                        /* code */
                        char newOrderBuffer[35];
                        read(sd,newOrderBuffer, 35);
                        NewOrder* newOrder = (struct NewOrder *) &newOrderBuffer;
                        break;
                    }
                    case 10:{
                        char deleteOrderBuffer[10];
                        read(sd,deleteOrderBuffer, 10);
                        DeleteOrder* deleteOrder = (struct DeleteOrder *) &deleteOrderBuffer;
                        break;
                    }
                    case 18: {
                        char modifyOrderBuffer[18];
                        read(sd,modifyOrderBuffer, 18);
                        ModifyOrderQuantity* modifyOrderQuantity = (struct ModifyOrderQuantity *) modifyOrderBuffer;
                        break;
                    }
                    case 34: {
                        char tradeBuffer[34];
                        read(sd,tradeBuffer, 34);
                        Trade* trade = (struct Trade*) tradeBuffer; 
                        break;
                    }
                    default:
                        break;
                    }
                }
            }
        }
        cleanup(socketsSoFar,toDeleteLater);
    }
    return 0;
}