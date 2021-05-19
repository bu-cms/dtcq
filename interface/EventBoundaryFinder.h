#ifndef EVENTBOUNDARYFINDER_H
#define EVENTBOUNDARYFINDER_H
#include <include/Component.h>
#include <include/Ports.h>
#include <stdint.h>
#include <queue>
using namespace std;

class EventBoundaryFinder : public Component {
    public:
		InputPort<uint64_t> in_fifo_i1_data;
		InputPort<bool> in_fifo_i1_data_valid;
		InputPort<bool> in_fifo_i1_data_empty;
		InputPort<bool> in_enable_fifo_i1_data_pop;
        OutputPort<bool> out_fifo_i1_pop ;
        OutputPort<bool> out_fifo_o1_read;
        OutputPort<uint64_t> out_fifo_o1_data;
        OutputPort<bool> out_fifo_o2_read;
        OutputPort<uint8_t> out_fifo_o2_data;

        EventBoundaryFinder();
		void tick() override;

};


#endif /* EVENTBOUNDARYFINDER_H */

