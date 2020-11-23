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
#include <unordered_map>

#define PORT 6969

using namespace std;

class Server{
    void sendResponse(int sd, int yesOrNo, uint64_t orderId){
        OrderResponse* response = new OrderResponse();
        response->orderId = orderId;
        if(yesOrNo == 1){
            response->status = OrderResponse::Status::ACCEPTED;
        }else{
            response->status = OrderResponse::Status::REJECTED;
        }
        if(send(sd,response,12,0) != 12){
            perror("Could not send response");
        }
    }

    void addSocket(vector<int>& socketsSoFar, int socketid){
        socketsSoFar.push_back(socketid);
    }
    void cleanup(vector<int>& socketsSoFar, vector<int>& socketid){
        if(socketid.size() == 0){
            return;
        }
        for(int deletenow: socketid){
            auto it = find(socketsSoFar.begin(), socketsSoFar.end(), deletenow);
            socketsSoFar.erase(it);
        }
    }

    public:
        Server(){
                int opt = 1;
    //all the individual socket "handlers"

    int buyThreshold = 20;
    int sellThreshold = 15;

    vector<int> socketsSoFar;
    //These are all the instruments and their associated 
    //net pos, buy qty, sell qty, buy hypothetical worst, sell hypothetical worst
    unordered_map<uint64_t, vector<uint64_t>> instrumentPositions;
    //Trader and his corresponding orders
    unordered_map<int, vector<uint64_t>> traderOrders;
    //every order ever recorded should exist here
    //the vector<int> represents respectively:
    //Question: Should I use the struct(?)
    //quantity, instrumentId, sell/buy (0/1), traderId
    unordered_map<uint64_t, vector<uint64_t>> orderAndCorrespondingDetails;

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
            addSocket(socketsSoFar, new_socket);
        }
        vector<int> toDeleteLater;
        for(i = 0; i < socketsSoFar.size(); i++){
            sd = socketsSoFar[i];
            //could further optimize by keeping a count that should equal activity
            if(FD_ISSET(sd, &readfds)){
                //did this socket say anything?
                if((valread = read(sd,buffer,16)) == 0){
                    //disconnected
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Host disconnected, ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    close(sd);
                    toDeleteLater.push_back(sd);
                    //Delete all orders associated with this trader and revert their corresponding effect
                    if(traderOrders.find(sd) != traderOrders.end()){
                        vector<uint64_t> ordersOfThisTrader = traderOrders[sd];
                        for(uint64_t order: ordersOfThisTrader){
                            if(orderAndCorrespondingDetails.find(order) != orderAndCorrespondingDetails.end()){
                                vector<uint64_t> singleOrderDetails = orderAndCorrespondingDetails[order];
                                uint64_t quantity = singleOrderDetails[0];
                                uint64_t instrumentId = singleOrderDetails[1];
                                uint64_t side = singleOrderDetails[2];
                                //might be worth having a function as I'm doing this twice
                                if(instrumentPositions.find(instrumentId) != instrumentPositions.end()){
                                    vector<uint64_t> positionDetail = instrumentPositions[instrumentId];
                                    if(side == 0){
                                        //we sold
                                        positionDetail[2] -= quantity;
                                        positionDetail[4] = max(positionDetail[2], positionDetail[2] - positionDetail[0]);
                                    }else{
                                        //we buy
                                        positionDetail[1] -= quantity;
                                        positionDetail[3] = max(positionDetail[1], positionDetail[1] + positionDetail[0]);
                                    }
                                    instrumentPositions[instrumentId] = positionDetail;
                                }//we cannot find given instrument in our records
                                orderAndCorrespondingDetails.erase(order);
                            }//else we cannot find this order in our record of orders
                        }
                        traderOrders.erase(sd);
                    }//else we cannot find this trader in our records - we cannot see what orders this trader has
                    //Also delete this trader from our book
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
                        uint64_t instrumentId = newOrder->listingId;
                        uint64_t orderId = newOrder->orderId;
                        uint64_t quantity = newOrder->orderQuantity;
                        char side = newOrder->side;
                        //first let's check if this would pass the theoretical limits
                        if(instrumentPositions.find(instrumentId) != instrumentPositions.end()){
                            //this instrument already exists
                            //need to check some conditions
                            vector<uint64_t> instrumentDetails = instrumentPositions[instrumentId];
                            uint64_t net = instrumentDetails[0];
                            uint64_t buy = instrumentDetails[1];
                            uint64_t sell = instrumentDetails[2];
                            uint64_t worstBuy = instrumentDetails[3];
                            uint64_t worstSell = instrumentDetails[4];
                            if(side == 'B'){
                                //check if the buy 
                                uint64_t theoreticalBuyAfterThisStep = max(buy + quantity, buy+quantity+net);
                                if(theoreticalBuyAfterThisStep <= buyThreshold){
                                    //actual do the changes
                                    instrumentDetails[1] += quantity;
                                    instrumentDetails[3] = theoreticalBuyAfterThisStep;
                                    instrumentPositions[instrumentId] = instrumentDetails;
                                    if(traderOrders.find(sd) != traderOrders.end()){
                                        traderOrders[sd].push_back(sd);
                                    }else{
                                        traderOrders[sd] = vector<uint64_t>{orderId};
                                    }
                                    orderAndCorrespondingDetails[orderId] = vector<uint64_t>{quantity,instrumentId,(uint64_t)1, (uint64_t)sd};
                                    sendResponse(sd,1,orderId);
                                }else{
                                    sendResponse(sd,0,orderId);
                                }
                            }else if(side == 'S'){
                                uint64_t theoreticalSellAfterThisStep = max(sell + quantity , sell + quantity - net);
                                if(theoreticalSellAfterThisStep <= sellThreshold){
                                    //actual do the changes
                                    instrumentDetails[2] += quantity;
                                    instrumentDetails[4] = theoreticalSellAfterThisStep;
                                    instrumentPositions[instrumentId] = instrumentDetails; 
                                    //still need to update the traderOrders and the orderAndCoresspondingDetails
                                    if(traderOrders.find(sd) != traderOrders.end()){
                                        traderOrders[sd].push_back(sd);
                                    }else{
                                        traderOrders[sd] = vector<uint64_t>{orderId};
                                    }
                                    orderAndCorrespondingDetails[orderId] = vector<uint64_t>{quantity,instrumentId,(uint64_t)0,(uint64_t)sd};
                                    sendResponse(sd,1,orderId);
                                }else{
                                    sendResponse(sd,0,orderId);
                                }
                            }
                        }else{
                            //the only thing that we need to check is the side and the respective quantities as nothing else affects this
                            if(side == 'B'){
                                if(quantity <= buyThreshold){
                                    //yes we are allowed. proceed
                                    sendResponse(sd,1,orderId);
                                }else{
                                    //return a NO
                                    sendResponse(sd,0,orderId);
                                }
                            }else if(side == 'S'){
                                if(quantity <= sellThreshold){
                                    //yes we are allowed. proceed
                                    sendResponse(sd,1,orderId);
                                }else{
                                    //return a NO
                                    sendResponse(sd,0,orderId);
                                }
                            }
                        }
                        break;
                    }
                    case 10:{
                        //deleting an order shouldn't require a response
                        char deleteOrderBuffer[10];
                        read(sd,deleteOrderBuffer, 10);
                        DeleteOrder* deleteOrder = (struct DeleteOrder *) &deleteOrderBuffer;

                        uint64_t orderId = deleteOrder->orderId;

                        if(orderAndCorrespondingDetails.find(orderId) != orderAndCorrespondingDetails.end()){
                            vector<uint64_t> orderDetails = orderAndCorrespondingDetails[orderId];
                            uint64_t quantity = orderDetails[0];
                            uint64_t instrumentId = orderDetails[1];
                            uint64_t side = orderDetails[2];
                            uint64_t traderId = orderDetails[3];

                            //delete it from the orders of the trader
                            //-----------------------------------------
                            if(traderOrders.find(traderId) != traderOrders.end()){
                                vector<uint64_t> hisOrders = traderOrders[traderId];
                                auto it = find(hisOrders.begin(), hisOrders.end(), orderId);
                                if(it != hisOrders.end()){
                                    hisOrders.erase(it);
                                    traderOrders[traderId] = hisOrders;
                                }//else magically this order is already missing from the traders orders
                            }//else the trader doesn't exist so we can't delete any of his orders
                            //-----------------------------------------

                            //modify the instrument positions accordingly
                            //-----------------------------------------
                            if(instrumentPositions.find(instrumentId) != instrumentPositions.end()){
                                vector<uint64_t> instrumentData = instrumentPositions[instrumentId];
                                if(side == 0){
                                    //we were selling
                                    instrumentData[2] -= quantity;
                                    instrumentData[4] = max(instrumentData[2], instrumentData[2] - instrumentData[0]);
                                }else{
                                    //we were buying
                                    instrumentData[1] -= quantity;
                                    instrumentData[3] = max(instrumentData[1], instrumentData[1] + instrumentData[0]);
                                }
                                instrumentPositions[instrumentId] = instrumentData;
                            }//else this instrument id does not appear in our records
                            //-----------------------------------------
                            //finally delete it from the orders
                            orderAndCorrespondingDetails.erase(orderId);
                        }//else this order doesn't even exist in our directory of orders
                        break;
                    }
                    case 18: {
                        char modifyOrderBuffer[18];
                        read(sd,modifyOrderBuffer, 18);
                        ModifyOrderQuantity* modifyOrderQuantity = (struct ModifyOrderQuantity *) modifyOrderBuffer;

                        //does this require response or no?

                        break;
                    }
                    case 34: {
                        //this is from the exchange so we don't need a response
                        char tradeBuffer[34];
                        read(sd,tradeBuffer, 34);
                        Trade* trade = (struct Trade*) tradeBuffer; 
                        uint64_t instrumentId = trade->listingId;
                        uint64_t tradeQuantity = trade->tradeQuantity;
                        if(instrumentPositions.find(instrumentId) != instrumentPositions.end()){
                            //found so just modify
                            vector<uint64_t> instrumentDetails = instrumentPositions[instrumentId];
                            instrumentDetails[0] += tradeQuantity;
                            instrumentDetails[3] = max(instrumentDetails[1], instrumentDetails[1] + instrumentDetails[0]);
                            instrumentDetails[4] = max(instrumentDetails[2], instrumentDetails[2] - instrumentDetails[0]);
                            instrumentPositions[instrumentId] = instrumentDetails;
                        }else{
                            //not found so must add
                            vector<uint64_t> instrumentDetails = {tradeQuantity, 0,0, max((uint64_t)0, tradeQuantity), max((uint64_t)0, -tradeQuantity)};
                            instrumentPositions[instrumentId] = instrumentDetails;
                        }
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
        }
};

int main(){
    Server s;
    return 0;
}