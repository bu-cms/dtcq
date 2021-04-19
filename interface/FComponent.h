#ifndef FCOMPONENT_H
#define FCOMPONENT_H
#include<DSPatch.h>

using namespace DSPatch;
class FComponent : public Component {
    public:
        FComponent();
        virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override;
    protected:
        virtual void ProcessInput_(SignalBus const & inputs) = 0;
        virtual void ProcessOutput_(SignalBus & outputs) = 0;
        bool state;
};

#endif /* FCOMPONENT_H */
