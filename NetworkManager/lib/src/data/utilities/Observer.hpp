#pragma once
#ifndef OBSERVER_HPP
#define OBSERVER_HPP
#include <set>

namespace ObserverEvent {
enum Event {
    configLoaded = 1,
    deviceConfigUpdated = 2,
    mdnsConfigUpdated = 3,
    networksConfigUpdated = 4,
    apConfigUpdated = 5,
    wifiTxPowerUpdated = 6,
    deviceDataJsonUpdated = 7,
};
}

class IObserver {
   public:
    virtual void update(ObserverEvent::Event event) = 0;
};

class ISubject {
   private:
    std::set<IObserver *> observers;

   public:
    void attach(IObserver *observer) { this->observers.insert(observer); }

    void detach(IObserver *observer) { this->observers.erase(observer); }

    void notify(ObserverEvent::Event event) {
        for (auto observer : this->observers) {
            observer->update(event);
        }
    }
};

#endif  // OBSERVER_HPP