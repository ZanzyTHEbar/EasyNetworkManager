#pragma once
#ifndef OBSERVER_HPP
#    define OBSERVER_HPP
#    include <set>

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

template <typename EnumT>
class IObserver {
   public:
    virtual void update(EnumT event) = 0;
};

template <typename EnumT>
class ISubject {
   private:
    std::set<IObserver<EnumT>*> observers;

   public:
    void attach(IObserver<EnumT>* observer) {
        this->observers.insert(observer);
    }

    void detach(IObserver<EnumT>* observer) {
        this->observers.erase(observer);
    }

    void notify(EnumT event) {
        for (auto observer : this->observers) {
            observer->update(event);
        }
    }
};

#endif  // OBSERVER_HPP