#include "Timer.h"
#include <Arduino.h>

void Timer::Set() {
      t0 = ~millis() + 1;
}
uint32_t Timer::deltaTiempo(){
      uint32_t dTiempo = t0 + millis();
      return dTiempo;
} 
 bool Timer::operator>(uint32_t ms){
      return deltaTiempo() > ms;
 }