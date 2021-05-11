#include<iostream>
#include<interface/EventBoundaryFinder.h>

EventBoundaryFinder::EventBoundaryFinder() {
	add_output(&out_fifo_i1_pop );
	add_output(&out_fifo_o1_read);
	add_output(&out_fifo_o1_data);
	add_output(&out_fifo_o2_read);
	add_output(&out_fifo_o2_data);
};

void EventBoundaryFinder::tick() {
    out_fifo_i1_pop.set_value(true);
    out_fifo_o2_read.set_value(false);
    out_fifo_o1_read.set_value(false);
    out_fifo_o1_data.set_value(0);
    out_fifo_o2_data.set_value(0);

    if(not in_fifo_i1_data_valid.get_value()) return;

    auto in_data = in_fifo_i1_data.get_value();

    // Fill output data FIFO
    out_fifo_o1_data.set_value(in_data);
    out_fifo_o1_read.set_value(true);

    // Fill output control FIFO
    uint8_t control_word = 0;
    // Leading bit in control word indicates new data word
    control_word |= ((uint8_t) 1) << 7;

    // New event indicated by new stream bit
    if(in_data & (((uint64_t)1)<<63)) {
        // Second Leading bit in control word indicates event boundary
        control_word |= ((uint8_t) 1) << 6;
    }

    out_fifo_o2_data.set_value(control_word);
    out_fifo_o2_read.set_value(true);

};
