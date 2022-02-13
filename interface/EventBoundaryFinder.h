#ifndef EVENTBOUNDARYFINDER_H
#define EVENTBOUNDARYFINDER_H
#include <include/Component.h>
#include <include/Ports.h>
#include <stdint.h>
#include <queue>
#include <assert.h>
using namespace std;

class EventBoundaryFinder : public Component {
    public:
        bool do_parse;

        // Define input and output ports
		InputPort<uint64_t> in_fifo_i1_data;
		InputPort<bool> in_fifo_i1_data_valid;
		InputPort<bool> in_fifo_i1_data_empty;
		InputPort<bool> in_enable_fifo_i1_data_pop;
        OutputPort<bool> out_fifo_i1_pop ;
        OutputPort<bool> out_fifo_o1_read;
        OutputPort<uint64_t> out_fifo_o1_data;
        OutputPort<bool> out_fifo_o2_read;
        OutputPort<uint16_t> out_fifo_o2_data;

        EventBoundaryFinder(bool _do_parse=false);
		void tick() override;
    private:
        uint32_t halt_time = 0;
        queue<uint64_t> queued_data_words;
        queue<uint16_t> queued_control_words;

};


#endif /* EVENTBOUNDARYFINDER_H */

