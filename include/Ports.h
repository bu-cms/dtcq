#ifndef PORTS_H
#define PORTS_H

#include<vector>
using namespace std;

class Propagatable {
	public:
        virtual void propagate() = 0;
};

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
class OutputPort: public Port<T>, public Propagatable {
    public:
        OutputPort(){};

        void connect(InputPort<T> * other) {
            connected_ports.push_back(other);
        }

        virtual void propagate() override{
            for(auto port: connected_ports) {
                port->set_value(value);
            }
        }
    protected:
        T value;
        vector<InputPort<T>*> connected_ports;
};



#endif /* PORTS_H */
