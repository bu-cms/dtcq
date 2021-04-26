#include<include/FIFO.h>
#include <DSPatch.h>
#include <ctime>
#include <iostream>
using namespace std;
using namespace DSPatch;

typedef FIFO<uint64_t> FIFO64;
typedef FIFO<uint8_t> FIFO8;

class DataPlayer final : public Component
{
public:
    DataPlayer() : Component()
    {
        SetOutputCount_( 2 );
    }
    virtual void PreProcess_(SignalBus const & inputs, SignalBus& outputs) override {
        outputs.SetValue(0, out_data);
        outputs.SetValue(1, out_read);
    }
    virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override {
        out_data = rand();
        out_read = true;
    };

protected:
    uint64_t out_data;
    bool out_read;

};


int main() {
    auto circuit = std::make_shared<Circuit>();
    auto player  = std::make_shared<DataPlayer>();
    auto fifo  = std::make_shared<FIFO64>();


    circuit->AddComponent(fifo);
    circuit->AddComponent(player);

    circuit->ConnectOutToIn(player, 0, fifo, FIFO64::INPUT::IN_DATA);
    circuit->ConnectOutToIn(player, 1, fifo, FIFO64::INPUT::IN_READ);


    // Circuit tick method 1: Manual
    for ( int i = 0; i < 10; ++i )
    {
        circuit->Tick();
        cout << i 
             <<  " " << fifo->d_get_buffer_size()
             << endl;
    }

}
