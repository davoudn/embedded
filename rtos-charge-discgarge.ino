#include <Arduino_FreeRTOS.h>
#include <semphr.h>  // add the FreeRTOS functions for Semaphores (or Flags).
#include <Wire.h>
#include "ArduinoJson.h"



// Declare a mutex Semaphore Handle which we will use to manage the Serial Port.
// It will be used to ensure only one Task is accessing this resource at any time.
SemaphoreHandle_t xSerialSemaphore;
// Logical control variables
//bool taskStarted = false;
//bool taskFinished = false;
bool command_arrived= false;
TaskHandle_t taskMeassureHandle;
TaskHandle_t taskRunHandle;

// define two Tasks for DigitalRead & AnalogRead
void taskControl( void *pvParameters );
void write_to_dac(int dac_id, float value);
void cellOff();
void taskListen( void *pvParameters );

void write_to_dac(int dac_id, float value){
  // transmit to device davoud 22222 
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
}

//
struct cv{};
struct cc{};
template <class T>
struct procedure;


template <typename PROCTYPE>
sctruct  base {
  private:
    PROCTYPE proc;

  public:

  base(StaticJsonDocument<120>& j_instruction_):proc(j_instruction_){}
  ~base(){}
  
  void run(){
         xTaskCreate( base<PROCTYPE>::taskMeassure, "Meassureing"   ,  256,  &proc->m_sample, 3,   &taskMeassureHandle ); //Task Handle
         xTaskCreate( base<PROCTYPE>::taskCheck   , "Runing the job",  256,  &proc->m_flags , 3,   &taskRunHandle );
  }
  
  void ckeck_it(){ 
          proc->check_it();
          return;    
  }

  void apply (){
        proc->apply();  
        return;
  }
 
  void sendData(){
       proc->sendData();
  }

  void sendMessageFinished(){
   //       if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 1 ) == pdTRUE ){
       proc->sendMessageFinished();
                                                           //                         xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others. 
  }

  void removeTasks(){
       proc->json_data["current"] = 0.0;
       proc->json_data["voltage"] = 0.0;
       proc->m_sample.m_voltage=0.0;
       proc->m_sample.m_current=0.0;
       proc->m_sample.m_time_interval=0.0;
       // vTaskDelete(taskRunHandle);
  }

  /* ----------------------------      Static functions      --------------------------------- */

   static void taskCheck(PROCTYPE* prc){
        for (;;){
          if (prc->taskStarted && !prc->taskFinished){
             prc->check_it();
             if (!prc->taskFinished)
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
       prc->sample.m_current = (current_voltage - 2.5) / 0.066;
       prc->sample.m_voltage = 5.0 * sensorValue1 / averageValue / 1024.0; //
       prc->sample.m_time_interval = time*0.001; // in seconds!!
   // }
     }
  }

};


template <> struct procedure<cv> {
   StaticJsonDocument<120> j_instruction, j_data, j_message;
   Sample m_sample;
   Flags  m_flags;
  procedure (StaticJsonDocument<120> j_instruction_):j_instruction(j_instruction_) {}
  ~procedure(){}

  void ckeck_it(){ 
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
         json_message["msg"]="jobstarted";
         json_message["type"]="message";
         serializeJson(json_message, Serial);
         pinMode(22, OUTPUT);
         digitalWrite(22, HIGH);     
         float applied_voltage = json_instruction_["applied_field"];
         write_to_dac(96, applied_voltage);
         pinMode(50, OUTPUT);
         digitalWrite(50, HIGH);      
        return;
   }

   void sendData(){
         json_data["type"] = "data";
         json_data["current"] = sample.m_current;
         json_data["voltage"] = sample.m_voltage;
         json_data["time_interval"] = sample.m_time_interval;
         serializeJson(json_data, Serial); 
         Serial.print("#\n");    
    }

    void sendMessageFinished(){
     //       if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 1 ) == pdTRUE ){
         json_message["type"] = "message"  ;     
         json_message["msg"] = "jobdone"   ;  
         serializeJson(json_message, Serial);    
         Serial.print("#\n");    
                                                           //                         xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
                                                           //   }            
    }
  };



// the setup function runs once when you press reset or power the board
void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
  taskStarted =false; 
  taskFinished=false;
  // Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
  // because it is sharing a resource, such as the Serial port.
  // Semaphores should only be used whilst the scheduler is running, but we can set it up here.
  if ( xSerialSemaphore == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
  }

   //Now set up two Tasks to run independently.
   xTaskCreate(
    taskControl
    ,  "Control"  // A name just for humans
    ,  256  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL //Parameters for the task
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL ); //Task Handle
   //Now set up two Tasks to run independently.
   xTaskCreate(
    taskListen
    ,  "Listen"  // A name just for humans
    ,  256  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL //Parameters for the task
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL ); //Task Handle
  // Now the Task scheduler, which takes over control of scheduling individual Tasks, is automatically started.
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
    // Now set up two Tasks to run independently.
     if ( command_arrived && !taskStarted && !taskFinished ){  
        if (json_ins_arrived["command2"] == "cv"){
            Base < Procedure<cv> > base_proc_cv (json_ins_arrived).run();
        //xTaskCreate( taskMeassure, "Meassureing"   ,  256,  NULL, 3,   &taskMeassureHandle ); //Task Handle
        //xTaskCreate( taskRun     , "Runing the job",  256,  NULL,  3,   &taskRunHandle     ); //Task Handle    
	}
        taskStarted = true; 
        command_arrived = false;          
        Serial.print("Started!!");      
        json_instruction = json_ins_arrived;            
     } 
     else if( command_arrived && taskStarted && !taskFinished){
            if (json_ins_arrived["command1"] = "cancel") {} 
            else {Serial.print("A task is running, first cancel the current task!!!");}
     }// Now free or "Give" the Serial         
     else if( taskStarted && taskFinished ) {
          //removeTasks(NULL);
         // removeTasks(taskRunHandle);
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

