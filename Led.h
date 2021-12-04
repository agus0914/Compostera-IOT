#pragma once 
#include<stdint.h>
// en el .h van las declaraciones
class Led
{
private:
      uint8_t pin;
      bool state;
public:
      void begin(uint8_t pinLed);
      void on();
      void off();
      void cambio();
      void brilloTiempo();
};
