#pragma once

#include <mbed.h>

#include <functional>
#include <memory>

#include <chrono>
using namespace std::chrono_literals;

template <class T, uint32_t SZ = 4>
class EventQueueMemPool {
public:

  // blocking event handler dispatcher
  void wait_for(rtos::Kernel::Clock::duration_u32 how_long = 1s) {
    T * ptr = nullptr;
    if (evQueue.try_get_for(how_long, &ptr)) {
      if (evHandler) {
        evHandler(*ptr); 
      }
      evPool.free(ptr);
    }
  }

  // non-blocking event handler dispatcher
  void poll() {
    T * ptr = nullptr;
    while (evQueue.try_get(&ptr)) {
      if (evHandler) {
        evHandler(*ptr); 
      }
      evPool.free(ptr);
    }
  }

  // 
  void handleQueue(std::function<void(T const &)> handler) {
    T * ptr = nullptr;
    while (evQueue.try_get(&ptr)) {
      if (evHandler) {
        handler(*ptr); 
      }
      evPool.free(ptr);
    }
  }

  // 
  T * getBuffer(bool commit = true) {
    if (dmaBuffer) {
      if (commit && evQueue.try_put(dmaBuffer.get())) {
        dmaBuffer.release(); // remove ownership
        dmaBuffer.reset(evPool.try_calloc()); // setup new dma buffer (if try_put failed, previous one will be freed)
      }
    } else {
      dmaBuffer.reset(evPool.try_calloc()); // setup new dma buffer (if try_put failed, previous one will be freed)
    }
    return dmaBuffer.get(); // keep the ownership
  }

  void free(T * ptr) {
    Serial.print("Freed: 0x");
    Serial.println(uint32_t(ptr), HEX);
    evPool.free(ptr);
  }

  rtos::MemoryPool<T,SZ>          evPool;
  rtos::Queue<T,SZ>               evQueue;
  std::function<void(T const &)>  evHandler;

  std::unique_ptr<T, std::function<void(T*)>>  dmaBuffer{ nullptr, std::bind(&EventQueueMemPool<T,SZ>::free, this, std::placeholders::_1) };

};