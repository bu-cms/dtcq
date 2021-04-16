#include<interface/EventBoundaryFinderStreamAligned.h>
void EventBoundaryFinderStreamAligned::Process_(SignalBus const & inputs, SignalBus& outputs){

    // Always read from input FIFO
    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_I1_POP, true);

    // Default situation: Do not write to output FIFOs.
    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_O1_READ, false);
    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_O2_READ, false);

    auto in_data_valid = inputs.GetValue<bool>(EventBoundaryFinder::INPUT::IN_FIFO_I1_DATA_VALID);
    if(not in_data_valid) return;
    if(not *in_data_valid) return;

    auto in_data = inputs.GetValue<uint64_t>(EventBoundaryFinder::INPUT::IN_FIFO_I1_DATA);
    if(not in_data) return;


    // Fill output data FIFO
    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_O1_DATA, *in_data);
    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_O1_READ, true);

    // Fill output control FIFO
    uint8_t control_word = 0;
    // Leading bit in control word indicates new data word
    control_word |= ((uint8_t) 1) << 7;

    // New event indicated by new stream bit
    if((*in_data & ((uint64_t)1)<<63) == *in_data) {
        // Second Leading bit in control word indicates event boundary
        control_word |= ((uint8_t) 1) << 6;
    }


    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_O2_DATA, control_word);
    outputs.SetValue(EventBoundaryFinder::OUTPUT::OUT_FIFO_O2_READ, true);


};