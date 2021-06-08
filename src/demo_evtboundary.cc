#include <include/FIFO.h>
#include <interface/Circuit.h>
#include <include/Component.h>
#include <include/Ports.h>
#include <ctime>
#include <iostream>
#include <stdint.h>
#include <memory>
#include <random>
#include <limits>
#include <interface/EventBoundaryFinder.h>
using namespace std;

typedef FIFO<uint64_t> FIFO64;
typedef FIFO<uint16_t> FIFO16;

uint64_t rand64() {
	std::random_device rd;
	std::mt19937_64 eng(rd());
	std::uniform_int_distribution<uint64_t> distr;
	return distr(eng);
}

class DataPlayer final : public Component
{
public:
	OutputPort<bool> out_read;
	OutputPort<uint64_t> out_data;
    DataPlayer() : Component() {
		add_output( &out_read );
		add_output( &out_data );
	};

    void tick() override {
		out_read.set_value(true);
		out_data.set_value(rand64());
    }
};


template<typename T>
class DataPrinter final : public Component{
    public:
		InputPort<T> in_data;
        DataPrinter(string input_tag) : Component() {
            tag = input_tag;
        };
        virtual void tick() override {
            cout << tag << " " << uint64_t(in_data.get_value()) << endl;
			in_data.set_value(0); // clears the current value after printing
        };

    protected:
        string tag;
};

int main() {
    auto circuit = std::make_shared<Circuit>();
    auto player  = std::make_shared<DataPlayer>();
    auto fifo_i1 = std::make_shared<FIFO64>();
    auto fifo_o1 = std::make_shared<FIFO64>();
    auto fifo_o2 = std::make_shared<FIFO16>();
    auto ebf     = std::make_shared<EventBoundaryFinder>();

    auto printer_data  = std::make_shared<DataPrinter<uint64_t>>(string("data"));
    auto printer_ctrl  = std::make_shared<DataPrinter<uint16_t>>(string("ctrl"));
    auto printer_debug  = std::make_shared<DataPrinter<uint64_t>>(string("debug"));

    circuit->add_component(player);
    circuit->add_component(fifo_i1);
    circuit->add_component(fifo_o1);
    circuit->add_component(fifo_o2);
    circuit->add_component(ebf);
    circuit->add_component(printer_debug);
    circuit->add_component(printer_data);
    circuit->add_component(printer_ctrl);

    player->out_data.connect( &(fifo_i1->in_data));
    player->out_read.connect( &(fifo_i1->in_push_enable));

    // Input FIFO <-> Boundary finder
    fifo_i1->out_data.connect( &(ebf->in_fifo_i1_data) );
    fifo_i1->out_data_valid.connect( &(ebf->in_fifo_i1_data_valid) );
    ebf->out_fifo_i1_pop.connect( &(fifo_i1->in_pop_enable) );

    // Boundary finder -> output data FIFO
    ebf->out_fifo_o1_data.connect( &(fifo_o1->in_data) );
    ebf->out_fifo_o1_read.connect( &(fifo_o1->in_push_enable) );

    // Boundary finder -> output control FIFO
    ebf->out_fifo_o2_data.connect( &(fifo_o2->in_data) );
    ebf->out_fifo_o2_read.connect( &(fifo_o2->in_push_enable) );


    ebf->out_fifo_o1_data.connect( &(printer_data->in_data) );
    ebf->out_fifo_o2_data.connect( &(printer_ctrl->in_data) );
    
    player->out_data.connect( &(printer_debug->in_data));

    for ( int i = 0; i < 15; ++i )
    {
        cout << "-- tick " << i <<endl;
        circuit->tick();
    }

}
