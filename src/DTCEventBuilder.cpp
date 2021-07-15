#include <interface/DTCEventBuilder.h>

using namespace std;

DTCEventBuilder::DTCEventBuilder(int _nchips, int output_links) : Component(), in_data_valid(_nchips), in_data(_nchips), in_control_valid(_nchips), in_control(_nchips), out_read_data(_nchips), out_read_control(_nchips),
words_to_read(_nchips, 0), 
control_full_event(_nchips, false), 
read_control_last_time(_nchips, false), 
read_data_last_time(_nchips, false), 
buffer_counter(_nchips, 0),
control_new_event_header(_nchips, false),
OUTPUT_LINKS(output_links) {
    nchips = _nchips;
    WORD_PER_CLOCK_TICK_TO_SEND_EVENT = OUTPUT_LINKS;// equals to number of output links with 25GB/s speed. By design this can be up to 16.
    for (int ichip=0; ichip<nchips; ichip++) {
        add_output( &(out_read_data[ichip]) );
        add_output( &(out_read_control[ichip]) );
    }
    add_output( &(out_event_ready) );
};

void DTCEventBuilder::tick() {
    clock_ticks_counter ++;
    if (remaining_time_to_send_last_event > 0) remaining_time_to_send_last_event--;
    if (out_event_ready.get_value()) out_event_ready.set_value(false);
    for (int ichip=0; ichip<nchips; ichip++) {
        if (in_data_valid[ichip].get_value() == true) {
            buffer_counter[ichip] += 1;
            words_to_read[ichip] -= 1;
            assert(words_to_read[ichip] >= 0);
        }
        if (in_control_valid[ichip].get_value() == true){
            //std::bitset<16> x(in_control[ichip].get_value());
            //std::cout<<"chip "<<ichip<<" control value = "<<x<<std::endl;
            assert(!control_full_event[ichip]);
            if ( (in_control[ichip].get_value() & (((uint16_t)1<<15))) && (buffer_counter[ichip]>0)) {
                control_full_event[ichip] = true;
                control_new_event_header[ichip] = true;
            }
            else {
                words_to_read[ichip] += 1;
            }
        }
        bool read_data_this_time = (words_to_read[ichip]>1) || (words_to_read[ichip]==1 && !read_data_last_time[ichip]);
        out_read_data[ichip].set_value( read_data_this_time );
        read_data_last_time[ichip] = read_data_this_time;
        out_read_control[ichip].set_value( (!control_full_event[ichip]) && (!read_control_last_time[ichip]) );
        read_control_last_time[ichip]=( (!control_full_event[ichip]) && (!read_control_last_time[ichip]) ); // avoid consecutive control read, otherwise might read more word than needed due to signal delay.
    }
    //std::cout<<"EBD is full event status = ";
    //for (auto i : control_full_event) std::cout<<i;
    //std::cout<<std::endl;
    if (std::all_of(control_full_event.begin(), control_full_event.end(), [](bool v){return v;} ) && std::all_of(words_to_read.begin(), words_to_read.end(), [](int i){return i==0;} ) && (remaining_time_to_send_last_event == 0)) { // if all chips have full data for the event, and the last event has been sent out
        if (WORD_PER_CLOCK_TICK_TO_SEND_EVENT>0) {
            remaining_time_to_send_last_event = int(std::accumulate(buffer_counter.begin(), buffer_counter.end(), 0)/WORD_PER_CLOCK_TICK_TO_SEND_EVENT);
        }
        out_event_ready.set_value(true);
        int maximum_number_of_words = *max_element(buffer_counter.begin(), buffer_counter.end());
        std::cout<<"New event processed after "<<clock_ticks_counter<<" clock ticks! with maximum number of words per chip = "<<maximum_number_of_words;
        std::cout<<" Will take "<<remaining_time_to_send_last_event<<" clock ticks to send it out."<<std::endl;
        clock_ticks_counter = 0;
        for (int ichip=0; ichip<nchips; ichip++) {
            control_full_event[ichip] = false;
            buffer_counter[ichip] = 0;
            if (control_new_event_header[ichip]) {
                control_new_event_header[ichip] = false;
                words_to_read[ichip] ++;
            }
        }
    }
}
