#pragma once

#if defined(ESP8266)

#include <algorithm>
#include <memory>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <data/config/platform_mutex.hpp>
#include <helpers/id_interface.hpp>

// EasyHelpers currently includes FreeRTOS semaphore headers unconditionally.
// Keep its observer API while using the real ESP8266 atomic lock instead.
namespace Helpers {

template <typename EnumT, typename PayloadT = void>
class IObserver : public IId {
   public:
    virtual void update(const EnumT& event, const PayloadT& payload) = 0;
};

template <typename EnumT>
class IObserver<EnumT, void> : public IId {
   public:
    virtual void update(const EnumT& event) = 0;
};

template <typename EnumT, typename PayloadT = void>
class ISubject {
   private:
    EasyNetworkManagerMutex mutex;
    using ObserverPtr_t = std::weak_ptr<IObserver<EnumT, PayloadT>>;
    using ObserversByNameMap_t = std::unordered_map<uint64_t, ObserverPtr_t>;

    ObserversByNameMap_t observers;

    std::vector<std::shared_ptr<IObserver<EnumT, PayloadT>>> snapshot(
        uint64_t key, bool all) {
        std::lock_guard<EasyNetworkManagerMutex> lock(mutex);
        std::vector<std::shared_ptr<IObserver<EnumT, PayloadT>>> result;
        result.reserve(observers.size());
        for (const auto& [observerKey, observerWeak] : observers) {
            if (!all && observerKey != key) {
                continue;
            }
            if (auto observer = observerWeak.lock()) {
                result.push_back(std::move(observer));
            }
        }
        return result;
    }

   public:
    virtual ~ISubject() { detachAll(); }

    void attach(ObserverPtr_t observerWeak) {
        std::lock_guard<EasyNetworkManagerMutex> lock(mutex);
        if (auto observer = observerWeak.lock()) {
            observers[observer->getID()] = observerWeak;
        }
    }

    void detach(const ObserverPtr_t& observerWeak) {
        std::lock_guard<EasyNetworkManagerMutex> lock(mutex);
        observers.erase(
            std::remove_if(
                observers.begin(), observers.end(),
                [&observerWeak](const ObserverPtr_t& observer) {
                    return observer.lock() == observerWeak.lock();
                }),
            observers.end());
    }

    void detach(uint64_t observerKey) {
        std::lock_guard<EasyNetworkManagerMutex> lock(mutex);
        observers.erase(observerKey);
    }

    void detachAll() {
        std::lock_guard<EasyNetworkManagerMutex> lock(mutex);
        observers.clear();
    }

    template <typename T = PayloadT>
    typename std::enable_if<!std::is_void<T>::value>::type notify(
        uint64_t key, EnumT event, const T& payload) {
        for (const auto& observer : snapshot(key, false)) {
            observer->update(event, payload);
        }
    }

    template <typename T = PayloadT>
    typename std::enable_if<!std::is_void<T>::value>::type notifyAll(
        EnumT event, const T& payload) {
        for (const auto& observer : snapshot(0, true)) {
            observer->update(event, payload);
        }
    }

    template <typename T = PayloadT>
    typename std::enable_if<std::is_void<T>::value>::type notify(
        uint64_t key, EnumT event) {
        for (const auto& observer : snapshot(key, false)) {
            observer->update(event);
        }
    }

    template <typename T = PayloadT>
    typename std::enable_if<std::is_void<T>::value>::type notifyAll(
        EnumT event) {
        for (const auto& observer : snapshot(0, true)) {
            observer->update(event);
        }
    }
};

}  // namespace Helpers

#else

#include_next <helpers/observer.hpp>

#endif  // ESP8266
