#include<interface/FComponent.h>

FComponent::FComponent() : Component::Component() {
    state = false;
}


void FComponent::Process_(SignalBus const & inputs, SignalBus& outputs){
    ProcessOutput_(outputs);
    if(state){
        ProcessInput_(inputs);
    }
    state = not state;
}
