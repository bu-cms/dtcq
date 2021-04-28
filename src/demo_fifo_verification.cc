#include<include/FIFO.h>
#include <DSPatch.h>
#include <ctime>
#include <iostream>
using namespace std;
using namespace DSPatch;

typedef FIFO<uint64_t> FIFO64;
typedef FIFO<uint8_t> FIFO8;

class HalfTimeDataPlayer final : public Component
{
public:
    HalfTimeDataPlayer() : Component()
    {
        SetOutputCount_( 2 );
    }
    virtual void PreProcess_(SignalBus const & inputs, SignalBus& outputs) override {
        outputs.SetValue(0, out_data);
        outputs.SetValue(1, out_read);
    }
    virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override {
		if (rand()%2==0)
		{
        	out_data = rand();
        	out_read = true;
		}
		else out_read = false;
    };

protected:
    uint64_t out_data;
    bool out_read;

};

class HalfTimeTrue final : public Component
{
public:
    HalfTimeTrue() : Component()
    {
        SetOutputCount_( 2 );
    }
    virtual void PreProcess_(SignalBus const & inputs, SignalBus& outputs) override {
		bool rnd = (rand()%2==0);
        outputs.SetValue(0, rnd);
        outputs.SetValue(1, rnd);
    }
    virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override {
		return;
    }
};

// 50% chance read data from FIFO
// always read from source generator if available
// compare the data from FIFO with data from source
class FIFOChecker final : public Component
{
public:
    enum INPUT {
        IN_FIFO_VALID=0, // bool
        IN_FIFO_EMPTY,    // bool
        IN_FIFO_DATA,    // T
        IN_SRC_VALID,   // bool
        IN_SRC_DATA,   // T
        IN_RND_READ,   // bool
        COUNT_IN
    };
    enum OUTPUT {
        OUT_FIFO_READ=0, // bool
        COUNT_OUT
    };
    FIFOChecker() : Component()
    {
        SetInputCount_(INPUT::COUNT_IN);
        SetOutputCount_(OUTPUT::COUNT_OUT);
    }
    virtual void PreProcess_(SignalBus const & inputs, SignalBus& outputs) override {
		read_in_last_tick = read_in_this_tick;
		read_in_this_tick = ( rand()%2==0 );
		if (read_in_this_tick) outputs.SetValue(OUTPUT::OUT_FIFO_READ, 1);
		else outputs.SetValue(OUTPUT::OUT_FIFO_READ, 0);
		//outputs.SetValue(OUTPUT::OUT_FIFO_READ, 1);
        return;
    }
    virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override {
		printf("Tick=%d:\n", tick_counter++);
		bool in_src_valid = *inputs.GetValue<bool>(INPUT::IN_SRC_VALID);
		if ( in_src_valid )
		{
			uint64_t in_src_data = *inputs.GetValue<uint64_t>(INPUT::IN_SRC_DATA);
			printf("\tSource data valid, read %lu, pushing to queue.\n", in_src_data);
			src_data.push(in_src_data);
		}
		else
		{
			printf("\tNo source data produced.\n");
		}
		bool in_fifo_valid = *inputs.GetValue<bool>(INPUT::IN_FIFO_VALID);
		bool in_fifo_empty = *inputs.GetValue<bool>(INPUT::IN_FIFO_EMPTY);
		//read_in_last_tick = read_in_this_tick;
		//read_in_this_tick = *inputs.GetValue<bool>(INPUT::IN_RND_READ);
		if (in_fifo_valid)
		{
			if (src_data.empty()) printf("\tsrc_data empty! something wrong?\n");
			uint64_t in_fifo_data = *inputs.GetValue<uint64_t>(INPUT::IN_FIFO_DATA);
			uint64_t to_compare = src_data.front();
			src_data.pop();
			if (read_in_last_tick) printf("\tFIFO data valid, read %lu, expecting %lu, so %s. \n", in_fifo_data, to_compare, in_fifo_data==to_compare?"good!":"bad!");
			else printf("\tFIFO data valid, read %lu, expecting %lu, but wasn't trying to read! so bad! \n", in_fifo_data, to_compare);
		}
		else if (read_in_last_tick)
		{
			if (in_fifo_empty) printf("\tWas tring to read from FIFO but FIFO empty. good!\n");
			else printf("\tWas tring to read from FIFO but no data comes out. so bad!\n");
		}
		else printf("\tNo data comes out from FIFO and wasn't trying to read. so good!\n");
    };
protected:
	std::queue<uint64_t> src_data;
	bool read_in_this_tick = false;
	bool read_in_last_tick = false;
	int tick_counter = 0;
};

int main(int argc, char* argv[]) {
	cout<<argc<<endl;
    int NCycles = 10;
	if (argc>1) NCycles = atoi(argv[1]);

    auto circuit = std::make_shared<Circuit>();
    auto player  = std::make_shared<HalfTimeDataPlayer>();
    auto fifo    = std::make_shared<FIFO64>();
    auto rnd     = std::make_shared<HalfTimeTrue>();
    auto checker = std::make_shared<FIFOChecker>();

    circuit->AddComponent(player);
    circuit->AddComponent(fifo);
    circuit->AddComponent(checker);
    circuit->AddComponent(rnd);
    circuit->ConnectOutToIn(player, 0, fifo, FIFO64::INPUT::IN_DATA);
    circuit->ConnectOutToIn(player, 1, fifo, FIFO64::INPUT::IN_READ);
    circuit->ConnectOutToIn(player, 0, checker, FIFOChecker::INPUT::IN_SRC_DATA);
    circuit->ConnectOutToIn(player, 1, checker, FIFOChecker::INPUT::IN_SRC_VALID);
    //circuit->ConnectOutToIn(checker, FIFOChecker::OUTPUT::OUT_FIFO_READ, fifo, FIFO64::INPUT::IN_POP);
    circuit->ConnectOutToIn(rnd, 0, fifo, FIFO64::INPUT::IN_POP);
    circuit->ConnectOutToIn(rnd, 1, checker, FIFOChecker::INPUT::IN_RND_READ);
    circuit->ConnectOutToIn(fifo, FIFO64::OUTPUT::OUT_DATA_VALID, checker, FIFOChecker::INPUT::IN_FIFO_VALID);
    circuit->ConnectOutToIn(fifo, FIFO64::OUTPUT::OUT_DATA, checker, FIFOChecker::INPUT::IN_FIFO_DATA);
    circuit->ConnectOutToIn(fifo, FIFO64::OUTPUT::OUT_EMPTY, checker, FIFOChecker::INPUT::IN_FIFO_EMPTY);
    for ( int i = 0; i < NCycles; ++i )
    {
        circuit->Tick();
		printf("FIFO buffer size = %d\n", fifo->d_get_buffer_size());
    }
}
