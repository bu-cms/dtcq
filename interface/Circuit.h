#ifndef CIRCUIT_H
#define CIRCUIT_H
#include<include/Component.h>
#include<vector>

using namespace std;
class Circuit {
    public:
        Circuit(){};
        void tick();

        void add_component(Component * component) {
            components.push_back(component);
        }
    protected:
        vector<Component*> components;
};
#endif /* CIRCUIT_H */
