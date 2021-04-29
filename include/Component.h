#ifndef COMPONENT_H
#define COMPONENT_H
class Component {
    public:
        Component(){};
        virtual void tick() = 0;
        virtual void post_tick() = 0;
};
#endif /* COMPONENT_H */
