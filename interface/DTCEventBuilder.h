#include <include/Component.h>
#include <include/Ports.h>
#include <algorithm>
#include <iostream>
#include <stdint.h>
#include <memory>
#include <numeric>
#include <assert.h>
#include <stdexcept>

using namespace std;
// only read data from the next event after all data from this event is processed
// processed = nothong done, for now
class DTCEventBuilder final : public Component
{
public:
    std::vector<InputPort<bool>> in_data_valid ;
    std::vector<InputPort<uint64_t>> in_data   ;
    std::vector<InputPort<bool>> in_control_valid ;
    std::vector<InputPort<uint16_t>> in_control    ;
    std::vector<OutputPort<bool>> out_read_data;
    std::vector<OutputPort<bool>> out_read_control ;
    OutputPort<bool> out_event_ready ;
    DTCEventBuilder(int _nchips, int output_links);
    void tick() override ;
    int get_ID();
private:
    int nchips;
    const int OUTPUT_LINKS;
    int ID = 0; // track the number of Event builders created
    std::vector<int> words_to_read;
    std::vector<int> buffer_counter; // instead of an actual buffer
    std::vector<bool> control_full_event;
    std::vector<bool> control_new_event_header;
    std::vector<bool> read_control_last_time;
    std::vector<bool> read_data_last_time;
    int clock_ticks_counter = 0;
    bool processing_new_event = true;
    int WORD_PER_CLOCK_TICK_TO_SEND_EVENT = 0; // equals to number of output links with 25GB/s speed. By design this can be up to 16.
    int remaining_time_to_send_last_event = 0;
};
