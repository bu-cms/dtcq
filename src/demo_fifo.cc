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

// Data fall through immediately but get printed out
class DataCounterPrinter final : public Component
{
public:
    DataCounterPrinter(const char* PrinterName) : Component()
    {
        SetInputCount_(1);
        SetOutputCount_(1);
        PrinterName_ = PrinterName;
    }
    virtual void PreProcess_(SignalBus const & inputs, SignalBus& outputs) override {
        return;
    }
    virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override {
        auto tmp_data = inputs.GetValue<uint64_t>( 0 );
        printf("%s@Count%04i : %lu\n", PrinterName_, Count, *tmp_data);
        outputs.SetValue(0, *tmp_data);
        Count += 1;
    };
protected:
    const char* PrinterName_;
    int Count = 0;
};

// A register that is constant
class RegisterConst final : public Component
{
public:
    RegisterConst(bool value) : Component()
    {
        value_ = value;
        SetOutputCount_(1);
    }
    virtual void PreProcess_(SignalBus const & inputs, SignalBus& outputs) override {
        return;
    }
    virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override {
        outputs.SetValue(0, value_);
    };
protected:
    bool value_;
};

// A register that is True in 90% of the time
class RegisterTrueInNineTenth final : public Component
{
public:
    RegisterTrueInNineTenth() : Component()
    {
        SetOutputCount_(1);
    }
    virtual void PreProcess_(SignalBus const & inputs, SignalBus& outputs) override {
        return;
    }
    virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override {
        int random = rand();
        if (random % 10 > 0) outputs.SetValue(1, value_);
        else               outputs.SetValue(0, value_);
    };
protected:
    bool value_;
};

int main(int argc, char* argv[]) {
	cout<<argc<<endl;
    int NFifo = 100;
	if (argc>1) NFifo = atoi(argv[1]);
    int NCycles = 1000;
	if (argc>2) NCycles = atoi(argv[2]);

    auto circuit = std::make_shared<Circuit>();
    auto player  = std::make_shared<DataPlayer>();
    auto regconst = std::make_shared<RegisterConst>(true);
    auto printer1 = std::make_shared<DataCounterPrinter>("Head");
    auto printer2 = std::make_shared<DataCounterPrinter>("Tail");


    circuit->AddComponent(player);
    circuit->AddComponent(regconst);
    circuit->AddComponent(printer1);
    circuit->AddComponent(printer2);
    circuit->ConnectOutToIn(player, 0, printer1, 0);
    std::shared_ptr<FIFO64> fifo_array[NFifo]; 
    for( int i = 0; i < NFifo; i++)
    {
        fifo_array[i]  = std::make_shared<FIFO64>();
        circuit->AddComponent(fifo_array[i]);
        circuit->ConnectOutToIn(regconst,   0, fifo_array[i], FIFO64::INPUT::IN_POP);
        if (i==0)
        {
            circuit->ConnectOutToIn(printer1, 0, fifo_array[i], FIFO64::INPUT::IN_DATA);
            circuit->ConnectOutToIn(player,   1, fifo_array[i], FIFO64::INPUT::IN_READ);
        }
        else
        {
            circuit->ConnectOutToIn(fifo_array[i-1], FIFO64::OUTPUT::OUT_DATA, fifo_array[i], FIFO64::INPUT::IN_DATA);
            circuit->ConnectOutToIn(fifo_array[i-1], FIFO64::OUTPUT::OUT_DATA_VALID, fifo_array[i], FIFO64::INPUT::IN_READ);
        }
    }

    circuit->ConnectOutToIn(fifo_array[NFifo-1], FIFO64::OUTPUT::OUT_DATA, printer2, 0);


    time_t t_start;
    time_t t_end;
    time(&t_start);
    for ( int i = 0; i < NCycles; ++i )
    {
        circuit->Tick();
    }
    time(&t_end);
    float seconds = difftime(t_end, t_start);
    printf("%d-FIFO Circuit run %d cycles in %.f seconds\n", NFifo, NCycles, seconds);

}
