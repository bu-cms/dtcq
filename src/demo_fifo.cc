#include<include/FIFO.h>
#include <DSPatch.h>
#include <ctime>
#include <iostream>
using namespace std;
using namespace DSPatch;
int main() {
    auto circuit = std::make_shared<Circuit>();

    // std::shared_ptr<Component> fifo(new FIFO<uint64_t>());

    FIFO<int> f;
    auto fifo = std::make_shared<FIFO<uint64_t>>();
    circuit->AddComponent(fifo);

    // circuit->ConnectOutToIn(randint, 0, ebf, 0 );
    // circuit->ConnectOutToIn(randint, 0, printint1, 0 );
    // circuit->ConnectOutToIn(ebf, 0, printint2, 0 );


    // // Circuit tick method 1: Manual
    // for ( int i = 0; i < 100; ++i )
    // {
    //     circuit->Tick();
    //     cout << endl;
    // }

}