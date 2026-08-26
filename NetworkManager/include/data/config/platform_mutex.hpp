#pragma once

#if defined(ESP8266)

#include <Arduino.h>
#include <atomic>

// ESP8266 has no FreeRTOS mutex implementation. Use its available C++ atomic
// primitive while yielding so callbacks from the cooperative runtime do not
// busy-spin indefinitely.
class EasyNetworkManagerMutex {
    std::atomic_flag locked = ATOMIC_FLAG_INIT;

   public:
    void lock() {
        while (locked.test_and_set(std::memory_order_acquire)) {
            yield();
        }
    }

    void unlock() { locked.clear(std::memory_order_release); }
};

#endif  // ESP8266
