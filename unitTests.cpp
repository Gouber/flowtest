#include "Server.hpp"
#include "Client.hpp"
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std;

void startServer(){
    Server* s = new Server();
    s->start();
}

void startSocket(Client* c ){
    c = new Client(8080);
}

void test_riskOnBuyingSingleInstrument(){
    sleep(1);
    Client* c = new Client(8080);
    Client* d = new Client(8080);
    Client* e = new Client(8080);
    sleep(1);
    bool res1 = c->sendOrder(1,1,15,1,'B');
    bool res2 = d->sendOrder(1,2,5,30,'B');
    bool res3 = e->sendOrder(1,3,1,30,'B');
    c->disconnect();
    bool res4 = e->sendOrder(1,4,1,30,'B');
    bool res5 = e->sendOrder(1,5,14, 30 , 'B');
    bool res6 = e->sendOrder(1,6,1,30,'B');
    assert(res1 == 1);
    assert(res2 == 1);
    assert(res3 == 0);
    assert(res4 == 1);
    assert(res5 == 1);
    assert(res6 == 0);
    d->disconnect();
    e->disconnect();
    sleep(1);
}

void test_riskOnSellingSingleInstrument(){

    sleep(1);
    Client* c = new Client(8080);
    Client* d = new Client(8080);
    Client* e = new Client(8080);
    bool res1 = c->sendOrder(1,1,15,20,'S');
    bool res2 = d->sendOrder(1,2,1,20,'S');
    assert(res1 == 1);
    assert(res2 == 0);
    c->disconnect();
    e->disconnect();
    bool res3 = d->sendOrder(1,3,1,20,'S');
    sleep(1);
    bool res4 = d->sendOrder(1,4,14,20,'S');
    assert(res3 == 1);
    d->disconnect();
}

void test_riskOnBuyingAndSellingSingleInstrument(){

    sleep(1);
    Client* c = new Client(8080);
    Client* d = new Client(8080);

    bool res1 = c->sendOrder(1,1,5,20,'B');
    bool res2 = d->sendOrder(1,2,5,20,'S');
    bool res3 = c->sendOrder(1,3,15,20,'B');
    bool res4 = d->sendOrder(1,4,10,20,'S');
    bool res5 = c->sendOrder(1,5,1,20,'B');
    bool res6 = d->sendOrder(1,6,1,20,'S');

    assert(res1 == 1);
    assert(res2 == 1);
    assert(res3 == 1);
    assert(res4 == 1);
    assert(res5 == 0);
    assert(res6 == 0);
    c->disconnect();
    d->disconnect();
}

void test_riskOnBuyingAndSellingSingleInstrumentWithDisconnect(){

    sleep(1);
    Client* c = new Client(8080);
    Client* d = new Client(8080);

    bool res1 = c->sendOrder(1,1,20,20,'B');
    bool res2 = d->sendOrder(1,2,20,20,'B');
    bool res3 = d->sendOrder(1,3,15,20,'S');
    c->disconnect();
    bool res4 = d->sendOrder(1,4,20,20,'B');
    assert(res1 == 1);
    assert(res2 == 0);
    assert(res3 == 1);
    assert(res4 == 1);
    d->disconnect();
}

void test_riskOnBuyingMultipleInstrumentsWithDisconnect(){
        printf("A");

    sleep(1);
    Client* c = new Client(8080);
    Client* d = new Client(8080);
    sleep(1);
    bool res1 = c->sendOrder(1,1,20,20,'B');
    bool res2 = c->sendOrder(2,2,20,20,'B');
    bool res3 = c->sendOrder(3,3,20,20,'B');
    bool res4 = d->sendOrder(1,4,1,20,'B');
    bool res5 = d->sendOrder(2,5,1,20,'B');
    bool res6 = d->sendOrder(3,6,1,20,'B');
    c->disconnect();
    bool res7 = d->sendOrder(1,4,1,20,'B');
    bool res8 = d->sendOrder(2,5,1,20,'B');
    bool res9 = d->sendOrder(3,6,1,20,'B');
    assert(res1 == 1);
    assert(res2 == 1);
    assert(res3 == 1);
    assert(res4 == 0);
    assert(res5 == 0);
    assert(res6 == 0);
    assert(res7 == 1);
    assert(res8 == 1);
    assert(res9 == 1);
    d->disconnect();
}

void test_complexExample1(){

    sleep(1);
    Client* c = new Client(8080);
    Client* d = new Client(8080);
    //buy max
    bool res1 = c->sendOrder(1,1,20,20,'B');
    bool res2 = c->sendOrder(2,2,20,20,'B');
    bool res3 = c->sendOrder(3,3,20,20,'B');
    assert(res1 == 1);
    assert(res2 == 1);
    assert(res3 == 1);

    //check I cannot sell above theoretical limist
    bool res4 = c->sendOrder(1,4,20,20,'S');
    bool res5 = c->sendOrder(2,5,20,20,'S');
    bool res6 = c->sendOrder(3,6,20,20,'S');
    assert(res4 == 0);
    assert(res5 == 0);
    assert(res6 == 0);

    //sell max
    bool res7 = c->sendOrder(1,7,15,20,'S');
    bool res8 = c->sendOrder(2,8,15,20,'S');
    bool res9 = c->sendOrder(3,9,15,20,'S');
    assert(res7 == 1);
    assert(res8 == 1);
    assert(res9 == 1);

    //Check I cannot buy anything anymore
    bool res10 = c->sendOrder(3,10,1,10,'B');
    bool res11 = c->sendOrder(2,11,1,10,'B');
    bool res12 = c->sendOrder(1,12,1,10,'B');
    assert(res10 == 0);
    assert(res11 == 0);
    assert(res12 == 0);

    //Check I cannot sell anything anymore
    bool res13 = c->sendOrder(3,13,1,10,'S');
    bool res14 = c->sendOrder(2,14,1,10,'S');
    bool res15 = c->sendOrder(1,15,1,10,'S');
    assert(res13 == 0);
    assert(res14 == 0);
    assert(res15 == 0);

    //Check the other client cannot sell anything anymore
    bool res16 = d->sendOrder(3,16,1,10,'S');
    bool res17 = d->sendOrder(2,17,1,10,'S');
    bool res18 = d->sendOrder(1,18,1,10,'S');

    assert(res16 == 0);
    assert(res17 == 0);
    assert(res18 == 0);


    d->disconnect();
    //check disconnect of 'd' does not affect connection of 'c'
    bool res19 = c->sendOrder(3,19,1,10,'B');
    bool res20 = c->sendOrder(2,20,1,10,'B');
    bool res21 = c->sendOrder(1,21,1,10,'B');
    assert(res19 == 0);
    assert(res20 == 0);
    assert(res21 == 0);

    bool res22 = c->sendOrder(3,22,1,10,'S');
    bool res23 = c->sendOrder(2,23,1,10,'S');
    bool res24 = c->sendOrder(1,24,1,10,'S');
    assert(res22 == 0);
    assert(res23 == 0);
    assert(res24 == 0);

    //finally check if a new client connects he can buy/sell a new instrument
    Client* f = new Client(8080);
    sleep(1);
    bool res25 = f->sendOrder(10,25,1,1,'B');

    bool res26 = f->sendOrder(10,26,1,10,'S');
    assert(res25 == 1);
    assert(res26 == 1);


    //check new client cannot buy/sell old "maxed-out" instrument
    bool res27 = f->sendOrder(3,27,1,1,'B');
    bool res28 = f->sendOrder(3,28,1,1,'S');
    assert(res27 == 0);
    assert(res28 == 0);
    c->disconnect();
    f->disconnect();
}


void test_cancellingSingleOrder(){
    sleep(1);
    Client* c = new Client(8080);
    bool res1 = c->sendOrder(1,1,10,10,'B');
    bool res2 = c->sendOrder(1,2,15,20,'B');
    assert(res1 == 1);
    assert(res2 == 0);
    c->deleteOrder(1);
    bool res3 = c->sendOrder(1,3,15,20,'B');
    assert(res3 == 1);
    c->disconnect();
}

void test_cancellingMultipleOrdersBuySide(){
    sleep(1);
    Client* c = new Client(8080);
    bool res1 = c->sendOrder(1,1,10,10,'B');
    bool res2 = c->sendOrder(1,2,5,10,'B');
    assert(res1 == 1);
    assert(res2 == 1);
    c->deleteOrder(1);
    bool res3 = c->sendOrder(1,3,15,10,'B');
    assert(res3 == 1);
    c->deleteOrder(3);
    c->deleteOrder(2);
    bool res4 = c->sendOrder(1,4,20,10,'B');
    assert(res4 == 1);
    c->disconnect();
}

void test_cancellingMultipleOrdersSellSide(){
    sleep(1);
    Client* c = new Client(8080);
    bool res1 = c->sendOrder(1,1,10,10,'S');
    bool res2 = c->sendOrder(1,2,5,10,'S');
    assert(res1 == 1);
    assert(res2 == 1);
    c->deleteOrder(1);
    bool res3 = c->sendOrder(1,3,10,10,'S');
    assert(res3 == 1);
    c->deleteOrder(3);
    c->deleteOrder(2);
    bool res4 = c->sendOrder(1,4,15,10,'S');
    assert(res4 == 1);
    c->disconnect();
}

void test_singleTradeGetsRecorded(){
    sleep(1);
    Client* c = new Client(8080);
    c->trade(4,1,20,20);
    bool res1 = c->sendOrder(4,1,16,20,'B');
    bool res2 = c->sendOrder(4,2,1,15,'S');
    assert(res1 == 0);
    //it should pass as it becomes max(1, 1 - 20)
    assert(res2 == 1);
    c->trade(4,1,-20,20);
    c->disconnect();
}

void test_givenExampleTest(){
    sleep(1);
    Client *c = new Client(8080);
    bool res1 = c->sendOrder(1,1,10,10,'B');
    bool res2 = c->sendOrder(2,2,15,15,'S');
    bool res3 = c->sendOrder(2,3,4,15,'B');
    bool res4 = c->sendOrder(2,4,20,15,'B');
    assert(res1 == 1);
    assert(res2 == 1);
    assert(res3 == 1);
    assert(res4 == 0);
    c->trade(2,1,-4,10);
    c->deleteOrder(3);
    c->disconnect();
}

void test_tradingInDeficit(){
    sleep(1);
    Client *c = new Client(8080);
    c->trade(1,1,-5,20);
    c-> trade(1,2,-25,20);
    bool res1 = c->sendOrder(1,1,10,10,'B');
    bool res2 = c->sendOrder(1,2,15,10,'S');
    c->trade(1,3,10,20);
    c->deleteOrder(1);
    assert(res1 == 1);
    assert(res2 == 0);
    Client *d = new Client(8080);
    bool res3 = d->sendOrder(1,3,20,1,'B');
    bool res4 = c->sendOrder(1,4,1,20,'S');
    assert(res3 == 1);
    assert(res4 == 0);
    d->disconnect();
    sleep(1);
    bool res5 = c->sendOrder(1,5,15,1,'B');
    assert(res5 == 1);
    c->disconnect();
}

void test_tradingInSurplus(){
    sleep(1);
    Client *c = new Client(8080);
    c->trade(1,1,50,20);
    //you cannot buy anything
    c->disconnect();
    sleep(1);
    //note that the exchange is "persistent?"
    Client *d = new Client(8080);
    bool res1 = d->sendOrder(1,1,20,20,'B');
    assert(res1 == 0);
    bool res2 = d->sendOrder(1,2,14,20,'S');
    assert(res2 == 1);
    d->deleteOrder(2);
    d->disconnect();
    Client *e = new Client(8080);
    bool res3 = e->sendOrder(1,3,15,20,'S');
    assert(res3 == 1);
    bool res4 = e->sendOrder(1,4,1,20,'S');
    assert(res4 == 0);
    e->disconnect();
    sleep(1);
}

void test_modifyingSingleOrderBuy(){
    sleep(1);
    Client *c = new Client(8080);
    bool res1 = c->sendOrder(1,1,20,20,'B');
    bool res2 = c->sendOrder(1,2,10,20,'S');
    assert(res1 == 1);
    assert(res2 == 1);
    c->modifyOrder(1,5);
    bool res3 = c->sendOrder(1,3,15,20,'B');
    assert(res3 == 1);
    c->disconnect();
    sleep(1);
}

void test_modifyingSingleOrderSell(){
    sleep(1);
    Client *c = new Client(8080);
    bool res1 = c->sendOrder(1,1,20,20,'B');
    bool res2 = c->sendOrder(1,2,10,20,'S');
    assert(res1 == 1);
    assert(res2 == 1);
    c->modifyOrder(2,15);
    bool res3 = c->sendOrder(1,3,1,1,'S');
    assert(res3 == 0);
    c->disconnect();
    Client *d = new Client(8080);
    bool res4 = d->sendOrder(1,4,11,1,'S');
    assert(res4 == 1);
    d->disconnect();
}




int main(){
    thread t1(startServer);    
    sleep(1);
    //Note because of the nature of the exchange trades never being reverted
    //Some of these tests intefere with one another
    //Hence you can run them in batches (see separation below)
    //Comment out all but a batch and see that it actually runs

    //Batch 1 
    //Basic functionality with some trades
    // test_riskOnBuyingSingleInstrument();
    // test_riskOnSellingSingleInstrument();
    // test_riskOnBuyingAndSellingSingleInstrument();
    // test_riskOnBuyingAndSellingSingleInstrumentWithDisconnect();
    // test_complexExample1();
    // test_cancellingSingleOrder();
    // test_cancellingMultipleOrdersBuySide();
    // test_cancellingMultipleOrdersSellSide();
    // test_singleTradeGetsRecorded();
    // test_givenExampleTest();
    // test_tradingInDeficit();
    // test_tradingInSurplus();


    //Batch 2 
    //Some more advanced trades
    test_modifyingSingleOrderBuy();
    test_modifyingSingleOrderSell();
}