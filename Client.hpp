
#include <inttypes.h>
#include "Definitions.hpp"


class Client{
    int sockfd;
    Header* h;
    void headerUpdate();

    public:
        Client(int portNumber);
        bool sendOrder(uint64_t instrumentId, uint64_t orderId, uint64_t quantity, uint64_t orderPrice, char side);
        void deleteOrder(uint64_t orderId);
        void modifyOrder(uint64_t orderId, uint64_t newQuantity);
        void trade(uint64_t instrumentId, uint64_t tradeId, uint64_t quantity, uint64_t price);
        void disconnect();

};