#include<DSPatch.h>
#include<queue>
#include<iostream>
using namespace DSPatch;
using namespace std;

class FIFO_READ_FAILURE_EXCEPTION: public exception
{
  virtual const char* what() const throw()
  {
    return "FIFO failed to read input data";
  }
};

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

        int d_get_buffer_size(){
            return _buffer.size();
        }
        virtual void PreProcess_(SignalBus const & inputs, SignalBus& outputs) override;
        virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override;
    protected:
        queue<T> _buffer;

        T out_data;
        bool out_data_valid;
        bool out_empty;
};

template<typename T>
FIFO<T>::FIFO() {
    SetInputCount_(FIFO::INPUT::COUNT_IN);
    SetOutputCount_(FIFO::OUTPUT::COUNT_OUT);
}

template<typename T>
void FIFO<T>::Process_(SignalBus const & inputs, SignalBus& outputs){
    // Defaults
    out_data = 0;
    out_data_valid = 0;
    out_empty = 0;

    auto in_data = inputs.GetValue<T>(FIFO::INPUT::IN_DATA);
    auto in_pop  = inputs.GetValue<bool>(FIFO::INPUT::IN_POP);
    auto in_read = inputs.GetValue<bool>(FIFO::INPUT::IN_READ);
    if(in_read and *in_read){
        if(not in_data) {
            throw new FIFO_READ_FAILURE_EXCEPTION();
        }
        _buffer.push(*in_data);
    }

    if(in_pop and *in_pop) {
        if(_buffer.size()>0) {
            out_data = _buffer.front();
            _buffer.pop();
            out_data_valid = 1;
        }
    }
    out_empty = _buffer.size() == 0;


}
template<typename T>
void FIFO<T>::PreProcess_(SignalBus const & inputs, SignalBus& outputs){
    outputs.SetValue(FIFO::OUTPUT::OUT_DATA, out_data);
    outputs.SetValue(FIFO::OUTPUT::OUT_DATA_VALID, out_data_valid);
    outputs.SetValue(FIFO::OUTPUT::OUT_EMPTY, out_empty);
}
