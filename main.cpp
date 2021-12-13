#include <Arduino.h>
#include"Timer.h"
#include <WiFi.h> 
#include <MQTT.h>

const char ssid[] = "xxx"; // Red Wifi por defecto
const char pass[] = "xxxx"; // Password de la Red Wifi

WiFiClient net;
MQTTClient client;

// Chequeo Wifi, Conecto y suscribo a topicos
void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("arduino")) { //, "public", "public")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nconnected!");

//Suscribimos a los topicos nodered
  /*client.subscribe("limiteHumedad"); //Motor con PWM
  client.subscribe("limiteTemp");  // Bomba de agua*/
  client.subscribe("LimitePH"); // Aereador
}



typedef enum {inicial,espera,midiendo,medido} estadosMed;
typedef enum {inicials,esperando, trabajo}estadoSalidas;
#define DHTTYPE DHT11
//===============pines de entrad sensores ===============
#define pin_dht 34 //este pin tiene que ser digital
#define pin_PH 33 // este pin tiene que ser analogico
#define Pin_Tapa 35 
//===============pines salidas de del sistema===========
#define pin_Motor 25
#define pin_Aereador 26
#define pin_Bomba 27

uint32_t tiempoRemover=172800000 ;//dos dias pasado a milli segundos 
float limiteHumedad=60.0;//limite inferior 
float LimiteTemp=65.0;//limite superior

//DHT dht(pin_dht,DHTTYPE);

Timer timerTemp,timerHumedad,timerPH,timerMotor,timerHAlert,timerTAlert;
bool MedTemperatura(float& medProbable,float& delta);
bool MedHumedad(float& medProbable,float& delta);
bool MedPH(float& medProbable, float& delta );
void ControlMotor();
void ControlAereador(float temperatura);
void ControlBomba(float humedad);



void setup() {
  //dht.begin();

  Serial.begin(115200);
  pinMode(pin_Motor,OUTPUT); // Motor
  pinMode(pin_Aereador,OUTPUT); // Aereador
  pinMode(pin_Bomba,OUTPUT); // Bomba
  pinMode(pin_PH,INPUT);  //PH
  pinMode(Pin_Tapa,INPUT);  //Tapa

  
  //ledcSetup(ledChannel,freq,resolution);
  //ledcAttachPin(25,ledChannel); // LED1 como PWM

  WiFi.begin(ssid, pass);

  //Configuro Servidor MQTT node Red
  client.begin("192.168.1.115", 1883, net);
 // client.onMessage(messageReceived);
  connect();
 
}

void loop() {
  client.loop(); // Si o si tiene que estar en el loop

  if (!client.connected()) {
    connect();

  }
  //======mediciones=======
  float medHum;
  float medPH  , deltaPH;
  float medTemp, deltaTemp;

  bool nuevaTemp, nuevaHum,nuevaPH;
  if(medHum>limiteHumedad){
    //el medidor de ph no funciona si no hay suficiente humedad
    nuevaPH   = MedPH(medPH,deltaPH);
  }

  nuevaTemp = MedTemperatura(medTemp,deltaTemp);
  nuevaHum  = MedHumedad(medHum,deltaTemp);

                                              
  //al haber nuevas mediciones se envian los datos nuevos
  
  //envio de señal mqtt
  if(nuevaHum){
    client.publish("humedad",String(medHum));
    Serial.print("Humedad Guardada");
    nuevaHum =false;
  }
  if(nuevaPH){
    client.publish("ph",String(medPH));
    Serial.print("PH Guardada");
     nuevaPH  =false;
  }
  if(nuevaTemp){
    client.publish("temp",String(medTemp));
    Serial.print("Temperatura Guardada");
    nuevaTemp = false;
    
  }

  //=======control de salidas=======
  ControlMotor();
  ControlBomba(medHum);
  ControlAereador(medTemp);

}

bool MedTemperatura(float& medProbable,float& delta){
  static estadosMed estado =espera;
  static int cantidadMed;
  static float mediciones[10];
  float medMax,medMin; 
  bool nuevaMedicion=false;

  if(estado==inicial){
    timerTemp.Set();
    estado=espera;
  }
  if(estado==espera ) {
    if(timerTemp>10000){
      estado=midiendo;
      timerTemp.Set();
    }
     
  }
  if(estado == midiendo) {
    if(timerTemp>2000){
      mediciones [cantidadMed]=(100+rand()%200)/10.00;
      //mediciones[cantidadMed]=dht.readTemperature();
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
      Serial.println(delta);
      /*for (int i = 0 ;i<10;i++){
      Serial.print(mediciones[i]);
      Serial.print(", ");
      }     */
      //===================================
  
    estado=espera;
    nuevaMedicion=true;
  
  }
  return nuevaMedicion;
}
bool MedHumedad(float& medProbable,float& delta){
  static estadosMed estado =espera;
  static int cantidadMed;
  static float mediciones[10];
  float medMax,medMin; 
  bool nuevaMedicion=false;

  if(estado==inicial){
    timerHumedad.Set();
    estado=espera;
 

  }
  if(estado==espera ) {
    if(timerHumedad>10000){
      estado=midiendo;
      timerHumedad.Set();
      
    }
     
  }
  if(estado == midiendo) {
    if(timerHumedad>3000){
      mediciones [cantidadMed]=(200+rand()%800)/10.00;
      //mediciones[cantidadMed]=dht.readHumidity();
      
     cantidadMed++;
     timerHumedad.Set();
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
    if(delta<5){
    //dado que el margen de error del propio sensor es de 5 grados 
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
    
    timerHumedad.Set();
     //=====================================
      Serial.print("medProbable :");
      Serial.println(medProbable);
      Serial.print("delta :");      
      Serial.println(delta);
      /*for (int i = 0 ;i<10;i++){
      Serial.print(mediciones[i]);
      Serial.print(", ");
      }     */
      //===================================
  
    estado=espera;
    nuevaMedicion = true;
  }
  return nuevaMedicion;
}
bool MedPH(float& medProbable, float& delta){
  static estadosMed estado =espera;
  static int cantidadMed;
  static float mediciones[10];
  float medMax,medMin; 
  bool nuevaMedicion=false;

  if(estado==inicial){
    timerPH.Set();
    estado=espera;
 

  }
  if(estado==espera ) {
    if(timerPH>300000){
      estado=midiendo;
      timerPH.Set();
      
    }
     
  }
  if(estado == midiendo) {
    if(timerPH>3000){
      mediciones [cantidadMed]=(rand()%140)/10.00;
      /*
      1023 - analogRead(pHpin)) / 73.07;
      */

      
     cantidadMed++;
     timerPH.Set();
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
    if(delta<5){
    //dado que el margen de error del propio sensor es de 5 grados 
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
    
    timerPH.Set();
     //=====================================
      Serial.print("medProbable :");
      Serial.println(medProbable);
      Serial.print("delta :");      
      Serial.println(delta);
      /*for (int i = 0 ;i<10;i++){
      Serial.print(mediciones[i]);
      Serial.print(", ");
      }     */
      //===================================
  
    estado=espera;
    nuevaMedicion = true;
  }
  return nuevaMedicion;
}

void ControlMotor(){
  static estadoSalidas estado = inicials;
  if (estado==inicials ){
    timerMotor.Set();
    
    estado = esperando;
  }
  if(estado==esperando){
    if(timerMotor>tiempoRemover&&digitalRead(Pin_Tapa)==LOW){

      estado = trabajo;
      timerMotor.Set();
    }
  }
  if(estado==trabajo){
      digitalWrite(pin_Motor,HIGH);
    if(timerMotor>60000){
      digitalWrite(pin_Motor,LOW);
      timerMotor.Set();
      estado = esperando;
    }    

  }
  
}
void ControlAereador(float temperatura){
  static estadoSalidas estado = inicials;
  if(estado==inicials){
      estado=esperando;
      timerTAlert.Set();

  }
  if(estado==esperando){
    if(temperatura>LimiteTemp){
      estado =trabajo;
      if(timerTAlert>10000){
      }
    }
  }
  if(estado==trabajo){
    digitalWrite(pin_Aereador,HIGH);
    if(temperatura<LimiteTemp-15.0){
      digitalWrite(pin_Aereador,LOW);
    }  
    estado=esperando;
  }

}
void ControlBomba(float humedad){
  static estadoSalidas estado=inicials;
  if(estado==inicials){
    estado=esperando;
  }
  if(estado==esperando){
    if(humedad<limiteHumedad){
      timerHAlert.Set();
      estado=trabajo;
    }

  }
  if(estado==trabajo){
    digitalWrite(pin_Bomba,HIGH);
    if(timerHAlert>10000){
    }
    if(humedad>limiteHumedad+10){
      digitalWrite(pin_Bomba,LOW);
      estado=esperando;
    }
  }
}
