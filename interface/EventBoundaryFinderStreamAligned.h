#ifndef EVENTBOUNDARYFINDERSTREAMALIGNED_H
#define EVENTBOUNDARYFINDERSTREAMALIGNED_H

#include<interface/EventBoundaryFinder.h>
class EventBoundaryFinderStreamAligned : public EventBoundaryFinder {
    public:
        EventBoundaryFinderStreamAligned(){};
    protected:
        virtual void ProcessInput_(SignalBus const & inputs) override;

};
#endif /* EVENTBOUNDARYFINDERSTREAMALIGNED_H */
