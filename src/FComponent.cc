#include<interface/FComponent.h>

FComponent::FComponent() : Component::Component() {
    state = false;
}


void FComponent::Process_(SignalBus const & inputs, SignalBus& outputs){
    if(state){
        ProcessInput_(inputs);
    }else{
        ProcessOutput_(outputs);
    }
    state = not state;
}