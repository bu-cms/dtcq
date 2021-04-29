#include<interface/Circuit.h>

void Circuit::tick(){
    for(auto component : components) {
        component->tick();
    }
    for(auto component : components) {
        component->post_tick();
    }
};