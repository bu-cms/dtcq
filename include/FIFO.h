#ifndef FIFO_H
#define FIFO_H
#include<include/Component.h>
#include<include/Ports.h>
#include<queue>
using namespace std;
template<typename T>
class FIFO : public Component {
    public:
        FIFO(){};

        InputPort<T> in_data;
        InputPort<bool> in_push_enable;
        InputPort<bool> in_pop_enable;

        OutputPort<T> out_data;
        OutputPort<bool> out_data_valid;
        OutputPort<bool> out_empty;

        queue<T> buffer;
        void tick() {
            out_data_valid.set_value(false);

            if(in_pop_enable.get_value() and buffer.size()>0){
                out_data.set_value(buffer.front());
                buffer.pop();
                out_data_valid.set_value(true);
            }
            if(in_push_enable.get_value()) {
                buffer.push(in_data.get_value());
            }

            out_empty.set_value(buffer.size()==0);
        }
        void post_tick(){
            out_data.propagate();
            out_data_valid.propagate();
            out_empty.propagate();
        }
};


#endif /* FIFO_H */
