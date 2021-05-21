#include <include/FIFO.h>
#include <interface/Circuit.h>
#include <include/Component.h>
#include <include/Ports.h>
#include <ctime>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <memory>
#include <random>
#include <limits>
#include <interface/EventBoundaryFinder.h>
using namespace std;

typedef FIFO<uint64_t> FIFO64;
typedef FIFO<uint8_t> FIFO8;

// read data from binary file then split streams into different chip managers
class DataPlayerFromFile final : public Component
{
public:
	std::vector<OutputPort<bool>> out_read;
	std::vector<OutputPort<uint64_t>> out_data;
    DataPlayerFromFile(const char* _file_name, int _nchips) : Component(), out_read(_nchips), out_data(_nchips), stored_data(_nchips) {
		nchips = _nchips;
		for (int ichip=0; ichip<nchips; ichip++) {
			add_output( &(out_read[ichip]) );
			add_output( &(out_data[ichip]) );
		}
		ifstream input(_file_name, std::ios::binary);
		int chip_locator = nchips-1;
		while (!input.eof()) {
			input.read( reinterpret_cast<char*>(&value), sizeof(value) ) ;
			if(value & (((uint64_t)1)<<63)) chip_locator++;
			if(chip_locator==nchips) chip_locator=0;
			stored_data[chip_locator].push(value);
		}
		input.close();
	};

    void tick() override {
		for (int ichip=0; ichip<nchips; ichip++) {
			if (stored_data[ichip].size()==0) {
				out_read[ichip].set_value(false);
				out_data[ichip].set_value(0);
			}
			else if (odd_tick) {
				out_read[ichip].set_value(true);
				out_data[ichip].set_value(stored_data[ichip].front());
				stored_data[ichip].pop();
				odd_tick = !odd_tick;
			}
			else {
				out_read[ichip].set_value(false);
				out_data[ichip].set_value(0);
				odd_tick = !odd_tick;
			}
		}
    }
private:
	int odd_tick=true;
	int nchips;
	std::vector<std::queue<uint64_t>> stored_data;
	uint64_t value;
};

// only read data from the next event after all data from this event is processed
// processed = nothong done, for now
class DummyEventBuilder final : public Component
{
public:
	std::vector<InputPort<bool>> in_data_valid ;
	std::vector<InputPort<uint64_t>> in_data   ;
	std::vector<InputPort<bool>> in_isNE_valid ;
	std::vector<InputPort<uint8_t>> in_isNE    ;
	std::vector<OutputPort<bool>> out_read     ;
    DummyEventBuilder(int _nchips) : Component(), in_data_valid(_nchips), in_data(_nchips), in_isNE_valid(_nchips), in_isNE(_nchips), out_read(_nchips), is_full_event(_nchips) {
		nchips = _nchips;
		for (int ichip=0; ichip<nchips; ichip++) {
			add_output( &(out_read[ichip]) );
		}
	};

    void tick() override {
		clock_ticks_counter ++;
		for (int ichip=0; ichip<nchips; ichip++) {
			if (is_full_event[ichip]) {
				out_read[ichip].set_value(false); // already have all data for this chip, will wait for others to finish
				//if (in_isNE_valid[ichip].get_value() == true) std::cout<<"Error: not expecting valid 'is New Event' when already having full event!"<<std::endl;
				// it takes 2 clock cycles to see the effect of setting out_read=false
			}
			else {
				if (in_isNE_valid[ichip].get_value() == true) {
					if (in_isNE[ichip].get_value()==192) {
						is_full_event[ichip]=true;
						out_read[ichip].set_value(false);
					}
				}
				else out_read[ichip].set_value(true);
			}
		}
		if (std::all_of(is_full_event.begin(), is_full_event.end(), [](bool v){return v;} ) ) { // if all chips have full event ready
			std::cout<<"New event processed after "<<clock_ticks_counter<<" clock ticks!"<<std::endl;
			clock_ticks_counter = 0;
			for (int ichip=0; ichip<nchips; ichip++) {
				is_full_event[ichip] = false;
				out_read[ichip].set_value(true);
			}
		}
    }
private:
	int nchips;
	std::vector<bool> is_full_event;
	int clock_ticks_counter = 0;
};


int main() {
	int nchips = 5;
    auto circuit = std::make_shared<Circuit>();
    auto player  = std::make_shared<DataPlayerFromFile>("../input/dtc11_elink0.bin", nchips); // there are 5 modules in dtc11 elink0
    auto evt_builder  = std::make_shared<DummyEventBuilder>(nchips); // there are 5 modules in dtc11 elink0
    circuit->add_component(player);
    circuit->add_component(evt_builder);

	std::vector<std::shared_ptr<FIFO64>>              fifos;
	std::vector<std::shared_ptr<EventBoundaryFinder>>  ebfs;
	for (int ichip=0; ichip<nchips; ichip++){
		fifos.push_back(std::make_shared<FIFO64>());
		ebfs.push_back(std::make_shared<EventBoundaryFinder>());
		circuit->add_component(fifos[ichip]);
		circuit->add_component(ebfs[ichip] );
		player->out_data[ichip].connect( &(fifos[ichip]->in_data) );
		player->out_read[ichip].connect( &(fifos[ichip]->in_push_enable) );
    	//Input FIFO <-> Boundary finder
    	fifos[ichip]->out_data.connect( &(ebfs[ichip]->in_fifo_i1_data) );
    	fifos[ichip]->out_data_valid.connect( &(ebfs[ichip]->in_fifo_i1_data_valid) );
    	ebfs[ichip]->out_fifo_i1_pop.connect( &(fifos[ichip]->in_pop_enable) );
    	//Boundary finder <-> event builder
    	ebfs[ichip]->out_fifo_o1_data.connect( &(evt_builder->in_data[ichip]) );
    	ebfs[ichip]->out_fifo_o1_read.connect( &(evt_builder->in_data_valid[ichip]) );
    	ebfs[ichip]->out_fifo_o2_data.connect( &(evt_builder->in_isNE[ichip]) );
    	ebfs[ichip]->out_fifo_o2_read.connect( &(evt_builder->in_isNE_valid[ichip]) );
		evt_builder->out_read[ichip].connect( &(ebfs[ichip]->in_enable_fifo_i1_data_pop) );
	}
	int inactive_time = 0;
	int i_tick = 0;
	ofstream output("output/fifo_sizes.txt");
    while (inactive_time<2000 )
    {
		i_tick++;
        //cout << "-- tick " << i <<endl;
        circuit->tick();
		bool activity=false;
		for (auto port:evt_builder->in_data_valid) activity = activity | port.get_value();
		if(!activity) inactive_time++;
		else inactive_time=0;
		//cout<<"player->out_data[0]="<<player->out_data[0].get_value()<<" evt_builder->in_data[0]="<<evt_builder->in_data[0].get_value()<<" evt_builder->in_isNE[0]="<<evt_builder->in_isNE[0].get_value()<<endl;
		for (int ichip=0; ichip<nchips; ichip++) {
			//(*outputs[ichip])<<to_string(fifos[ichip]->d_get_buffer_size()).c_str()<<",";
			if (ichip>0) output<<",";
			output<<to_string(fifos[ichip]->d_get_buffer_size());
		};
		output<<endl;
    }
	cout<<"total ticks="<<i_tick<<endl;
}
