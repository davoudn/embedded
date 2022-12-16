#include <Arduino_FreeRTOS.h>
#include <semphr.h>  // add the FreeRTOS functions for Semaphores (or Flags).
#include <Wire.h>
#include "ArduinoJson.h"



// Declare a mutex Semaphore Handle which we will use to manage the Serial Port.
// It will be used to ensure only one Task is accessing this resource at any time.
SemaphoreHandle_t xSerialSemaphore;
// Logical control variables
bool taskStarted = false;
bool taskFinished = false;
bool command_arrived= false;
TaskHandle_t taskMeassureHandle;
TaskHandle_t taskRunHandle;

// JSON variables
DeserializationError error;
StaticJsonDocument<120>  json_instruction, json_data, json_message, json_ins_arrived;
String json_string_instruction = "", str = ""; //read until timeout

struct Sample{
  float m_voltage;
  float m_current;
  float m_time_interval;
};

Sample sample;
// define two Tasks for DigitalRead & AnalogRead
void taskChargeDischarge( void *pvParameters );
void taskChargeDischarge( void *pvParameters );
void taskControl( void *pvParameters );
void removeTasks( void *pvParameters );
void write_to_dac(int dac_id, float value);
void cellOff();
//
struct cv{
};

struct  cc{
  static void finished(bool& para){
                  digitalWrite(50,LOW); 
                  para = true;
                  cellOff();  
                  }  

};

template <class T>
struct check;

template <> struct check<cv>{
  
  static void ckeck_it(Sample& s, StaticJsonDocument<120>& jpara, bool& para, void (*action)(bool&)){ 
    float dummy = jpara["field_cutoff"];
    if (s.m_current * s.m_voltage < dummy * s.m_voltage) action(para);
    return;    
    }
  static void finished(bool& para){
                  digitalWrite(50,LOW); 
                  para = true;
                  cellOff();  
                  return;
                  } 
  static void apply (StaticJsonDocument<120>& json_instruction_){
          pinMode(22, OUTPUT);
          digitalWrite(22, HIGH);     
          float applied_voltage = json_instruction_["applied_field"];
          write_to_dac(96, applied_voltage);
          pinMode(50, OUTPUT);
          digitalWrite(50, HIGH);      
          return;
  }
  static void run(StaticJsonDocument<120>& json_arrived_){
         xTaskCreate( taskMeassure, "Meassureing"   ,  256,  NULL, 3,   &taskMeassureHandle ); //Task Handle
         xTaskCreate( check<cv>::taskRun     , "Runing the job",  256,  NULL,  3,   &taskRunHandle );
	 }
  static void taskRun(StaticJsonDocument<120>& json_arrived_){
  	check<cv>::apply(json_instruction);
        json_message["msg"]="jobstarted";
	json_message["type"]="message";
	serializeJson(json_message, Serial); 
  	for (;;){
          if (taskStarted && !taskFinished){
                                        check<cv>::do_it(sample,json_instruction,taskFinished,&check<cv>::finished);
                                        if (!taskFinished) 
                                               sendData(json_data);
                                        else                
                                               sendMessageFinished();
          }   
       }
  }

void removeTasks(void *any)
{
  json_data["current"] = 0.0;
  json_data["voltage"] = 0.0;
  sample.m_voltage=0.0;
  sample.m_current=0.0;
  sample.m_time_interval=0.0;
 // vTaskDelete(taskMeassureHandle);
 // vTaskDelete(taskRunHandle);
       

  }
};

template <> struct check<cc>{
  static void check_it(Sample& s, StaticJsonDocument<120>& jpara, bool& para, void (*action)(bool&)){ 
    float dummy = jpara["field_cutoff"];
    if (s.m_voltage * s.m_current > dummy * s.m_current) action(para);
    }
    
  static void finished(bool& para){
                  digitalWrite(50,LOW); 
                  para = true;
                  cellOff();  
                  return;
                  } 
                  
  static void apply (StaticJsonDocument<120>& json_instruction_){
                                                     pinMode(22, OUTPUT);
                                                     digitalWrite(22,LOW);   //set pin 8 HIG*/   
                                                     float applied_current = json_instruction["applied_field"];
                                                     write_to_dac(96, applied_current);
                                                     pinMode(50, OUTPUT);
                                                     digitalWrite(50,HIGH);   //set pin 8 HIG*/     
          return;
  }
};

void sendData(StaticJsonDocument<120>& json_data_){
    json_data_["type"] = "data";
    json_data_["current"] = sample.m_current;
    json_data_["voltage"] = sample.m_voltage;
    json_data_["time_interval"] = sample.m_time_interval;
    serializeJson(json_data_, Serial); 
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
        //xTaskCreate( taskMeassure, "Meassureing"   ,  256,  NULL, 3,   &taskMeassureHandle ); //Task Handle
        //xTaskCreate( taskRun     , "Runing the job",  256,  NULL,  3,   &taskRunHandle     ); //Task Handle    
	  check<cv>::run(json_ins_arrived);
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
          removeTasks(NULL);
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
/* ----------------------------------------------------------------------------------------- */
void taskMeassure( void *pvParameters)  // This is a Task.
{
    // make the pushbutton's pin an input:
// Sensor reading aux variables
const float averageValue = 500.0;
const int meassure_interval = 2000;
long int sensorValue = 0;  // variable to store the sensor value read
long int sensorValue1 = 0;

float current_voltage = 0.0;
float voltage_voltage=0.0;
float time = 0;    

  for (;;) {
    // read the input pin:
  //  if (time >= meassure_interval ){  
       sensorValue = 0;
       sensorValue1 = 0;
    for (int i = 0; i < averageValue; i++) {
       sensorValue += analogRead(A0);
       sensorValue1+= analogRead(A1);
    // wait 2 milliseconds before the next loop
       delay(0.5);
    }
    sensorValue = sensorValue / averageValue;
    current_voltage = sensorValue * 5.0 / 1024.0;
    sample.m_current = (current_voltage - 2.5) / 0.066;  
    sample.m_voltage = 5.0 * sensorValue1 / averageValue / 1024.0; // 
    sample.m_time_interval = time*0.001; // in seconds!!
   // }
  }
}


void cellOff()
{
  return;  
}

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


