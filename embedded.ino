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
int  command = 0, signal=0, no_of_procs=0;
//TaskHandle_t taskMeassureHandle;
//TaskHandle_t taskRunHandle;

// define two Tasks for DigitalRead & AnalogRead
void taskControl( void *pvParameters );
void write_to_dac(int dac_id, float value);
void cellOff();
void taskListen( void *pvParameters );

void write_to_dac(int dac_id, float value)
{
  // transmit to device #4
  int dummy = (value/10.0)*4095;
  //Serial.print (value);
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
   Flags():m_taskStarted(0),m_taskFinished(0){}
   Flags(int m1, int m2):m_taskStarted(m1),m_taskFinished(m2){}
  int m_taskStarted;
  int m_taskFinished;
};

//
struct cv{};
struct cc{};
template <typename PROCTYPE>
struct Procedure;
struct ProcHandle;

struct BaseProc{
  BaseProc( StaticJsonDocument<120> j_instruction_):j_instruction(j_instruction_){}
   StaticJsonDocument<120> j_status;
   StaticJsonDocument<120> j_instruction;
   Sample m_sample;
   Flags  m_flags;

   virtual void apply(){;}
   virtual void check_it(){;}

 };
 

 template <> struct Procedure<cv>:public BaseProc {
    Procedure<cv>( StaticJsonDocument<120> j_instruction_): BaseProc( j_instruction_) {}

  void check_it(){ 
          float dummy = j_instruction["field_cutoff"];
          if (m_sample.m_current * m_sample.m_voltage < dummy * m_sample.m_voltage) {
                m_flags.m_taskFinished = 1;
                digitalWrite(50,LOW);
                cellOff();
          }
          
          return;    
  }

  void apply (){
        // j_message["msg"]="jobstarted";
        // j_message["type"]="message";
        // serializeJson(j_message, Serial);
        //Serial.print("dddddddddd");
         pinMode(22, OUTPUT);
         digitalWrite(22, HIGH);     
         float applied_voltage = j_instruction["applied_field"];
         write_to_dac(96, applied_voltage);
         pinMode(50, OUTPUT);
         digitalWrite(50, HIGH);      
         m_flags.m_taskStarted = 1;
        return;
   }


  };

 
 /* ----------------------------      Static functions      --------------------------------- */
template <class PROCHANDLE>
 struct Base {
   static void taskApplyCheck(PROCHANDLE* prochandle){
        prochandle->proc->apply();
      
     //   Serial.print(prc->m_flags.m_taskStarted);
     //   Serial.print(prc->m_flags.m_taskFinished);
        for (;;){
           if (prochandle->proc->m_flags.m_taskStarted && !prochandle->proc->m_flags.m_taskFinished){
            // Serial.print("Checking data\n");
              prochandle->proc->check_it();
              if (!prochandle->proc->m_flags.m_taskFinished){
            //    Serial.print("Sending data\n");
                  prochandle->sendData();
              }                
              else{   
                prochandle->sendMessageFinished();
                prochandle->removeTasks();      
              }                 
           }
          Meassure(prochandle);
        }
  }

  static void taskMeassure(PROCHANDLE* prochandle){
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
       prochandle->proc->m_sample.m_current = (current_voltage - 2.5) / 0.066;
       prochandle->proc->m_sample.m_voltage = 5.0 * sensorValue1 / averageValue / 1024.0; //
       prochandle->proc->m_sample.m_time_interval = time*0.001; // in seconds!!
       prochandle->proc->sendData();       
   // }
     }
  }

static void Meassure(PROCHANDLE* prochandle){
     int meassure_interval = 2000;
     long int sensorValue = 0, sensorValue1 = 0;
     float current_voltage = 0.0, voltage_voltage=0.0, averageValue = 500.0;
     float time = 0;

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
       prochandle->proc->m_sample.m_current = (current_voltage - 2.5) / 0.066;
       prochandle->proc->m_sample.m_voltage = 5.0 * sensorValue1 / averageValue / 1024.0; //
       prochandle->proc->m_sample.m_time_interval = time*0.001; // in seconds!!    
  }  
static void taskControl( PROCHANDLE *prochandle )  // This is a Task.
{  
  int count =0;
  //Serial.print(command);
  for (;;){
      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.;      
    //if  (co->)
    if ( command_arrived  && prochandle->no_of_procs==0 ){  
        if ( json_ins_arrived["command2"] == "cv" ){
            
            //Serial.print(no_of_procs);
            prochandle->proc = new  Procedure<cv> (json_ins_arrived);
            prochandle->no_of_procs++;         
            xTaskCreate(Base<ProcHandle>::taskApplyCheck   , "Runing jobs",  256,  prochandle , 3,   &prochandle->taskApplycheckHandle );
   
            command_arrived = false;
       	}
    }
    else {
      if( command_arrived && prochandle->no_of_procs==1 ){
            if (json_ins_arrived["command1"] = "cancel") {} 
            else {
              Serial.print("A task is running, first cancel the current task!!!");
            }             
      }
    } 
}
}
/*     ---------------------------------------------------------------------------- */
static void taskListen( PROCHANDLE* prochandle )  // This is a Task.
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
      command_arrived= true;
/*if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 1 ) == pdTRUE ){      
      command = 1;
      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.;
}   */
//Serial.print(signal);   

  }
}

};

struct ProcHandle {
   StaticJsonDocument<120> j_instruction, j_data, j_message;
   BaseProc* proc;
   TaskHandle_t  taskApplycheckHandle;
   int no_of_procs;
  //ProcHandle (StaticJsonDocument<120> j_instruction_):j_instruction(j_instruction_), m_flags() {}
    ProcHandle ():no_of_procs(0) {}
   ~ProcHandle (){}

 void run(){
         xTaskCreate( Base<ProcHandle>::taskControl  ,  "Control",  256,  this,  3,  NULL ); //Task Handle
         xTaskCreate( Base<ProcHandle>::taskListen   ,  "Listen",  256,  this,  3,  NULL ); //Task Handle
  }


   void sendData(){
         j_data["type"] = "data";
         j_data["current"] = proc->m_sample.m_current;
         j_data["voltage"] = proc->m_sample.m_voltage;
         j_data["time_interval"] = proc->m_sample.m_time_interval;
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
       proc->m_sample.m_voltage=0.0;
       proc->m_sample.m_current=0.0;
       proc->m_sample.m_time_interval=0.0;
       vTaskDelete(taskApplycheckHandle);
  }
  };

 ProcHandle* ph;
void setup() {
  Serial.begin(9600);
  while (!Serial) {; }

  if ( xSerialSemaphore == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
  }
  ph = new ProcHandle();
  ph->run();
}




void loop(){}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void cellOff()
{
  return;  
}

