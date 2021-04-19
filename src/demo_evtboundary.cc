#include<include/FIFO.h>
#include <DSPatch.h>
#include <ctime>
#include <iostream>
#include <interface/EventBoundaryFinderStreamAligned.h>
using namespace std;
using namespace DSPatch;

typedef FIFO<uint64_t> FIFO64;
typedef FIFO<uint8_t> FIFO8;
int main() {
    auto circuit = std::make_shared<Circuit>();
    auto fifo_i1 = std::make_shared<FIFO64>();
    auto fifo_o1 = std::make_shared<FIFO64>();
    auto fifo_o2 = std::make_shared<FIFO8>();
    // auto ebf     = std::make_shared<EventBoundaryFinderStreamAligned>();
    // circuit->AddComponent(fifo_i1);
    // circuit->AddComponent(fifo_o1);
    // circuit->AddComponent(fifo_o2);
    // circuit->AddComponent(ebf);

    // // Input FIFO <-> Boundary finder
    // circuit->ConnectOutToIn(fifo_i1, FIFO64::OUTPUT::OUT_DATA,       ebf, EventBoundaryFinder::INPUT::IN_FIFO_I1_DATA );
    // circuit->ConnectOutToIn(fifo_i1, FIFO64::OUTPUT::OUT_DATA_VALID, ebf, EventBoundaryFinder::INPUT::IN_FIFO_I1_DATA_VALID);
    // circuit->ConnectOutToIn(ebf,     EventBoundaryFinder::OUTPUT::OUT_FIFO_I1_POP, fifo_i1, FIFO64::INPUT::IN_POP);

    // // Boundary finder -> output data FIFO
    // circuit->ConnectOutToIn(ebf, EventBoundaryFinder::OUTPUT::OUT_FIFO_O1_DATA, fifo_o1, FIFO64::INPUT::IN_DATA);
    // circuit->ConnectOutToIn(ebf, EventBoundaryFinder::OUTPUT::OUT_FIFO_O1_READ, fifo_o1, FIFO64::INPUT::IN_READ);

    // // Boundary finder -> output control FIFO
    // circuit->ConnectOutToIn(ebf, EventBoundaryFinder::OUTPUT::OUT_FIFO_O2_DATA, fifo_o2, FIFO8::INPUT::IN_DATA);
    // circuit->ConnectOutToIn(ebf, EventBoundaryFinder::OUTPUT::OUT_FIFO_O2_READ, fifo_o2, FIFO8::INPUT::IN_READ);

    // // Circuit tick method 1: Manual
    // for ( int i = 0; i < 100; ++i )
    // {
    //     circuit->Tick();
    // }

}