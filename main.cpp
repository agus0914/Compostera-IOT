#include <Arduino.h>
#include"Timer.h"
typedef enum {inicial,espera,midiendo,medido} estadosMed;

Timer timerTemp;
float MedTemp(float& medProbable,float& delta);

void setup() {
   Serial.begin(115200);
}

void loop() {
  float medTemp,deltaTemp;

  MedTemp(medTemp,deltaTemp);
  

}

float MedTemp(float& medProbable,float& delta){
  static estadosMed estado =espera;
  static int cantidadMed;
  static float mediciones[10];
  float medMax,medMin; 

  if(estado==inicial){
    timerTemp.Set();
    estado=espera;
 

  }
  if(estado==espera ) {
    if(timerTemp>1000){
      estado=midiendo;
      timerTemp.Set();
      
    }
     
  }
  if(estado == midiendo) {
    if(timerTemp>10){
      mediciones [cantidadMed]=(100+rand()%200)/10.00;
      
     cantidadMed++;
     timerTemp.Set();
    }
    if(cantidadMed==10){
      estado=medido;
      cantidadMed=0;
    }
   

  }
  if (estado == medido) {
    medMax=mediciones[0];
    medMin=mediciones[0];
    for(int i=0;i<10;i++){

      if(mediciones[i]>medMax){
        medMax=mediciones[i];
        
     


      }
      if(mediciones[i]<medMin){
        medMin=mediciones[i];
      }
    }
    medProbable=(medMin+medMax)/2.0;
    
    delta=medProbable-medMin;
    if(delta<2){
    //dado que el margen de error del propio sensor es de 2 grados 
    //no es coerente que el margen de error de las mediciones se inferior 
      delta=2;
    }
    /*
    //este seria un simple promedio de las mediciones 
    for(int i =0;i<10;){
      medProbable=medProbable+mediciones[i];

    }
    medProbabable/10;
    */
    
    timerTemp.Set();
     //=====================================
      Serial.print("medProbable :");
      Serial.println(medProbable);
      Serial.print("delta :");      
      Serial.print(delta);
      /*for (int i = 0 ;i<10;i++){
      Serial.print(mediciones[i]);
      Serial.print(", ");
      }     */
      //===================================
  
    estado=espera;
  
  }
}
