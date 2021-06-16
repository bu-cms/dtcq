#include <include/Component.h>
#include <include/Ports.h>
#include <deque>
#include <algorithm>
#include <iostream>
#include <boost/filesystem.hpp>
#include <fstream>
#include <stdint.h>
#include <memory>
#include <random>
#include <bitset>
#include <limits>
#include <assert.h>
#include <stdexcept>

class ChipDataPlayer final : public Component
{
public:
    std::vector<OutputPort<bool>> out_read;
    std::vector<OutputPort<uint64_t>> out_data;

    ChipDataPlayer(vector<string> file_name_list, int _nchips, vector<float> elink_chip_ratio, bool is_random_l1, bool use_trigger_rule);

    void tick() override;
private:
    const bool RANDOM_L1;
    const bool TRIGGER_RULE;
    unsigned long long nticks = 0;
    int triggered_events = 0;
    int nchips;
    int nbunch = 0; // cyclic counting bunch from 0-3563;
    std::vector<std::ifstream*> input_streams;
    std::vector<int> nevents;
    uint64_t value;
    static const int ticks_per_word_per_elink = 20; // assuming all chip has rate of 1.28Gbps, 400M * 64 / 1.28G = 20
    std::vector<int> ticks_per_word; //ticks_per_word_per_elink divided by e-link-to-chip ratio
    static const int min_ticks_per_event =  533; // 400M / 750k = 533.3
    static const int bunches_per_orbit = 3564;
    bool bunch_not_empty[bunches_per_orbit] = {0}; //modified in initializer
    static const int trigger_rule_max_L1As = 8;
    static const int trigger_rule_bunch_period = 130; // No more than 8 L1As within 130 bunch crossings;
    std::deque<int> time_since_recent_L1As;
};
