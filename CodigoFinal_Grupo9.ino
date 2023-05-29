/* 
Proyecto final Instrumentación electrónica.
El código permite dosificar, pesar, medir la temperatura y mezclar.

Comandos desde consola de Arduino: 
- Si quiere mezclar ingrese el valor de la velocidad al que desea mezclar (vel= ) 
- Si quiere dosificar ingrese el valor de ml que desea (v= )

Autores: 
Juan Manuel Silva 
Sara Angarita
Federico De Angulo
Federico Berbesi 

Mayo 26 del 2023

*/

//// Librerias ////
#include <Wire.h> 
#include <OneWire.h>
#include <DallasTemperature.h>
#include "HX711.h"
//#include <SoftwareSerial.h>

//// Constantes simbolicas ////
#define DT_Balanza  A1                                      // Pin módulo HX711 DT
#define SCK_Balanza  A0                                     // Pin módulo HX711 SCK 
#define PWM_1  9                                            // Pin bomba de agua
#define PWM_2  10                                           // Pin bomba de agua
#define StepPin 4                                           // Pin motor paso a paso
#define Encoder_pin 8                                       // Pin de conexión del encoder  
#define PulsosRotacion 20                                   // Valor de pulsos de rotación del encoder


////  Pines ////
const int pinDatosDQ = 2;                                   // Pin de conexión con el sensor DS18B20

//// Declaración de objetos ////
OneWire oneWireObjeto(pinDatosDQ);                          // Creación del objeto OneWire del sensor DS18B20
DallasTemperature sensorDS18B20(&oneWireObjeto);            // Creación de la instancia DallasTemperature del sensor DS18B20
HX711 balanza(DT_Balanza, SCK_Balanza);                     // Creación del objeto HX711 de la balanza
//SoftwareSerial miBT(11,12);

/////  Declaración de variables /////

String estado;
String estado2 = "";

float escala_balanza = 1087;                               // Valor de calibración de la balanza
float a;                                                   // Primera medición de la balanza
float b;                                                   // Segunda medición de la balanza


int Densidad = 1;                                          // Valor de la densidad del agua 
int v = 0;                                                 // Volumen deseado
int OnOff = 1;                                             // Switch On/Off para la bomba  (1 = On | 0 = Off)
float pesoRecipiente = 0;
float pesoLiquido = 0;


int Encoder = 0;                                           // Valor del Output del encoder 
int Estado = 1;                                            // Estado actual del codificador para reconocer los cambios de pulso
int Pulsos = 0;                                            // Contador de pulsos en el motor paso a paso
int previousV = 0;
int previousVel = 0;

unsigned int t=0;                                          // Tiempo del motor paso a paso
unsigned int vel=0;                                        // Rango: 980 - 9800 que corresponde a 150 - 15 RPMs aprox.
unsigned long InitialTime = 0;                             // Variable que almacena el tiempo inicial 
unsigned long FinalTime;                                   // Variable que almacena el tiempo final 
float RPMs = 0;                                                // Inicializar variable de RPMs
  
String inputString;                                        // Variable para almacenar la cadena de entrada


///// Banderas /////
bool recibeVel = false;                                   // Bandera de que se recibio una entrada para mezclar
bool recibeAgua = false;                                  // Bandera de que se recibio una entrada para dosificar
bool printed = false;                                     // Bandera para imprimir


///// Timers /////
float tiempoAnterior = 0;
float tiempoInicial;


int valor = 0;

void setup() {

    Serial.begin(9600);                                 // Velocidad de comunicación  
    
                                       
    
    // Balanza
    balanza.tare(25);
    balanza.set_scale(escala_balanza);

    // Motor paso a paso
    pinMode( StepPin , OUTPUT);                           // Inicializar pines del motor paso a paso
    pinMode( Encoder_pin , INPUT);                        // Inicializar pines del motor paso a paso

    // Sensor de temperatura 
    sensorDS18B20.begin();                                // Incializar sensor de temperatura DS18B20
    const uint8_t resol=9;                                // Resolución del sensor de temperatura DS18B20 
    sensorDS18B20.setResolution(resol);                   // Configurar la resolución deseada en el sensor de temperatura DS18B20 
    sensorDS18B20.setWaitForConversion(false);            // Desactivar la espera de conversión 

    //tiempoInicial = millis();
    
}


void loop() {
  
  
  float tiempoActual = millis();                         // Obtener el tiempo actual desde que se inicio el programa
  if (tiempoActual - tiempoAnterior >= 3000) {
      
      tiempoAnterior = tiempoActual;                     // Actualizar el tiempo anterior
                        

      
      sensorDS18B20.requestTemperatures();               // Lectura de la temperatura del sensor DS18B20

      Serial.print(RPMs);
      Serial.print(" RPM");   // Imprimir la Variable de los RPM del motor paso a paso
      Serial.print("_");
      
      Serial.print(sensorDS18B20.getTempCByIndex(0)*1.0091+2.1588);    // Obtener la lectura e imprimir los datos del sensor DS18B20
      Serial.print("°C");
      Serial.print("_");

      
      Serial.print(balanza.get_units(1));             // Leer e imprimir los valores de la pesa
      Serial.println("g");
     
      

    }


///////// Ingresar información desde Serial ///////

  if (Serial.available()) {                      // Inicia comunicación con el módulo Bluetooth HC-05
  
      inputString = Serial.readString();
      char seleccionador = inputString.charAt(0);
      valor = inputString.substring(1).toInt();

      if(seleccionador=='b'){                     // Comando desde la aplicación cuando se oprime el botón de emergencia
        recibeAgua=false;
        recibeVel= false;
        t = 0;
      }
      
      if (seleccionador == 'v') {                 // Comando desde la aplicación cuando se recibe el valor de dosificación
        
        v = valor;
        
        if (v != 0) {
          recibeAgua = true;
        } 

        Dosificar(v);  
      }

      if (seleccionador == 's') {                // Comando desde la aplicación cuando se recibe el valor de mezclar
        
        vel = valor;

        if (vel == 0){
          t = 0;
        }

        else if (vel<97){
          recibeVel = true;
          t = 140379*(1/pow(vel,0.989));
        }

        else if (vel<140){
          recibeVel = true;
          t = 140379*(1/pow(vel,0.99));
        }
          
        else {
          recibeVel = true;
          t = 140350*(1/pow(vel,0.99));   
        } 
      }

      
  }
  
    if (recibeVel == true){
    Mezclar(t);
    }

}

///////// Función de dosificar /////////

void Dosificar(int v){

  if (recibeAgua == true){                        // Si se recibe por el serial la cantidad a dosificar se activa la función
      pesoLiquido = balanza.get_units(15);        // Promedio de la medición de la balanza entre 15 tomas
      while ((v / Densidad) - 5> pesoLiquido){   
        analogWrite(PWM_1, 170);

        a = balanza.get_units(1);                  // Primera medición de la balanza
        b = balanza.get_units(1);                  // Segunda medición de la balanza
        
        pesoLiquido = min (a,b);
      }

      if ((v / Densidad)*0.8 < pesoLiquido){        // Si el valor que se desea es menor al peso que se toma de la balanza ejecute

        OnOff = 0;                                // Apagar la bomba

        analogWrite(PWM_1, 0);

        
      }
  }
  
}


////// Función de mezclar //////

void Mezclar(int t){
 
    digitalWrite(StepPin, HIGH);                    // Activar el motor paso a paso
    delayMicroseconds(t);                           // Delay de espera
    digitalWrite(StepPin, LOW);                     // Desactivar el motor paso a paso
    delayMicroseconds(t);                           // Delay de espera
    
    Encoder = digitalRead(Encoder_pin);             // Lectura del encoder
  
    if(Encoder == LOW && Estado == 1){
      
      Estado = 0;
                 
    }
  
    if(Encoder == HIGH && Estado == 0){
      Pulsos++;                                      // Se suma al contador de pulsos
      Estado = 1;
        
    }
  
    if(Pulsos == PulsosRotacion){

      FinalTime = millis();                        
      RPMs = 60000/(FinalTime - InitialTime);      // Calculo de la velocidad de los RPMs
      Pulsos = 0;                                  // Contador pulsos 
      InitialTime = FinalTime;                     // Se vuelve a inicalizar la variable de tiempo inicial con el valor actual del tiempo final 
   
    }
  
    if (t < 0){                                    // Si el t es menor a 0 parar de mezclar
      t = 0;
    }
 
}
