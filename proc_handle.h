#include "proc_types.h"

#include "base.h"

template <class PROCHANDLE>
struct tmp{
static void taskControl( PROCHANDLE* prochandle ) 
{  
  for (;;){
    xSemaphoreGive( prochandle->xSerialSemaphore );    
    if (  prochandle->command_arrived==1  && prochandle->no_of_procs==0 ){  
        Serial.print("before cv cv.\n");
        if ( prochandle->j_ins_arrived["c2"] == "cv" ){
          
         //prochandle->proc = new  Procedure<cv> (prochandle->j_ins_arrived);
          prochandle->no_of_procs+=1;         
          prochandle->setInstruction();
          Serial.print(prochandle->no_of_procs);//prochandle->no_of_procs);
          xTaskCreate(Base<PROCHANDLE>::taskApplyCheck   , "Runing jobs",  256,  prochandle , 3,   &prochandle->taskApplycheckHandle );
         //  prochandle->command_arrived = 0;
       	}
        prochandle->command_arrived = 0;
    }
    else {    
      if(  prochandle->command_arrived==1 && prochandle->no_of_procs==1 ){
            prochandle->command_arrived=0;
           //Serial.print("A task is running, first cancel the current task!!!");        
            if ( prochandle->j_ins_arrived["c2"] == "cancel") {
               Serial.print("The task was clanceled by user!");
            //   prochandle->proc->cellOff();
              // prochandle->removeTasks();
            } 
            else{
              Serial.print("A task is running, first cancel the current task!!!");
            }            
      }
    } 
  // check if task is finished, then clear the task.
    if (prochandle->proc->m_flags.m_taskStarted && prochandle->proc->m_flags.m_taskFinished){
        prochandle->removeTasks();
        prochandle->sendMessageFinished();
    }
  }

}
};
//TaskHandle_t taskRunHandle; 
struct ProcHandle {
   StaticJsonDocument<120> j_instruction, j_data, j_message,j_ins_arrived;
   BaseProc* proc;
   SemaphoreHandle_t xSerialSemaphore;
   TaskHandle_t  taskApplycheckHandle;
   int command_arrived;
   int no_of_procs;
  //ProcHandle (StaticJsonDocument<120> j_instruction_):j_instruction(j_instruction_), m_flags() {}
    ProcHandle ():no_of_procs(0),command_arrived(0), proc (new Procedure<cv>())
    {
       if ( xSerialSemaphore == NULL ){
          xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
          if ( ( xSerialSemaphore ) != NULL )
          xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
       }

    }
   ~ProcHandle (){}

  void run(){
            xTaskCreate( Base<ProcHandle>::taskListen   ,  "Listen",  256,  this,  3,  NULL ); //Task Handle 
            xTaskCreate( tmp<ProcHandle>::taskControl  ,  "Control",  256,  this,  3,  NULL ); //Task Handle
        
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
         j_message["msg"]  = "jobdone"   ;  
         serializeJson(j_message, Serial);    
         Serial.print("#\n");    
                                                           //                         xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for other                                                      //   }            
    }
    void setInstruction()
    {
      proc->setInstruction(j_ins_arrived);
    }
    void removeTasks(){
       vTaskDelete(taskApplycheckHandle);
       no_of_procs--;
       delete proc;
       taskApplycheckHandle = NULL;
       Serial.println("Everything were cleared! :-)!");
    }
  };
