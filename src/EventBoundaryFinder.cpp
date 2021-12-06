#include<iostream>
#include<bitset>
#include<interface/EventBoundaryFinder.h>

EventBoundaryFinder::EventBoundaryFinder(bool _do_parse) : do_parse(_do_parse) {
	add_output(&out_fifo_i1_pop );
	add_output(&out_fifo_o1_read);
	add_output(&out_fifo_o1_data);
	add_output(&out_fifo_o2_read);
	add_output(&out_fifo_o2_data);
	in_enable_fifo_i1_data_pop.set_value(true); //can be controled by others, otherwise not used
};

void EventBoundaryFinder::tick() {
    out_fifo_i1_pop.set_value(false);
    out_fifo_o2_read.set_value(false);
    out_fifo_o1_read.set_value(false);
    out_fifo_o1_data.set_value(0);
    out_fifo_o2_data.set_value(0);

    if (queued_data_words.size()>0) {
        uint64_t in_data = queued_data_words.front();
        uint16_t control_word = queued_control_words.front();
        out_fifo_o2_read.set_value(true);
        out_fifo_o1_read.set_value(true);
        out_fifo_o1_data.set_value(in_data);
        out_fifo_o2_data.set_value(control_word);
        queued_data_words.pop();
        queued_control_words.pop();
        return;
    }
    if (halt_time>0) {
        halt_time--;
        return;
    }

    out_fifo_i1_pop.set_value(in_enable_fifo_i1_data_pop.get_value());

    if(not in_fifo_i1_data_valid.get_value()) return;
    

    auto in_data = in_fifo_i1_data.get_value();

    // Fill output data FIFO
    out_fifo_o1_data.set_value(in_data);
    out_fifo_o1_read.set_value(true);

    // Fill output control FIFO
    uint16_t control_word = 0;
    // Leading bit in control word indicates new event boundary
    // Second bit indicates event boundary at beginning
    // 3-8 bit indicate location of event boundary
    // 9-16 bit is the event tag
    // For now only using the first 2 bits

    // New event indicated by new stream bit
    if(in_data & (((uint64_t)1)<<63)) {
        // Second Leading bit in control word indicates event boundary
        control_word |= ((uint16_t) 3) << 14;
        uint8_t n_boundaries = ((uint8_t)0xff)>>1; // to capture 2-8 bits
        n_boundaries = n_boundaries & (in_data>>56);
        //cout<<"# "<<(int)n_boundaries<<" #"<<endl;
        assert(n_boundaries<=5); // unlikely but 5 events with 11 bits each?
        for (uint8_t iboundary=0; iboundary<n_boundaries; iboundary++) {
            // append halt time related to each new event
            if (do_parse) halt_time += (in_data>>(48-8*iboundary)) & ((uint64_t) 0xff);
            // for more than 1 event boundary in the same word, duplicate the 
            // data and control words and send later
            if (iboundary>0) {
                queued_data_words.push(in_data);
                queued_control_words.push( ((uint16_t)1)<<15 );
            }
        }
        out_fifo_i1_pop.set_value(false);
    }
    bitset<64> b_data(in_data);
    bitset<16> b_control(control_word);
    //std::cout<<"EBF: data="<<b_data<<std::endl;
    //std::cout<<"EBF: control="<<b_control<<std::endl;

    out_fifo_o2_data.set_value(control_word);
    out_fifo_o2_read.set_value(true);


};
