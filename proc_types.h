#include "proc_base.h"

template <> struct Procedure<cv>:public BaseProc {
  Procedure( StaticJsonDocument<120> j_instruction_): BaseProc( j_instruction_) {}

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

template <> struct Procedure<cc>:public BaseProc {
  Procedure( StaticJsonDocument<120> j_instruction_): BaseProc( j_instruction_) {}

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

