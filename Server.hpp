
#include <inttypes.h>
#include <vector>
#include <unordered_map>
#include <inttypes.h>


using namespace std;


class Server{

    //default values
    int buyMax = 20;
    int sellMax = 15;

    //We use this map to check our packets arrive in order
    //Note certain assumptions were made (which given the nature of the code, can easily be changed)
    //See inside Server.cpp these assumptions
    unordered_map<int, vector<uint64_t>> consistencyChecker;

    //A map where every entry represents the specific details about an instrument position that we need
    //entry: instrumentId
    //key: Net, Buy, Sell, Hyp Buy, Hyp Sell 
    unordered_map<uint64_t, vector<uint64_t>> instrumentPositions;

    //A map where we keep track of every order a trader did
    //key: traderId (aka his socket descriptor)
    //entry: {order1, order2, ....}
    unordered_map<int, vector<uint64_t>> traderOrders;

    //A map where we keep track of what each order is
    //entry: orderId
    //key: quantity, instrumentId, sell/buy (0/1), traderId
    unordered_map<uint64_t, vector<uint64_t>> orderAndCorrespondingDetails;

    void addTraderAndHisOrder(int sd, uint64_t orderId);
    void addSocket(vector<int>& socketsSoFar, int socketid);
    void sendResponse(int sd, int yesOrNo, uint64_t orderId);
    void cleanup(vector<int>& socketsSoFar, vector<int>& socketid);

    public:
        Server(int buyMax, int sellMax);
        Server(){};
        void start();
};