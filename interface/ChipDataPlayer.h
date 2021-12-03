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

    ChipDataPlayer(vector<vector<unsigned short>> _vec_event_chip_sizes, int _nchips, vector<float> elink_chip_ratio, bool is_random_l1, bool use_trigger_rule);

    void tick() override;
private:
    const bool RANDOM_L1;
    const bool TRIGGER_RULE;
    unsigned long long nticks = 0;
    int max_event_idx;
    int triggered_events;
    int nchips;
    int nbunch = 0; // cyclic counting bunch from 0-3563;
    std::vector<std::vector<unsigned short>> vec_event_chip_sizes;
    uint64_t value;
    static const int ticks_per_word_per_elink = 20; // assuming all chip has rate of 1.28Gbps, 400M * 64 / 1.28G = 20
    std::vector<int> ticks_per_word; //ticks_per_word_per_elink divided by e-link-to-chip ratio
    static const int trigger_rate =  750; // in kHz
    int ticks_per_event =  400*1000/trigger_rate; // 533, but to be modified in constructor to account for bunch structure
    static const int bunches_per_orbit = 3564;
    bool bunch_not_empty[bunches_per_orbit] = {0}; //modified in initializer
    static const int trigger_rule_max_L1As = 8;
    static const int trigger_rule_bunch_period = 130; // No more than 8 L1As within 130 bunch crossings;
    std::deque<int> time_since_recent_L1As;
    std::vector<std::deque<unsigned short>> remaining_words_for_triggered_events;
    std::vector<bool> new_event_flag;

    int potential_trigger_counts = 0;
    int blocked_trigger_counts = 0;
};
