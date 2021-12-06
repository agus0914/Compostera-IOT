#include <Arduino.h>
#include<DHT.h>
#include"Timer.h"
#include <WiFi.h> //<WiFi101.h>
#include <MQTT.h>

const char ssid[] = "cotemax_d88";
const char pass[] = "cotea01264";

WiFiClient net;
MQTTClient client;

const int freq =5000; // freq for PWM
const int ledChannel = 0; // channel pwm
const int resolution = 10; // resolution 8,10,12,15

int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by

long duration;
float distanceCm;
#define SOUND_SPEED 0.034
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;


// Chequeo Wifi, Conecto y suscribo a topicos
void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

//Conectamos a Wifi

  Serial.print("\nconnecting...");
  while (!client.connect("arduino")) { //, "public", "public")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

//Suscribimos a los topicos nodered
  client.subscribe("brillo"); //Motor con PWM
  client.subscribe("led1");  // Bomba de agua
  client.subscribe("led2"); // Aereador

}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  // set the brightness of pin 2:
  
  if (topic=="brillo") {
      ledcWrite(ledChannel, payload.toInt());
  }  
  if (topic=="led1") {
    if (payload=="on") digitalWrite(26, HIGH);
    if (payload=="off") digitalWrite(26, LOW);
  }
  if (topic=="led2") {
    if (payload=="on") digitalWrite(27, HIGH);
    if (payload=="off") digitalWrite(27, LOW);
  }
  // Note: Do not use the client in the callback to publish, subscribe or
  // unsubscribe as it may cause deadlocks when other things arrive while
  // sending and receiving acknowledgments. Instead, change a global variable,
  // or push to a queue and handle it in the loop after calling `client.loop()`.
}



typedef enum {inicial,espera,midiendo,medido} estadosMed;
#define DHTTYPE DHT11
//===============pines de entrad y salida===============
#define pin_dht 2 

DHT dht(pin_dht,DHTTYPE);

Timer timerTemp,timerHumedad;
void MedTemperatura(float& medProbable,float& delta);
void MedHumedad(float& medProbable,float& delta);

void setup() {
  dht.begin();
  Serial.begin(115200);
  pinMode(25,OUTPUT); // Motor
  pinMode(26,OUTPUT); // Aereador
  pinMode(27,OUTPUT); // Bomba
  pinMode(34,INPUT);  //PH
  pinMode(35,INPUT);  //Tapa

  pinMode(13,INPUT);  // ECHO
  pinMode(12,OUTPUT); // pulse
  ledcSetup(ledChannel,freq,resolution);
  ledcAttachPin(25,ledChannel); // LED1 como PWM

  WiFi.begin(ssid, pass);

  //Configuro Servidor MQTT node Red
  client.begin("192.168.1.135", 1883, net);
  client.onMessage(messageReceived);

  connect();
 
}

void loop() {
  client.loop(); // Si o si tiene que estar en el loop

  if (!client.connected()) {
    connect();
  }
  float medTemp,deltaTemp;
  float medHum ,deltaHum;
  MedTemperatura(medTemp,deltaTemp);
  MedHumedad(medHum,deltaTemp);
  
  

}

void MedTemperatura(float& medProbable,float& delta){
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
  
  }
}
void MedHumedad(float& medProbable,float& delta){
  static estadosMed estado =espera;
  static int cantidadMed;
  static float mediciones[10];
  float medMax,medMin; 

  if(estado==inicial){
    timerHumedad.Set();
    estado=espera;
 

  }
  if(estado==espera ) {
    if(timerHumedad>1000){
      estado=midiendo;
      timerHumedad.Set();
      
    }
     
  }
  if(estado == midiendo) {
    if(timerHumedad>10){
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
  
  }
}
