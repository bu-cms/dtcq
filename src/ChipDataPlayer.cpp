#include <interface/ChipDataPlayer.h>

using namespace std;

ChipDataPlayer::ChipDataPlayer(vector<string> file_name_list, int _nchips, vector<float> elink_chip_ratio, bool is_random_l1, bool use_trigger_rule) : Component(), out_read(_nchips), out_data(_nchips), nevents(_nchips, 0), RANDOM_L1(is_random_l1), TRIGGER_RULE(use_trigger_rule) {
    assert(_nchips == file_name_list.size());
    assert(_nchips == elink_chip_ratio.size());
    nchips = _nchips;
    for (int ichip=0; ichip<nchips; ichip++) {
        add_output( &(out_read[ichip]) );
        add_output( &(out_data[ichip]) );
        assert( elink_chip_ratio[ichip]>0 );
        ticks_per_word.push_back( int(ticks_per_word_per_elink/elink_chip_ratio[ichip]) );
        std::ifstream* new_stream = new std::ifstream(file_name_list[ichip].c_str(), std::ios::binary);
        if (!bool(*new_stream)) throw std::invalid_argument((string("Unable to open file ")+file_name_list[ichip]).c_str());
        input_streams.push_back( new_stream );
    }
    // edit bunch_not_empty according to LHC filling scheme
    bool* position_in_orbit = bunch_not_empty;
    for (int i=0;i<3;i++){
        for (int j=0; j<2; j++) {
            for (int k=0; k<3; k++) {
                std::fill(position_in_orbit, position_in_orbit+72, true);
                position_in_orbit+=72;
                position_in_orbit+=8;
            }
            position_in_orbit+=30;
        }
        for (int k=0; k<4; k++) {
            std::fill(position_in_orbit, position_in_orbit+72, true);
            position_in_orbit+=72;
            position_in_orbit+=8;
        }
        position_in_orbit+=31;
    }
    for (int j=0; j<3; j++) {
        for (int k=0; k<3; k++) {
            std::fill(position_in_orbit, position_in_orbit+72, true);
            position_in_orbit+=72;
            position_in_orbit+=8;
        }
        position_in_orbit+=30;
    }
    position_in_orbit+=81;
    assert( position_in_orbit - bunch_not_empty == bunches_per_orbit );
};

void ChipDataPlayer::tick() {
    if (RANDOM_L1) {
        // Check trigger every 25ns (10 clock ticks) at bunch crossings
        // Implemented trigger rule: no more than 8 triggers 130 bunch crossings
        if (nticks%10==0) {
            assert(nbunch<bunches_per_orbit);
            for (int i=0; i<time_since_recent_L1As.size(); i++) time_since_recent_L1As[i]++;
            if (time_since_recent_L1As.size()>0 && time_since_recent_L1As.front()>trigger_rule_bunch_period) time_since_recent_L1As.pop_front();
            int new_rand = rand();
            if (time_since_recent_L1As.size()<trigger_rule_max_L1As && bunch_not_empty[nbunch] && rand()%int(min_ticks_per_event/10)==0) {
                triggered_events ++;
                if (TRIGGER_RULE) time_since_recent_L1As.push_back(0);
            }
            nbunch ++;
            if (nbunch == bunches_per_orbit) nbunch = 0;
        }
    }
    for (int ichip=0; ichip<nchips; ichip++) {
        // Condition to read a new word: 1. File not at EOF; 2. matches ticks_per_word to be consistent with e-link speed 3. Does not exceed trigger rate
        bool trigger_condition = false;
        if (RANDOM_L1)  trigger_condition = (triggered_events > nevents[ichip]);
        else trigger_condition = ( (min_ticks_per_event+nticks)/(1+nevents[ichip]) >= min_ticks_per_event );
        // start over if at the end of input file
        if ( input_streams[ichip]->eof() ) {
            input_streams[ichip]->clear();
            input_streams[ichip]->seekg(0);
        }
        if ( (!input_streams[ichip]->eof()) && (nticks%ticks_per_word[ichip]==0) && trigger_condition) {
            value = 0;
            input_streams[ichip]->read( reinterpret_cast<char*>(&value), sizeof(value) ) ;
            if(value & (((uint64_t)1)<<63)) nevents[ichip]++;
            bitset<64> b_value(value);
            //std::cout<<"Event player chip "<<ichip<<" data = "<<b_value<<std::endl;
            //std::cout<<"Event player chip "<<ichip<<" input file get single character = "<<input_streams[ichip]->get()<<std::endl;
            out_read[ichip].set_value(true);
            out_data[ichip].set_value(value);
        }
        else {
            out_read[ichip].set_value(false);
            out_data[ichip].set_value(0);
        }
    }
    nticks ++;
}
