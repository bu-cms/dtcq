#include <include/FIFO.h>
#include <interface/Circuit.h>
#include <include/Component.h>
#include <include/Ports.h>
#include <ctime>
#include <iostream>
#include <stdint.h>
#include <memory>

using namespace std;

typedef FIFO<uint64_t> FIFO64;
typedef FIFO<uint8_t> FIFO8;

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
		out_data.set_value(rand());
    }
};

// A register that is constant
class RegisterConst final : public Component
{
public:
	OutputPort<bool> out_value;
    RegisterConst(bool value) : Component() {
		add_output( &out_value );
		out_value.set_value(value);
	}
    void tick() {
    }
protected:
};

int main(int argc, char* argv[]) {
    int NFifo = 100;
	if (argc>1) NFifo = atoi(argv[1]);
    int NCycles = 100000;
	if (argc>2) NCycles = atoi(argv[2]);

    auto circuit = std::make_shared<Circuit>();
    auto player  = std::make_shared<DataPlayer>();
    auto regconst = std::make_shared<RegisterConst>(true);

    circuit->add_component(player);
    circuit->add_component(regconst);

    std::shared_ptr<FIFO64> fifo_array[NFifo]; 
    for( int i = 0; i < NFifo; i++)
    {
        fifo_array[i]  = std::make_shared<FIFO64>();
        circuit->add_component(fifo_array[i]);
        regconst->out_value.connect( &(fifo_array[i]->in_pop_enable) );
        if (i==0)
        {
			player->out_data.connect( &(fifo_array[i]->in_data) );
			player->out_read.connect( &(fifo_array[i]->in_push_enable) );
        }
        else
        {
			fifo_array[i]->out_data.connect( &(fifo_array[i]->in_data) );
			fifo_array[i]->out_data_valid.connect( &(fifo_array[i]->in_push_enable) );
        }
    }



    time_t t_start;
    time_t t_end;
    time(&t_start);
    for ( int i = 0; i < NCycles; ++i )
    {
        circuit->tick();
    }
    time(&t_end);
    float seconds = difftime(t_end, t_start);
    printf("%d-FIFO Circuit run %d cycles in %.f seconds\n", NFifo, NCycles, seconds);

}
