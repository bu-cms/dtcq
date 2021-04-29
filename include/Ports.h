#ifndef PORTS_H
#define PORTS_H

#include<vector>
using namespace std;
template<typename T>
class Port {
    public:
        Port(){};
        T get_value(){
            return value;
        };
        void set_value(T new_value) {
            value = new_value;
        }
    protected:
        T value;
};
template<typename T>
class InputPort: public Port<T> {
};


template<typename T>
class OutputPort: public Port<T> {
    public:
        OutputPort(){};

        void connect(InputPort<T> * other) {
            connected_ports->push_back(other);
        }

        void propagate() {
            for(auto port: connected_ports) {
                port->set_value(value);
            }
        }
    protected:
        T value;
        vector<InputPort<T>*> connected_ports;
};



#endif /* PORTS_H */
