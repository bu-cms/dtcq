#include<include/FIFO.h>
#include <DSPatch.h>
#include <ctime>
#include <iostream>
#include <interface/EventBoundaryFinderStreamAligned.h>
using namespace std;
using namespace DSPatch;

typedef FIFO<uint64_t> FIFO64;
typedef FIFO<uint8_t> FIFO8;

class DataPlayer final : public FComponent
{
public:
    DataPlayer() : FComponent()
    {
        SetOutputCount_( 2 );
    }

protected:
    uint64_t out_data;
    bool out_read;
    virtual void ProcessOutput_(SignalBus & outputs) override {
        outputs.SetValue(0, out_data);
        // cout << "DATA " << *outputs.GetValue<uint64_t>(0) << endl;
        outputs.SetValue(1, out_read);
    }
    virtual void ProcessInput_(SignalBus const & inputs) override {
        out_data = uint64_t(rand());
        out_read = true;
    };

};

template<typename T>
class DataPrinter final : public FComponent{
    public:
        DataPrinter(string input_tag) : FComponent() {
            tag = input_tag;
            SetInputCount_(1);
            SetOutputCount_(0);
        };

    protected:
        string tag;
        T data;
        virtual void ProcessOutput_(SignalBus & outputs) override {
            cout << tag << " " << int(data) << endl;
        };
        virtual void ProcessInput_(SignalBus const & inputs) override {
            auto ptr = inputs.GetValue<T>(0);
            if(ptr){
                data = *ptr;
            }
        };
};

int main() {
    auto circuit = std::make_shared<Circuit>();

    auto player  = std::make_shared<DataPlayer>();
    auto fifo_i1 = std::make_shared<FIFO64>();
    auto fifo_o1 = std::make_shared<FIFO64>();
    auto fifo_o2 = std::make_shared<FIFO8>();
    auto ebf     = std::make_shared<EventBoundaryFinderStreamAligned>();

    // auto printer_data  = std::make_shared<DataPrinter<uint64_t>>(string("data"));
    // auto printer_ctrl  = std::make_shared<DataPrinter<uint8_t>>(string("ctrl"));
    auto printer_debug  = std::make_shared<DataPrinter<uint64_t>>(string("debug"));

    circuit->AddComponent(player);
    circuit->AddComponent(fifo_i1);
    // circuit->AddComponent(fifo_o1);
    // circuit->AddComponent(fifo_o2);
    // circuit->AddComponent(ebf);
    // circuit->AddComponent(printer_data);
    // circuit->AddComponent(printer_ctrl);
    circuit->AddComponent(printer_debug);

    // circuit->ConnectOutToIn(player, 0,       fifo_i1, FIFO64::INPUT::IN_DATA );
    // circuit->ConnectOutToIn(player, 1,       fifo_i1, FIFO64::INPUT::IN_READ);

    // // Input FIFO <-> Boundary finder
    // circuit->ConnectOutToIn(fifo_i1, FIFO64::OUTPUT::OUT_DATA,       ebf, EventBoundaryFinder::INPUT::IN_FIFO_I1_DATA );
    // circuit->ConnectOutToIn(fifo_i1, FIFO64::OUTPUT::OUT_DATA_VALID, ebf, EventBoundaryFinder::INPUT::IN_FIFO_I1_DATA_VALID);
    // circuit->ConnectOutToIn(ebf,     EventBoundaryFinder::OUTPUT::OUT_FIFO_I1_POP, fifo_i1, FIFO64::INPUT::IN_POP);

    // // Boundary finder -> output data FIFO
    // circuit->ConnectOutToIn(ebf, EventBoundaryFinder::OUTPUT::OUT_FIFO_O1_DATA, fifo_o1, FIFO64::INPUT::IN_DATA);
    // circuit->ConnectOutToIn(ebf, EventBoundaryFinder::OUTPUT::OUT_FIFO_O1_READ, fifo_o1, FIFO64::INPUT::IN_READ);

    // // Boundary finder -> output control FIFO
    // circuit->ConnectOutToIn(ebf, EventBoundaryFinder::OUTPUT::OUT_FIFO_O2_DATA, fifo_o2, FIFO8::INPUT::IN_DATA);
    // circuit->ConnectOutToIn(ebf, EventBoundaryFinder::OUTPUT::OUT_FIFO_O2_READ, fifo_o2, FIFO8::INPUT::IN_READ);


    // circuit->ConnectOutToIn(ebf, EventBoundaryFinder::OUTPUT::OUT_FIFO_O1_DATA, printer_data, 0);
    // circuit->ConnectOutToIn(ebf, EventBoundaryFinder::OUTPUT::OUT_FIFO_O2_DATA, printer_ctrl, 0);
    
    circuit->ConnectOutToIn(player, 0, printer_debug, 0);

    // Circuit tick method 1: Manual
    bool state = 0;
    for ( int i = 0; i < 10; ++i )
    {
        cout << "-- tick " << i <<endl;
        circuit->Tick();
        state = not state;
    }

}