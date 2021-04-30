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

class HalfTimeDataPlayer final : public Component
{
public:
	OutputPort<bool> out_read;
	OutputPort<uint64_t> out_data;
    HalfTimeDataPlayer() : Component() {
		add_output( &out_read );
		add_output( &out_data );
	};
    void tick() override {
		if (rand()%2==0)
		{
			out_read.set_value(true);
			out_data.set_value(rand());
		}
		else {
			out_read.set_value(false);
			out_data.set_value(0);
		}
    }
};

// 50% chance read data from FIFO
// always read from source generator if available
// compare the data from FIFO with data from source
class FIFOChecker final : public Component
{
public:
	InputPort<bool> in_fifo_valid;
	InputPort<bool> in_fifo_empty;
	InputPort<uint64_t> in_fifo_data;
	InputPort<bool> in_src_valid;
	InputPort<uint64_t> in_src_data;
	OutputPort<bool> out_fifo_read;
    FIFOChecker() : Component()
    {
		add_output( &out_fifo_read );
    }
    void tick() override {
		printf("Tick=%d:\n", tick_counter++);
		if ( in_src_valid.get_value() )
		{
			uint64_t IN_SRC_DATA = in_src_data.get_value();
			printf("\tSource data valid, read %lu, pushing to queue.\n", IN_SRC_DATA);
			src_data.push(IN_SRC_DATA);
		}
		else
		{
			printf("\tNo source data produced.\n");
		}
		bool IN_FIFO_VALID = in_fifo_valid.get_value();
		bool FIFO_EMPTY_LAST_TICK = FIFO_EMPTY_THIS_TICK;
		bool FIFO_EMPTY_THIS_TICK = in_fifo_empty.get_value();
		if (IN_FIFO_VALID)
		{
			if (src_data.empty()) printf("\tsrc_data empty! something wrong?\n");
			uint64_t IN_FIFO_DATA = in_fifo_data.get_value();
			uint64_t TO_COMPARE = src_data.front();
			src_data.pop();
			if (READ_IN_LAST_TICK) printf("\tFIFO data valid, read %lu, expecting %lu, so %s. \n", IN_FIFO_DATA, TO_COMPARE, IN_FIFO_DATA==TO_COMPARE?"good!":"bad!");
			else printf("\tFIFO data valid, read %lu, expecting %lu, but wasn't trying to read! so bad! \n", IN_FIFO_DATA, TO_COMPARE);
		}
		else if (READ_IN_LAST_TICK)
		{
			if (FIFO_EMPTY_LAST_TICK) printf("\tWas tring to read from FIFO but FIFO empty. good!\n");
			else printf("\tWas tring to read from FIFO but no data comes out. so bad!\n");
		}
		else printf("\tNo data comes out from FIFO and wasn't trying to read. so good!\n");
		READ_IN_LAST_TICK = READ_IN_THIS_TICK;
		READ_IN_THIS_TICK = ( rand()%2==0 );
		out_fifo_read.set_value(READ_IN_THIS_TICK);
    };
protected:
	std::queue<uint64_t> src_data;
	bool READ_IN_THIS_TICK = false;
	bool READ_IN_LAST_TICK = false;
	bool FIFO_EMPTY_LAST_TICK = true;
	bool FIFO_EMPTY_THIS_TICK = true;
	int tick_counter = 0;
};

int main(int argc, char* argv[]) {
    int NCycles = 10;
	if (argc>1) NCycles = atoi(argv[1]);

    auto circuit = std::make_shared<Circuit>();
    auto player  = std::make_shared<HalfTimeDataPlayer>();
    auto fifo    = std::make_shared<FIFO64>();
    auto checker = std::make_shared<FIFOChecker>();

    circuit->add_component(player);
    circuit->add_component(fifo);
    circuit->add_component(checker);
	player->out_data.connect( &(fifo->in_data) );
	player->out_read.connect( &(fifo->in_push_enable) );
	player->out_data.connect( &(checker->in_src_data) );
	player->out_read.connect( &(checker->in_src_valid) );
	checker->out_fifo_read.connect( &(fifo->in_pop_enable) );
	fifo->out_data.connect( &(checker->in_fifo_data) );
	fifo->out_data_valid.connect( &(checker->in_fifo_valid) );
	fifo->out_empty.connect( &(checker->in_fifo_empty) );
	
    for ( int i = 0; i < NCycles; ++i )
    {
        circuit->tick();
		printf("FIFO buffer size = %d\n", fifo->d_get_buffer_size());
    }
}
