#pragma once
#include <stdint.h>
class Timer {
      private:
      uint32_t t0;
      public:
      void Set();
      uint32_t deltaTiempo();
      bool operator> (uint32_t ms);
};