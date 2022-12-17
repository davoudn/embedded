#include <Arduino_FreeRTOS.h>
#include <semphr.h>  // add the FreeRTOS functions for Semaphores (or Flags).
#include <Wire.h>
#include "ArduinoJson.h"
#include <ArxContainer.h>



// Declare a mutex Semaphore Handle which we will use to manage the Serial Port.
// It will be used to ensure only one Task is accessing this resource at any time.
SemaphoreHandle_t xSerialSemaphore;
// Logical control variables
//bool taskStarted = false;
//bool taskFinished = false;
bool command_arrived= false;
//TaskHandle_t taskMeassureHandle;
//TaskHandle_t taskRunHandle;

// define two Tasks for DigitalRead & AnalogRead
void taskControl( void *pvParameters );
void write_to_dac(int dac_id, float value);
void cellOff();
void taskListen( void *pvParameters );

void write_to_dac(int dac_id, float value){
  // transmit to device #4
  int dummy = (value/10.0)*4095;
  byte myBytes[2];
  myBytes[0] = dummy/256;
  myBytes[1] = dummy%256; 
  Wire.beginTransmission(96);
  Wire.write(myBytes[0]);  
  Wire.write(myBytes[1]);            // sends instruction byte  
 // Wire.write(x2);              // sends one byte  
  Wire.endTransmission();    // stop transmitting
    return;
}

// JSON variables
DeserializationError error;
StaticJsonDocument<120>  json_instruction, json_data, json_ins_arrived;
String json_string_instruction = "", str = ""; //read until timeout

struct Sample{
  float m_voltage;
  float m_current;
  float m_time_interval;
};

struct Flags{
  bool m_taskStarted;
  bool m_taskFinished;
};

//
struct cv{};
struct cc{};
template <class T>
struct Procedure;

 /* ----------------------------      Static functions      --------------------------------- */
template <class PROCTYPE>
 struct Base {
   static void taskApplyCheck(PROCTYPE* prc){
        prc->apply();
        for (;;){
          if (prc->m_flags.m_taskStarted && !prc->m_flags.m_taskFinished){
             prc->check_it();
             if (!prc->m_flags.m_taskFinished)
                prc->sendData();
             else
                prc->sendMessageFinished();
          }
       }
  }

  static void taskMeassure(PROCTYPE *prc){
     float averageValue = 500.0;
     int meassure_interval = 2000;
     long int sensorValue = 0;  // variable to store the sensor value read
     long int sensorValue1 = 0;

     float current_voltage = 0.0;
     float voltage_voltage=0.0;
     float time = 0;

     for (;;) {
  //  if (time >= meassure_interval ){
       sensorValue = 0;
       sensorValue1 = 0;
       for (int i = 0; i < averageValue; i++) {
           sensorValue += analogRead(A0);
           sensorValue1+= analogRead(A1);
           delay(0.5);
       }
       sensorValue = sensorValue / averageValue;
       current_voltage = sensorValue * 5.0 / 1024.0;
       prc->m_sample.m_current = (current_voltage - 2.5) / 0.066;
       prc->m_sample.m_voltage = 5.0 * sensorValue1 / averageValue / 1024.0; //
       prc->m_sample.m_time_interval = time*0.001; // in seconds!!
   // }
     }
  }
 };

struct  ProcHandle {
  StaticJsonDocument<120> j_status;
 
};

template <> struct Procedure<cv>: public ProcHandle {
   StaticJsonDocument<120> j_instruction, j_data, j_message;
   Sample m_sample;
   Flags  m_flags;
  Procedure (StaticJsonDocument<120> j_instruction_):j_instruction(j_instruction_) {}
  ~Procedure(){}

 void run(){
         xTaskCreate(Base<Procedure<cv>>::taskMeassure, "Meassureing"   ,  256,  &this->m_sample, 3,  NULL ); //Task Handle
         xTaskCreate(Base<Procedure<cv>>::taskApplyCheck   , "Runing the job",  256,  &this->m_flags , 3,   NULL );
  }

  void check_it(){ 
          float dummy = j_instruction["field_cutoff"];
          if (m_sample.m_current * m_sample.m_voltage < dummy * m_sample.m_voltage) done();
          
          return;    
  }

  void done(){
    m_flags.m_taskFinished = true;
    digitalWrite(50,LOW);
    cellOff();
    return;
  }

  void apply (){
         j_message["msg"]="jobstarted";
         j_message["type"]="message";
         serializeJson(j_message, Serial);
         pinMode(22, OUTPUT);
         digitalWrite(22, HIGH);     
         float applied_voltage = j_instruction["applied_field"];
         write_to_dac(96, applied_voltage);
         pinMode(50, OUTPUT);
         digitalWrite(50, HIGH);      
        return;
   }

   void sendData(){
         j_data["type"] = "data";
         j_data["current"] = m_sample.m_current;
         j_data["voltage"] = m_sample.m_voltage;
         j_data["time_interval"] = m_sample.m_time_interval;
         serializeJson(j_data, Serial); 
         Serial.print("#\n");    
    }

    void sendMessageFinished(){
     //       if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 1 ) == pdTRUE ){
         j_message["type"] = "message"  ;     
         j_message["msg"] = "jobdone"   ;  
         serializeJson(j_message, Serial);    
         Serial.print("#\n");    
                                                           //                         xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for other                                                      //   }            
    }
    void removeTasks(){
       json_data["current"] = 0.0;
       json_data["voltage"] = 0.0;
       m_sample.m_voltage=0.0;
       m_sample.m_current=0.0;
       m_sample.m_time_interval=0.0;
       // vTaskDelete(taskRunHandle);
  }
  };

 arx::vector<ProcHandle*> co;
void setup() {
  Serial.begin(9600);
  while (!Serial) {; }

  if ( xSerialSemaphore == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
  }

   xTaskCreate( taskControl  ,  "Control",  256,  NULL,  3,  NULL ); //Task Handle
   xTaskCreate( taskListen   ,  "Listen",  256,  NULL,  3,  NULL ); //Task Handle
}

void loop(){}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/
void taskControl( void *pvParameters )  // This is a Task.
{  
  int count =0;
  for (;;){
    //  xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.;
     if ( command_arrived  && co.size()==0 ){  
        if ( json_ins_arrived["command2"] == "cv" ){
            Procedure<cv>* proc_handle_cv = new  Procedure<cv> (json_ins_arrived);
            co.push_back(proc_handle_cv);
            proc_handle_cv->run();
        command_arrived = false;
       	}
        //taskStarted = true; 
     }    
     else if( command_arrived ){
            if (json_ins_arrived["command1"] = "cancel") {} 
            else {Serial.print("A task is running, first cancel the current task!!!");}
     }// Now free or "Give" the Serial         
     else if(!1 ) {
       }// Now free or "Give" the Serial                        
     } 
}
/*     ---------------------------------------------------------------------------- */
void taskListen( void *pvParameters )  // This is a Task.
{  
    int count =0;
  for (;;){
     //if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 1 ) == pdTRUE ){
      while (Serial.available() == 0) {}     //wait for data available
      json_string_instruction = Serial.readString();  //read until timeout
    //  xSemaphoreGive( xSerialSemaphore );  
      Serial.print(json_string_instruction); 
      str = json_string_instruction; 
      deserializeJson(json_ins_arrived, str);
      command_arrived = true;
    //  xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.;
  }
}

void cellOff()
{
  return;  
}

