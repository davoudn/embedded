#include "proc_base.h"

template <> struct Procedure<cv>:public BaseProc {
  Procedure( StaticJsonDocument<120> j_instruction_): BaseProc( j_instruction_) {}
  Procedure(): BaseProc() {}
  void check_it(){ 
          float dummy = j_instruction["field_cutoff"];
          if (m_sample.m_current * sgn_appliedField < dummy * sgn_appliedField) {
                m_flags.m_taskFinished = 1;
                cellOff();
          }
          return;    
  }

  void apply (){
         cellOn();     
         float applied_voltage = j_instruction["applied_field"];
         write_to_dac(96, applied_voltage);
         pinMode(50, OUTPUT);
         digitalWrite(50, LOW);      
         m_flags.m_taskStarted = 1;
        return;
   }
  };

template <> struct Procedure<cc>:public BaseProc {
  Procedure( StaticJsonDocument<120> j_instruction_): BaseProc( j_instruction_) {}
  Procedure(): BaseProc() {}

  void check_it(){ 
          float dummy = j_instruction["field_cutoff"];
          if (m_sample.m_voltage * sgn_appliedField   > dummy * sgn_appliedField ) {
                m_flags.m_taskFinished = 1;
                cellOff();
          }
          return;    
  }

  void apply (){
         cellOn();   
         float applied_voltage = j_instruction["applied_field"];
         write_to_dac(96, applied_voltage);
         pinMode(50, OUTPUT);
         digitalWrite(50, HIGH);      
         m_flags.m_taskStarted = 1;
        return;
   }
  };

