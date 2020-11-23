#include "Server.hpp"



int main(int argc, char* argv[]){
    int buyMax = atoi(argv[1]);
    int sellMax = atoi(argv[2]);
    Server* server = new Server(buyMax, sellMax);
    server->start();
    return -1;
}