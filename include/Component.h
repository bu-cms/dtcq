#ifndef COMPONENT_H
#define COMPONENT_H

#include <Ports.h>
class Component {
    public:
        Component(){};
		void add_output(Propagatable* port){
			output_ports.push_back( port );
		}
        virtual void tick() = 0;
        virtual void post_tick() {
			for (auto port:output_ports) port->propagate();
		}
	protected:
		vector<Propagatable*> output_ports;
};
#endif /* COMPONENT_H */
