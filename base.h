#include <semphr.h>  // add the FreeRTOS functions for Semaphores (or Flag
#include "ArduinoJson.h"
/* ----------------------------      Static functions      --------------------------------- */
template <class PROCHANDLE>
 struct Base {
   static void taskApplyCheck(PROCHANDLE* prochandle){
        prochandle->proc->apply();
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

/*     ---------------------------------------------------------------------------- */
static void taskListen( PROCHANDLE* prochandle )  // This is a Task.
{  
  String json_string_instruction = "", str = ""; 
  for (;;){
     //if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 1 ) == pdTRUE ){
     while (Serial.available() == 0) {}     //wait for data available
      json_string_instruction = Serial.readString();  //read until timeout
    //  xSemaphoreGive( xSerialSemaphore );  
      Serial.print(json_string_instruction); 
      str = json_string_instruction; 
      deserializeJson(prochandle->j_ins_arrived, str);
      prochandle->command_arrived = 1;
  }   
}

/*     ---------------------------------------------------------------------------- */
};

