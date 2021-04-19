#ifndef EVENTBOUNDARYFINDER_H
#define EVENTBOUNDARYFINDER_H
#include<DSPatch.h>
#include<queue>
#include<interface/FComponent.h>
using namespace DSPatch;
using namespace std;

class EventBoundaryFinder : public FComponent {
    public:
        EventBoundaryFinder();

        // The Boundary finder is interfaced to three FIFOs:
        // 64-bit input words (AURORA blocks) are read from FIFO_I1
        // The same words are then passed through and written to FIFO_O1,
        // and control words containing event boundary info are written 
        // to FIFO_O2
        enum INPUT {
            // Input FIFO
            IN_FIFO_I1_DATA=0,     // uint64_t
            IN_FIFO_I1_DATA_VALID, // bool
            IN_FIFO_I1_DATA_EMPTY, // bool
            COUNT_IN

        };
        enum OUTPUT {
            // Input FIFO
            OUT_FIFO_I1_POP=1,  // bool
            
            // First output FIFO (data FIFO)
            OUT_FIFO_O1_READ, // bool
            OUT_FIFO_O1_DATA, // uint64_t

            // Second output FIFO (control FIFO)
            OUT_FIFO_O2_READ, // bool
            OUT_FIFO_O2_DATA, // uint64_t
            COUNT_OUT
        };
    protected:
        virtual void ProcessOutput_(SignalBus & outputs) override;
        bool out_fifo_i1_pop;
        bool out_fifo_o1_read;
        uint64_t out_fifo_o1_data;

        bool out_fifo_o2_read;
        uint64_t out_fifo_o2_data;
};


#endif /* EVENTBOUNDARYFINDER_H */

