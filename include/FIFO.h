#include<DSPatch.h>
#include<queue>
using namespace DSPatch;
using namespace std;

template<typename T>
class FIFO : public Component {
    public:
        FIFO();

        enum INPUT {
            IN_DATA=0, // T
            IN_POP,    // bool
            IN_READ,   // bool
            COUNT_IN
        };
        enum OUTPUT {
            OUT_DATA=0,     // T
            OUT_DATA_VALID, // bool
            OUT_EMPTY,      //bool
            COUNT_OUT
        };
    
    protected:
        virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override;
        queue<T> _buffer;
};

template<typename T>
FIFO<T>::FIFO() {
    SetInputCount_(FIFO::INPUT::COUNT_IN);
    SetOutputCount_(FIFO::OUTPUT::COUNT_OUT);
}

template<typename T>
void FIFO<T>::Process_(SignalBus const & inputs, SignalBus& outputs){
    auto in_data = inputs.GetValue<T>(FIFO::INPUT::IN_DATA);
    auto in_pop  = inputs.GetValue<bool>(FIFO::INPUT::IN_POP);
    auto in_read = inputs.GetValue<bool>(FIFO::INPUT::IN_READ);

    if(in_read){
        _buffer.push(*in_data);
    }
    if(in_pop) {
        outputs.SetValue(FIFO::OUTPUT::OUT_DATA, _buffer.front());
        _buffer.pop();
        outputs.SetValue(FIFO::OUTPUT::OUT_DATA_VALID, 1);
    } else {
        outputs.SetValue(FIFO::OUTPUT::OUT_DATA_VALID, 0);
    }

    outputs.SetValue(FIFO::OUTPUT::OUT_EMPTY, _buffer.size()==0);
}
