#include "Definitions.hpp"
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#define PORT 6969

int main(){

    Header *h = new Header();
    h->version = 69;
    h->payloadSize = 35;
    h->sequenceNumber = 11;
    h->timestamp = 1212121;
    NewOrder *n = new NewOrder();
    n->messageType = 50;
    n->listingId = 51;
    n->orderId = 52;
    n->orderQuantity = 53;
    n->orderPrice = 54;
    n->side = 's';

    NewOrderWithHeader* nh = new NewOrderWithHeader();
    nh->header = *h;
    nh->newOrder = *n;


    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
        printf("\n Socker creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET,"127.0.0.1", &serv_addr.sin_addr) <= 0){
        printf("\n  Invalid address / Address not supported \n");
        return -1;
    }

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0){
        printf("\n Connection failed \n");
        return -1;
    }
    send(sock, nh, 51, 0);
    printf("Sent");
    printf("%s\n", buffer);
    close(sock);
    return 0;
}