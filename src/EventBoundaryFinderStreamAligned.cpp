#include<iostream>
#include<interface/EventBoundaryFinderStreamAligned.h>
void EventBoundaryFinderStreamAligned::Process_(SignalBus const & inputs, SignalBus& outputs){
    // Default: push no output
    out_fifo_o2_read = false;
    out_fifo_o1_read = false;
    out_fifo_o1_data = 0;
    out_fifo_o2_data = 0;

    auto in_data_valid = inputs.GetValue<bool>(EventBoundaryFinder::INPUT::IN_FIFO_I1_DATA_VALID);
    if(not in_data_valid) return;
    if(not *in_data_valid) return;

    auto in_data = inputs.GetValue<uint64_t>(EventBoundaryFinder::INPUT::IN_FIFO_I1_DATA);
    if(not in_data) return;

    // Fill output data FIFO
    out_fifo_o1_data = *in_data;
    out_fifo_o1_read = true;

    // Fill output control FIFO
    uint8_t control_word = 0;
    // Leading bit in control word indicates new data word
    control_word |= ((uint8_t) 1) << 7;

    // New event indicated by new stream bit
    if((*in_data & ((uint64_t)1)<<63) == *in_data) {
        // Second Leading bit in control word indicates event boundary
        control_word |= ((uint8_t) 1) << 6;
    }

    out_fifo_o2_data = control_word;
    out_fifo_o2_read = true;

};
