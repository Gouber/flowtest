#include <stdint.h>

struct Header
{
 uint16_t version; // Protocol version
 uint16_t payloadSize; // Payload size in bytes
 uint32_t sequenceNumber; // Sequence number for this package
 uint64_t timestamp; // Timestamp, number of nanoseconds from Unix epoch.
} __attribute__ ((__packed__));
static_assert(sizeof(Header) == 16, "The Header size is not correct");


struct NewOrder
{
 static constexpr uint16_t MESSAGE_TYPE = 1;
 uint16_t messageType; // Message type of this message
 uint64_t listingId; // Financial instrument id associated to this message
 uint64_t orderId; // Order id used for further order changes
 uint64_t orderQuantity; // Order quantity
 uint64_t orderPrice; // Order price, the price contains 4 implicit decimals
 char side; // The side of the order, 'B' for buy and 'S' for sell
} __attribute__ ((__packed__));
static_assert(sizeof(NewOrder) == 35, "The NewOrder size is not correct");

struct DeleteOrder
{
 static constexpr uint16_t MESSAGE_TYPE = 2;
 uint16_t messageType; // Message type of this message
 uint64_t orderId; // Order id that refers to the original order id
} __attribute__ ((__packed__));
static_assert(sizeof(DeleteOrder) == 10, "The DeleteOrder size is not correct");

struct ModifyOrderQuantity
{
 static constexpr uint16_t MESSAGE_TYPE = 3;
 uint16_t messageType; // Message type of this message
 uint64_t orderId; // Order id that refers to the original order id
 uint64_t newQuantity; // The new quantity
} __attribute__ ((__packed__));
static_assert(sizeof(ModifyOrderQuantity) == 18, "The ModifyOrderQuantity size is not correct");

struct Trade
{
 static constexpr uint16_t MESSAGE_TYPE = 4;
 uint16_t messageType; // Message type of this message
 uint64_t listingId; // Financial instrument id associated to this message
 uint64_t tradeId; // Order id that refers to the original order id
 uint64_t tradeQuantity; // Trade quantity
 uint64_t tradePrice; // Trade price, the price contains 4 implicit decimals
} __attribute__ ((__packed__));
static_assert(sizeof(Trade) == 34, "The Trade size is not correct");


struct OrderResponse
{
 static constexpr uint16_t MESSAGE_TYPE = 5;
 enum class Status : uint16_t
 {
 ACCEPTED = 0,
 REJECTED = 1,
 };
 uint16_t messageType; // Message type of this message
 uint64_t orderId; // Order id that refers to the original order id
 Status status; // Status of the order
} __attribute__ ((__packed__));
static_assert(sizeof(OrderResponse) == 12, "The OrderResponse size is not correct");


struct NewOrderWithHeader{
    Header header;
    NewOrder newOrder;
}__attribute__((__packed__));
static_assert(sizeof(NewOrderWithHeader) == 51, "Not correct");

struct DeleteOrderWithHeader{
    Header header;
    DeleteOrder deleteOrder;
}__attribute__((__packed__));
static_assert(sizeof(DeleteOrderWithHeader) == 26, "Not correct");

struct ModifyOrderQuantityWithHeader{
    Header header;
    ModifyOrderQuantity modifyOrderQuantity;
}__attribute__((__packed__));
static_assert(sizeof(ModifyOrderQuantityWithHeader) == 34, "Not correct");

struct TradeWithHeader{
    Header header;
    Trade trade;
}__attribute__((__packed__));
static_assert(sizeof(TradeWithHeader) == 50, "Not correct");
