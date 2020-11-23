#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include "Client.hpp"



void Client::disconnect(){
    close(this->sockfd);
}

void Client::headerUpdate(){
    this->h->sequenceNumber += 1;
    this->h->timestamp += 1;
}

bool Client::sendOrder(uint64_t instrumentId, uint64_t orderId, uint64_t quantity, uint64_t orderPrice, char side){
    NewOrderWithHeader* orderandheader = new NewOrderWithHeader();
    h->payloadSize = 35;
    orderandheader->header = *(h);
    NewOrder *neworder = new NewOrder();
    neworder->listingId = instrumentId;
    neworder->orderId = orderId;
    neworder->orderQuantity = quantity;
    neworder->orderPrice = orderPrice;
    neworder->side = side;
    orderandheader->newOrder = *neworder;
    send(this->sockfd, orderandheader, 51, 0);
    char *buffer[12];
    read(this->sockfd, buffer, 12);
    OrderResponse *response = (struct OrderResponse*) buffer;
    this->headerUpdate();
    if(response->status == OrderResponse::Status::ACCEPTED){
        return true;
    }else{
        return false;
    }
}


void Client::deleteOrder(uint64_t orderId){
    h->payloadSize = 10;
    DeleteOrderWithHeader* dh = new DeleteOrderWithHeader();
    DeleteOrder* d = new DeleteOrder();
    d->orderId = orderId;
    dh->header = *(h);
    dh->deleteOrder = *d;
    send(this->sockfd, dh, 26, 0);
    this->headerUpdate();
}

void Client::modifyOrder(uint64_t orderId, uint64_t newQuantity){
    h->payloadSize = 18;
    ModifyOrderQuantity *m = new ModifyOrderQuantity();
    m->orderId = orderId;
    m->newQuantity = newQuantity;
    ModifyOrderQuantityWithHeader *mh = new ModifyOrderQuantityWithHeader();
    mh -> header = *(h);
    mh -> modifyOrderQuantity = *m;
    send(this->sockfd, mh, 34, 0);
    this->headerUpdate();
}

void Client::trade(uint64_t instrumentId, uint64_t tradeId, uint64_t quantity, uint64_t price){
    h->payloadSize = 34;
    Trade *t = new Trade();
    t->tradeId = tradeId;
    t->listingId = instrumentId;
    t->tradeQuantity = quantity;
    t->tradePrice = price;
    TradeWithHeader* th = new TradeWithHeader();
    th->header = *(h);
    th->trade = *t;
    send(this->sockfd, th, 50,0);
    this->headerUpdate();
}


Client::Client(int port){
    
    //handle connection
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    if( (sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ){
        printf("\n Socker creation error \n");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if(inet_pton(AF_INET,"127.0.0.1", &serv_addr.sin_addr) <= 0){
        printf("\n  Invalid address / Address not supported \n");
    }

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr) ) < 0){
        printf("\n Connection failed \n");
    }
    this->sockfd = sock;

    //handle the header
    Header* h = new Header();
    h->sequenceNumber = 1;
    h->timestamp = 1;
    h->version = 1;
    this->h = h;

}