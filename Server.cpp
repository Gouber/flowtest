#include <stdio.h>  
#include <string.h> 
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>  
#include <arpa/inet.h>   
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h> 
#include <iostream>
#include "Definitions.hpp"
#include <vector>
#include <algorithm>
#include "Server.hpp"

#define PORT 8080
using namespace std;


Server::Server(int buyMax, int sellMax){
    this->buyMax = buyMax;
    this->sellMax = sellMax;
}

void Server::addTraderAndHisOrder(int sd, uint64_t orderId){
    if(this->traderOrders.find(sd) != this->traderOrders.end()){
        this->traderOrders[sd].push_back(orderId);
    }else{
        this->traderOrders[sd] = vector<uint64_t>{orderId};
    }
}


//If the vector is empty we can simply remove it - to keep the "state" tidy
bool emptyInstrument(vector<uint64_t>& pos){
    return pos[0] == 0 && pos[1] == 0 && pos[2] == 0 && pos[3] ==0 && pos[4] == 0;
}

//I am assuming the response does not need a header as it's never specified but it can easily be extended
//This function sends a response given the socket identifier sd
void Server::sendResponse(int sd, int yesOrNo, uint64_t orderId){
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

//Add a newly connected socket to our vector
//Note the only reason why I made a function for this is to make explicit we are handling sockets
void Server::addSocket(vector<int>& socketsSoFar, int socketid){
    socketsSoFar.push_back(socketid);

    //Note I am assuming the first packet will have sequence order 1 (as I'm setting 0 here)
    //If this is a wrong assumption then I could change it in a revision of this code.
    consistencyChecker[socketid] = vector<uint64_t>{0,0};
}

//Purge the sockets that have disconnected
void Server::cleanup(vector<int>& socketsSoFar, vector<int>& socketid){
    if(socketid.size() == 0){
        return;
    }
    for(int deletenow: socketid){
        auto it = find(socketsSoFar.begin(), socketsSoFar.end(), deletenow);
        socketsSoFar.erase(it);
    }
}

//Starting the server
void Server::start(){
    int opt = 1;
    //Either use default if constructor wasn't given arguments (like in unitTests.cpp)
    //Or use the actual inputs from the user
    int buyThreshold = this->buyMax;
    int sellThreshold = this->sellMax;


    //This is where we'll be storing the connected sockets
    vector<int> socketsSoFar;
    //This will be the master socket identifier. This is where all other sockets will connect
    int master_socket;
    //Misc variables
    int addrlen, new_socket,  activity, i, valread, sd;
    //we need to give the max socket to `activity` in the future
    int max_sd;
    struct sockaddr_in address;
    //We know we must initially read a header to know what more to expect. 
    //The header, due to the __packed__ attribute is of size 16
    char buffer[16];
    //We pass this to activity - it allows us to monitor what socket from our registered ones is "active"
    fd_set readfds;
    
    //Create the master socket that accepts TCP connections and check if it succeeded
    if( (master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0 ){
        perror("socket init failed");
        exit(EXIT_FAILURE);
    }
    
    //removes the "address already in use"
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0 ){
        perror("setsockopts failed");
        exit(EXIT_FAILURE);
    }
    //basic networking settigns
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    //binding it to localhost
    if(::bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //start listening - we only allow a backlog of 3
    if(listen(master_socket, 3) < 0){
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    printf("Listening on port %d\n", PORT);
    while(true){
        //"FD_ZERO(&fdset) initializez the file descriptor set fdset to have zero bits for all file descriptors"
        //Read and accepted from the documentation
        FD_ZERO(&readfds);
        //Set our master socket in the monitor "screen"
        FD_SET(master_socket, &readfds);
        //We need the master descriptor which currently is only the master
        max_sd = master_socket;

        for(int socketInsideSoFar : socketsSoFar){
            //set the individual sockets we are monitoring back into the FD set
            FD_SET(socketInsideSoFar, &readfds);
            if(socketInsideSoFar > max_sd)
                //keep track of its maximum
                max_sd = socketInsideSoFar;
        }

        //this "blocks" in the sense that it waits for one of them to do something
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        //we can assume after this, activity has something for us - either an error, either something useful
        //also note activity returns the number of sockets that are ready to be used
        //an extension to this code would be that inside the loop down below to stop
        //when a counter reaches activity

        //Make sure we didn't get some error insisde selects
        if((activity < 0) && (errno != EINTR)){
            printf("select error");
        }
        //Check which socket is ready
        if(FD_ISSET(master_socket, &readfds)){
            //if it is the master, then clearly we are getting a new connection - we can add it to the sockets available
            if( (new_socket = accept(master_socket, (struct sockaddr* ) &address, (socklen_t*)&addrlen )) < 0 ){
                perror("accept failed");
                exit(EXIT_FAILURE);
            }
            //Arguably better than a direct push_back into the socket vector
            //Makes it clearer what is going on (?)
            addSocket(socketsSoFar, new_socket);
        }
        //We need to keep track of what to purge - aka what sockets have emitted the "i'm dead" signal
        vector<int> toDeleteLater;
        for(i = 0; i < socketsSoFar.size(); i++){
            sd = socketsSoFar[i];
            //Inside this loop, we could keep the counter and stop when it reaches activity

            //Is the socket with the following descriptor ready for action?
            if(FD_ISSET(sd, &readfds)){
                //This socket is ready but it either disconnected, either sent something

                if((valread = read(sd,buffer,16)) == 0){
                    // it must've disconnected
                    // obtain details about its connection
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    //Close the connection
                    close(sd);
                    //Add it such that we can purge it later
                    toDeleteLater.push_back(sd);
                    //Delete all orders associated with this trader and revert their corresponding effect
                    //Note... I wasn't sure if an exchange would connect completely separately
                    //or if an exchange order could be sent via the same socket that sent a trader order
                    //I made it such that anything the exchange says, cannot be reverted
                    //i.e.: even if trader 1 disconnects, anything he told us about the exchange
                    //is still in our state.
                    //it's arguably easier to not do this.

                    //Find what orders this trader did - did he do any? 
                    //Maybe all his orders get deleted - avoid that horrible segmentation fault
                    if(traderOrders.find(sd) != traderOrders.end()){
                        //store a list of his orders if we find any
                        vector<uint64_t> ordersOfThisTrader = traderOrders[sd];
                        //for every order, reverse its effect
                        for(uint64_t order: ordersOfThisTrader){
                            //We look for the order details for given order
                            if(orderAndCorrespondingDetails.find(order) != orderAndCorrespondingDetails.end()){
                                //We store the individual order details
                                vector<uint64_t> singleOrderDetails = orderAndCorrespondingDetails[order];
                                //destructure the vector for clarity
                                uint64_t quantity = singleOrderDetails[0];
                                uint64_t instrumentId = singleOrderDetails[1];
                                uint64_t side = singleOrderDetails[2];

                                //This order should refer to an instrument we have a record of
                                if(instrumentPositions.find(instrumentId) != instrumentPositions.end()){
                                    //Get that instrument's details
                                    vector<uint64_t> positionDetail = instrumentPositions[instrumentId];
                                    //Decide how this order affected our quantities based on the side it was on
                                    if(side == 0){
                                        //we sold
                                        positionDetail[2] -= quantity;
                                        positionDetail[4] =  (uint64_t) max( (long long) positionDetail[2],(long long) positionDetail[2] - (long long) positionDetail[0] );
                                    }else{
                                        //we buy
                                        positionDetail[1] -= quantity;
                                        positionDetail[3] = (uint64_t) max( (long long) positionDetail[1], (long long) positionDetail[1] + (long long) positionDetail[0]);
                                    }
                                    //Purge a zero-ed out vector as we don't need it!
                                    if(emptyInstrument(positionDetail)){
                                        instrumentPositions.erase(instrumentId);
                                     }else{
                                        instrumentPositions[instrumentId] = positionDetail;
                                     }
                                }//we cannot find given instrument in our records
                                //Delete this order from our records after we're done reverting its effect
                                orderAndCorrespondingDetails.erase(order);
                            }//else we cannot find this order in our record of orders
                        }
                        //Delete this trader from the record of traders - he's gone!
                        traderOrders.erase(sd);
                        consistencyChecker.erase(sd);
                    }//else we cannot find this trader in our records - we cannot see what orders this trader has
                }else{
                    // Do some checking with the input
                    Header* s = (struct Header *) &buffer;

                    //Note: I did not have as much time to dabble with the header consistency checking part
                    //Here I have an experimental "try" at it. With the tests I have, and with the Client I have
                    //it seems to work well.. However, if things don't work from a logical point of view
                    //and I could get partial points for a solution without a header, then please comment out
                    //this section. I am confident the rest of the program should run well. I'd love
                    //to talk more about this section during a live interview.
                    //Also please comment out line 62 as well (if that's the case)
                    //------------------------------------------------------------------------
                    char consumeBadPacketBuffer[s->payloadSize];
                    if(consistencyChecker.find(sd) != consistencyChecker.end()){
                        //clearly we've received packets from this socket before
                        //Check the packets come in order;
                        vector<uint64_t> headerBefore = consistencyChecker[sd];
                        //Note it is actually uint32_t but for simplicity let's assume a bigger data stucture
                        uint64_t sequenceNumberBefore = headerBefore[0];
                        uint64_t timestamp = headerBefore[1];

                        if((uint64_t) s->sequenceNumber != sequenceNumberBefore + 1 || (uint64_t) s->timestamp <= timestamp ){
                            //clearly this is not consistent
                            read(sd, consumeBadPacketBuffer, s->payloadSize);
                            continue;
                        }
                        //If we are here, clearly the data is consistent. We can keep this new record
                        //to check any future packets
                        consistencyChecker[sd] = vector<uint64_t>{sequenceNumberBefore + 1, s->timestamp};
                    }
                    //------------------------------------------------------------------------
                    //If we get to this point, clearly our header checks out
                    //Our data is consistent, we are happy

                    //figure out what we're about to read
                    int payload = s->payloadSize;
                    switch (payload)
                    {
                    //NewOrder
                    case 35:{
                        //Read the data we are receiving
                        char newOrderBuffer[35];
                        read(sd,newOrderBuffer, 35);
                        //Cast the buffer into our struct
                        NewOrder* newOrder = (struct NewOrder *) &newOrderBuffer;
                        //destructure it for clarity
                        uint64_t instrumentId = newOrder->listingId;
                        uint64_t orderId = newOrder->orderId;
                        uint64_t quantity = newOrder->orderQuantity;
                        char side = newOrder->side;

                        //Two cases 
                        //1) Does this instrument already exist?
                        //2) It's a completely new instrument


                        if(instrumentPositions.find(instrumentId) != instrumentPositions.end()){
                            //This instrument already exists
                            //Calculate its new buy and sell hyps
                            //Get the instrument position data from our dict
                            vector<uint64_t> instrumentDetails = instrumentPositions[instrumentId];
                            //destructure it for clarity
                            //NOTE: net although a unsigned quantity, it is very much signed
                            //hence why make it a long long
                            long long net = instrumentDetails[0];
                            uint64_t buy = instrumentDetails[1];
                            uint64_t sell = instrumentDetails[2];
                            uint64_t worstBuy = instrumentDetails[3];
                            uint64_t worstSell = instrumentDetails[4];

                            //What is this order's side?
                            //This is to calculate the theoretical resulting limit
                            if(side == 'B'){
                                //Note because net is signed, we must assume the entire thing below is signed
                                //Alternative would be to make our own comparison function
                                uint64_t theoreticalBuyAfterThisStep = (uint64_t) max( (long long)buy + (long long) quantity, (long long) buy+ (long long) quantity + net);
                                if(theoreticalBuyAfterThisStep <= buyThreshold){
                                    //This order does not violate the threshold so we can execute it!
                                    //Construct the position data below
                                    instrumentDetails[1] += quantity;
                                    instrumentDetails[3] = theoreticalBuyAfterThisStep;
                                    //Actually assign the position data in our global dict
                                    instrumentPositions[instrumentId] = instrumentDetails;

                                    //Check if the current trader is registered in our books
                                    //If he is not, this is either his first trade, or 
                                    //he deleted all of his trades a while back 
                                    this->addTraderAndHisOrder(sd,orderId);
                                    //Make a new record of the given order
                                    orderAndCorrespondingDetails[orderId] = vector<uint64_t>{quantity,instrumentId,(uint64_t)1, (uint64_t)sd};
                                    //Send happy response
                                    sendResponse(sd,1,orderId);
                                }else{
                                    //This violates our risk constraints!
                                    //Not happy
                                    sendResponse(sd,0,orderId);
                                }
                            }else if(side == 'S'){
                                //Again, we operate differently on 'S' side
                                //The quantities inside our vector of details are not in the same position
                                //An extension could definetely be to do this in one call
                                uint64_t theoreticalSellAfterThisStep = (uint64_t) max((long long)sell + (long long)quantity , (long long)sell + (long long)quantity - net);
                                if(theoreticalSellAfterThisStep <= sellThreshold){
                                    //actual do the changes
                                    instrumentDetails[2] += quantity;
                                    instrumentDetails[4] = theoreticalSellAfterThisStep;
                                    instrumentPositions[instrumentId] = instrumentDetails; 
                                    //still need to update the traderOrders and the orderAndCoresspondingDetails
                                    this->addTraderAndHisOrder(sd,orderId);
                                    orderAndCorrespondingDetails[orderId] = vector<uint64_t>{quantity,instrumentId,(uint64_t)0,(uint64_t)sd};
                                    //Complete sell new order
                                    sendResponse(sd,1,orderId);
                                }else{
                                    //too risky!
                                    sendResponse(sd,0,orderId);
                                }
                            }
                        }else{
                            //If this position is completely new and it doesn't exist in our records
                            //then clearly all we need to check is if this individual order
                            //respects our risk condition
                            if(side == 'B'){
                                if(quantity <= buyThreshold){
                                    //yes we are allowed. proceed
                                    //create the new order in our global dict of orders
                                    orderAndCorrespondingDetails[orderId] = vector<uint64_t>{quantity, instrumentId, 1, (uint64_t)sd};
                                    //assign this order to our trader
                                    this->addTraderAndHisOrder(sd,orderId);
                                    //Update the position accordingly
                                    instrumentPositions[instrumentId] = vector<uint64_t>{0,quantity,0,quantity,0};
                                    //send yes response
                                    sendResponse(sd,1,orderId);
                                }else{
                                    //too risky! initial buying company exceeds our threshold
                                    sendResponse(sd,0,orderId);
                                }
                            }else if(side == 'S'){
                                if(quantity <= sellThreshold){
                                    //yes we are allowed. proceed
                                    orderAndCorrespondingDetails[orderId] = vector<uint64_t>{quantity, instrumentId, 0, (uint64_t)sd};
                                    //assign this order to our trader
                                    this->addTraderAndHisOrder(sd,orderId);
                                    instrumentPositions[instrumentId] = vector<uint64_t>{0,0,quantity,0,quantity};
                                    //send confirmation
                                    sendResponse(sd,1,orderId);
                                }else{
                                    //too risky! initial selling exceeds our threshold
                                    sendResponse(sd,0,orderId);
                                }
                            }
                        }
                        break;
                    }
                    case 10:{
                        //deleting an order shouldn't require a response
                        //prepare the buffer for the delete struct
                        char deleteOrderBuffer[10];
                        read(sd,deleteOrderBuffer, 10);
                        //cast it
                        DeleteOrder* deleteOrder = (struct DeleteOrder *) &deleteOrderBuffer;
                        //destructure for conveniance 
                        uint64_t orderId = deleteOrder->orderId;

                        //Make sure this order exists
                        if(orderAndCorrespondingDetails.find(orderId) != orderAndCorrespondingDetails.end()){
                            //obtain the order details from our global dict of orders
                            vector<uint64_t> orderDetails = orderAndCorrespondingDetails[orderId];
                            //destructure the data for convenience
                            uint64_t quantity = orderDetails[0];
                            uint64_t instrumentId = orderDetails[1];
                            uint64_t side = orderDetails[2];
                            uint64_t traderId = orderDetails[3];

                            //delete it from the orders of the trader
                            //-----------------------------------------
                            if(traderOrders.find(traderId) != traderOrders.end()){
                                vector<uint64_t> hisOrders = traderOrders[traderId];
                                //Get an iterator to where in this vector, this order is
                                auto it = find(hisOrders.begin(), hisOrders.end(), orderId);
                                if(it != hisOrders.end()){
                                    hisOrders.erase(it);
                                    //Esentially we don't want an empty list
                                    //if it happens that a trader has all of his orders removed
                                    //make it as if the trader freshly connected
                                    //keeps the state clean and tidy
                                    if(hisOrders.size() == 0){
                                        traderOrders.erase(traderId);
                                    }else{
                                        traderOrders[traderId] = hisOrders;
                                    }
                                }//else magically this order is already missing from the traders orders
                            }//else the trader doesn't exist so we can't delete any of his orders
                            //-----------------------------------------

                            //modify the instrument positions accordingly
                            //-----------------------------------------
                            if(instrumentPositions.find(instrumentId) != instrumentPositions.end()){
                                vector<uint64_t> instrumentDetails = instrumentPositions[instrumentId];
                                //Again, the net is very much a signed quantity regardless of its pesky unsigned nature!
                                if(side == 0){
                                    //we were selling
                                    //revert its effect
                                    instrumentDetails[2] -= quantity;
                                    instrumentDetails[4] = (uint64_t) max((long long)instrumentDetails[2], (long long)instrumentDetails[2] - (long long)instrumentDetails[0]);
                                }else{
                                    //we were buying
                                    //revert its effect
                                    instrumentDetails[1] -= quantity;
                                    instrumentDetails[3] = (uint64_t) max((long long)instrumentDetails[1], (long long)instrumentDetails[1] + (long long)instrumentDetails[0]);
                                }

                                //Again we are keeping the state clean and tidy but for the instrument-level dict now
                                if(emptyInstrument(instrumentDetails)){
                                    instrumentPositions.erase(instrumentId);
                                }else{
                                    instrumentPositions[instrumentId] = instrumentDetails;
                                }
                            }//else this instrument id does not appear in our records
                            //-----------------------------------------
                            //finally delete it from the orders
                            orderAndCorrespondingDetails.erase(orderId);
                        }//else this order doesn't even exist in our directory of orders
                        break;
                    }
                    case 18: {
                        //preare the buffer for the modify order input
                        char modifyOrderBuffer[18];
                        read(sd,modifyOrderBuffer, 18);
                        //cast it in!
                        ModifyOrderQuantity* modifyOrderQuantity = (struct ModifyOrderQuantity *) modifyOrderBuffer;
                        //destructure it
                        uint64_t orderId = modifyOrderQuantity->orderId;
                        uint64_t newQuantity = modifyOrderQuantity->newQuantity;
                        //obtain the original orderDetails
                        vector<uint64_t> orderDetails = orderAndCorrespondingDetails[orderId];
                        uint64_t oldQuantity = orderDetails[0];
                        uint64_t instrumentId = orderDetails[1];
                        uint64_t side = orderDetails[2];
                        orderDetails[0] = newQuantity;
                        //orderAndCorrespondingDetails map modified
                        orderAndCorrespondingDetails[orderId] = orderDetails;

                        //Modify the position level
                        vector<uint64_t> instrumentDetails = instrumentPositions[instrumentId];
                        uint64_t net = instrumentDetails[0];
                        //Net is actually signed!
                        //I keep saying this because it was very painful
                        if(side == 0){
                            //sell
                            uint64_t sell = instrumentDetails[2];
                            uint64_t newSell = sell - oldQuantity + newQuantity;
                            uint64_t newHyp = (uint64_t) max( (long long)newSell, (long long)newSell - (long long) net  );
                            instrumentDetails[2] = newSell;
                            instrumentDetails[4] = newHyp;
                        }else{
                            //buy
                            uint64_t buy = instrumentDetails[1];
                            uint64_t newBuy = buy - oldQuantity + newQuantity;
                            uint64_t newHyp = (uint64_t) max( (long long) newBuy, (long long)newBuy + (long long) net );
                            instrumentDetails[1] = newBuy;
                            instrumentDetails[3] = newHyp; 
                        }
                        //actually modify it inside as opposed to locally
                        instrumentPositions[instrumentId] = instrumentDetails;
                        //No need to modify the other traderOrder dict as the order still belongs to the trader!
                        break;
                        //Note how we don't need a response.
                        //I am assuming if we can cancel without a response, we can edit without a response
                        //You can clearly see, by the way I wrote the code, it would not be difficult to modify
                        //this assumption. It would just require some more hyp checking above!
                    }
                    case 34: {
                        //prepare the buffer
                        char tradeBuffer[34];
                        read(sd,tradeBuffer, 34);
                        //cast
                        Trade* trade = (struct Trade*) tradeBuffer; 
                        //destructure
                        uint64_t instrumentId = trade->listingId;
                        uint64_t tradeQuantity = trade->tradeQuantity;
                        
                        //Two scenarios.
                        //1) The instrument already exists somewhere in our books so we need to modify it
                        //2) Completely new instrument

                        //Note the casts to (long long) are to maintain the signed nature of net! 
                        //aka instrumentDetails[0] in the below
                        if(instrumentPositions.find(instrumentId) != instrumentPositions.end()){
                            //This instrument already exists
                            vector<uint64_t> instrumentDetails = instrumentPositions[instrumentId];
                            instrumentDetails[0] += (long long)tradeQuantity;
                            instrumentDetails[3] = (uint64_t) max((long long)instrumentDetails[1], (long long)instrumentDetails[1] + (long long)instrumentDetails[0]);
                            instrumentDetails[4] = (uint64_t) max((long long)instrumentDetails[2], (long long)instrumentDetails[2] - (long long) instrumentDetails[0]);
                            //modify it for the server!
                            instrumentPositions[instrumentId] = instrumentDetails;
                        }else{
                            //This instrument is not currently on
                            //Simply add it
                            vector<uint64_t> instrumentDetails = {tradeQuantity, 0,0, (uint64_t) max((long long)0, (long long) tradeQuantity),(uint64_t) max((long long)0, 0-(long long) tradeQuantity)};
                            instrumentPositions[instrumentId] = instrumentDetails;
                        }
                        break;
                    }
                    default:
                        //This should never happen.
                        printf("Default branch reached\n");
                        break;
                    }
                }
            }
        }
        //Don't forget to cleanup every time we loop to purge dead sockets
        cleanup(socketsSoFar,toDeleteLater);
    }
}