#include <interface/ChipDataPlayer.h>

using namespace std;

ChipDataPlayer::ChipDataPlayer(vector<vector<unsigned short>> _vec_event_chip_sizes, int _nchips, vector<float> elink_chip_ratio, bool is_random_l1, bool use_trigger_rule) : 
    Component(), out_read(_nchips), out_data(_nchips),
    RANDOM_L1(is_random_l1), TRIGGER_RULE(use_trigger_rule),
    max_event_idx(_vec_event_chip_sizes.size()), new_event_flag(_nchips, true),
    vec_event_chip_sizes(_vec_event_chip_sizes),
    remaining_words_for_triggered_events(_nchips)
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
    if (RANDOM_L1) {
        // Check trigger every 25ns (10 clock ticks) at bunch crossings
        // Implemented trigger rule: no more than 8 triggers 130 bunch crossings
        if (nticks%10==0) {
            assert(nbunch<bunches_per_orbit);
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
                        remaining_words_for_triggered_events[ichip].push_back(
                                vec_event_chip_sizes[triggered_event_idx][ichip]/64 + (vec_event_chip_sizes[triggered_event_idx][ichip] % 64 != 0)
                                );
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
    for (int ichip=0; ichip<nchips; ichip++) {
        // Condition to read a new word: 1. File not at EOF; 2. matches ticks_per_word to be consistent with e-link speed 3. Does not exceed trigger rate
        bool trigger_condition = (remaining_words_for_triggered_events[ichip].size()>0);
        if ( (nticks%ticks_per_word[ichip]==0) && trigger_condition) {
            if (new_event_flag[ichip]) {
                value = 1ull<<63;
                new_event_flag[ichip]=false;
            }
            else {
                value = 0ull;
            }
            remaining_words_for_triggered_events[ichip][0] -= 1;
            if (remaining_words_for_triggered_events[ichip][0]<=0) {
                remaining_words_for_triggered_events[ichip].pop_front();
                new_event_flag[ichip]=true;
            }
            out_read[ichip].set_value(true);
            out_data[ichip].set_value(value);
            continue;
        }
        out_read[ichip].set_value(false);
        out_data[ichip].set_value(0ull);
    }
    nticks ++;
}
