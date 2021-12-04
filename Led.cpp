#include <Arduino.h>
#include "Led.h"

// en el .cpp van los desarrollos

void Led::begin(uint8_t pinLed ){
      pin = pinLed;
      pinMode(pin,OUTPUT);
      state = false;
      digitalWrite(pin, state);
}
void Led::on(){
      state=true;
      digitalWrite(pin,state);

}
void Led::off(){
      state=false;
      digitalWrite(pin,state);
}
 void Led::cambio(){
      if(state==true){
             off();
      }else{
             on();
       }

}
