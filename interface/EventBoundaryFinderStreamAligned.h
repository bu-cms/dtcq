#ifndef EVENTBOUNDARYFINDERSTREAMALIGNED_H
#define EVENTBOUNDARYFINDERSTREAMALIGNED_H

#include<interface/EventBoundaryFinder.h>
class EventBoundaryFinderStreamAligned : public EventBoundaryFinder {
    public:
        EventBoundaryFinderStreamAligned(){};
        virtual void Process_(SignalBus const & inputs, SignalBus& outputs) override;
};
#endif /* EVENTBOUNDARYFINDERSTREAMALIGNED_H */
