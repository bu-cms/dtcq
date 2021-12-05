#include <interface/ChipDataPlayer.h>

using namespace std;

ChipDataPlayer::ChipDataPlayer(int _nchips, vector<vector<unsigned short>> _vec_event_chip_sizes, vector<vector<unsigned short>> _vec_event_chip_parse_time, vector<float> elink_chip_ratio, int _NE, bool is_random_l1, bool use_trigger_rule) : 
    Component(), out_read(_nchips), out_data(_nchips),
    RANDOM_L1(is_random_l1), TRIGGER_RULE(use_trigger_rule),
    max_event_idx(_vec_event_chip_sizes.size()), new_event_flag(_nchips, true),
    vec_event_chip_sizes(_vec_event_chip_sizes), NE(_NE),
    remaining_bits_for_triggered_events(_nchips),
    queued_empty_event(_nchips), vec_event_chip_parse_time(_vec_event_chip_parse_time)
    {
    assert(_nchips == vec_event_chip_sizes[0].size());
    assert(_nchips == elink_chip_ratio.size());
    nchips = _nchips;
    for (int ichip=0; ichip<nchips; ichip++) {
        add_output( &(out_read[ichip]) );
        add_output( &(out_data[ichip]) );
        assert( elink_chip_ratio[ichip]>0 );
        ticks_per_word.push_back( int(1.0*ticks_per_word_per_elink/elink_chip_ratio[ichip]) );
    }
    // edit bunch_not_empty according to LHC filling scheme
    bool* position_in_orbit = bunch_not_empty;
    int non_empty_bunches = 0;
    for (int i=0;i<3;i++){
        for (int j=0; j<2; j++) {
            for (int k=0; k<3; k++) {
                std::fill(position_in_orbit, position_in_orbit+72, true);
                position_in_orbit+=72;
                non_empty_bunches+=72;
                position_in_orbit+=8;
            }
            position_in_orbit+=30;
        }
        for (int k=0; k<4; k++) {
            std::fill(position_in_orbit, position_in_orbit+72, true);
            position_in_orbit+=72;
            non_empty_bunches+=72;
            position_in_orbit+=8;
        }
        position_in_orbit+=31;
    }
    for (int j=0; j<3; j++) {
        for (int k=0; k<3; k++) {
            std::fill(position_in_orbit, position_in_orbit+72, true);
            position_in_orbit+=72;
            non_empty_bunches+=72;
            position_in_orbit+=8;
        }
        position_in_orbit+=30;
    }
    position_in_orbit+=81;
    assert( position_in_orbit - bunch_not_empty == bunches_per_orbit );
    // rescale the ticks per event to make final trigger rate being 750kHz
    ticks_per_event = 400*1000*non_empty_bunches/bunches_per_orbit/trigger_rate;
};

void ChipDataPlayer::tick() {
    // First part check if new event is triggered
    if (RANDOM_L1) {
        // Check trigger every 25ns (10 clock ticks) at bunch crossings
        if (nticks%10==0) {
            assert(nbunch<bunches_per_orbit);
            // Implemented trigger rule: no more than 8 triggers 130 bunch crossings
            for (int i=0; i<time_since_recent_L1As.size(); i++) time_since_recent_L1As[i]++;
            if (time_since_recent_L1As.size()>0 && time_since_recent_L1As.front()>trigger_rule_bunch_period) time_since_recent_L1As.pop_front();
            // first event always trigger, otherwise depends on the toss and trigger rule
            //if ((time_since_recent_L1As.size()<trigger_rule_max_L1As && bunch_not_empty[nbunch] && rand()%int(ticks_per_event/10)==0) || (nticks==0)) {
            if ((bunch_not_empty[nbunch] && rand()%int(ticks_per_event/10)==0) || (nticks==0)) {
                potential_trigger_counts += 1;
                if (time_since_recent_L1As.size()>=trigger_rule_max_L1As) {
                    blocked_trigger_counts += 1;
                    cout<<"Blocked trigger according to trigger rule, current ratio = "<<blocked_trigger_counts/potential_trigger_counts<<endl;
                }
                else {
                    triggered_events++;
                    int triggered_event_idx = rand() % max_event_idx;
                    // load the event size per chip for this event idx
                    for (int ichip=0; ichip<nchips; ichip++) {
                        remaining_bits_for_triggered_events[ichip].push_back(vec_event_chip_sizes[triggered_event_idx][ichip]);
                        queued_chip_parse_time[ichip].push_back(vec_event_chip_parse_time[triggered_event_idx][ichip]);
                    }
                    if (TRIGGER_RULE) time_since_recent_L1As.push_back(0);
                }
            }
            nbunch ++;
            if (nbunch == bunches_per_orbit) nbunch = 0;
        }
    }
    else {
        throw std::runtime_error("Flat trigger rate unimplemented.");
    }

    // Check if link has enough bandwidth to send next event's data
    for (int ichip=0; ichip<nchips; ichip++) {
        bool trigger_condition = (remaining_bits_for_triggered_events[ichip].size()>0);
        // simulates when e-link has capacity to transfer a new word
        if (nticks%ticks_per_word[ichip]==0) {
            // when padding < 8 bits, a new empty event with all zeroes is needed
            // if this is what happened last time, will send an empty event this time
            if (queued_empty_event[ichip]) {
                value = 0ull;
                queued_empty_event[ichip] = false;
                out_read[ichip].set_value(true);
                out_data[ichip].set_value(value);
            }
            // Otherwise try to read data belong to the next event
            else if (remaining_bits_for_triggered_events[ichip].size()>0) {
                // construct a 64 bits data word with:
                // 1 bit: has new event boundary
                // 2-8 bits: number of event boundaries
                // k*8 + (1-8) bits: parsing time for events starting at the kth event boundary
                // Note this is not the actual data format, just for convenience in simulation
                bool has_event_boundary=false;
                uint8_t number_of_boundaries=0;
                vector<uint8_t> parsing_time_per_boundaries(7,0);
                unsigned short remaining_bits_to_read = 64;
                while (remaining_bits_to_read>0 && remaining_bits_for_triggered_events[ichip].size()>0) {
                    if (new_event_flag[ichip]) {
                        has_event_boundary = true;
                        parsing_time_per_boundaries[number_of_boundaries] = queued_chip_parse_time[ichip].front();
                        queued_chip_parse_time[ichip].pop_front();
                        number_of_boundaries += 1;
                        new_event_flag[ichip] = false;
                    }
                    int read_bits = min(remaining_bits_to_read, remaining_bits_for_triggered_events[ichip].front());
                    remaining_bits_for_triggered_events[ichip].front() -= read_bits;
                    if (remaining_bits_for_triggered_events[ichip].front()==0) {
                        remaining_bits_for_triggered_events[ichip].pop_front();
                        new_event_flag[ichip] = true;
                    }
                    remaining_bits_to_read -= read_bits;
                }
                value = ((uint64_t)has_event_boundary)<<7;
                value = value | number_of_boundaries;
                for (uint8_t parsing_time_iboundary : parsing_time_per_boundaries) {
                    value = value << 8;
                    value = value | parsing_time_iboundary;
                }
                cerr<<"has_event_boundary="<<has_event_boundary<<"number_of_boundaries="<<number_of_boundaries<<endl;
                cerr<<bitset<64>(value)<<endl;
                out_read[ichip].set_value(true);
                out_data[ichip].set_value(value);
            }
            // If there is no triggered event, no signal sent
            else {
                out_read[ichip].set_value(false);
                out_data[ichip].set_value(0ull);
            }
        }
        // no signal sent if link un-available
        else {
            out_read[ichip].set_value(false);
            out_data[ichip].set_value(0ull);
        }
    }// end for loop enumerating all chips
    nticks ++;
}
