#ifndef CIRCUIT_H
#define CIRCUIT_H
#include<include/Component.h>
#include<vector>
#include<memory>

using namespace std;
class Circuit {
    public:
        Circuit(){};
        void tick();

        void add_component(std::shared_ptr<Component> component) {
            components.push_back(component);
        }
    protected:
        vector<std::shared_ptr<Component>> components;
};
#endif /* CIRCUIT_H */
