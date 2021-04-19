#include<iostream>
#include<interface/EventBoundaryFinder.h>
using namespace DSPatch;

EventBoundaryFinder::EventBoundaryFinder() {
    SetInputCount_(INPUT::COUNT_IN);
    SetOutputCount_(OUTPUT::COUNT_OUT);
};

void EventBoundaryFinder::ProcessOutput_(SignalBus & outputs){
    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_I1_POP, out_fifo_i1_pop);

    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_O1_DATA, out_fifo_o1_data);
    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_O1_READ, out_fifo_o1_read);

    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_O2_DATA, out_fifo_o2_data);
    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_O2_READ, out_fifo_o2_read);
};