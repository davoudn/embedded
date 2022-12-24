#include "ArduinoJson.h"
#include <ArxContainer.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>  
#include <Wire.h>

void write_to_dac(int dac_id, float value)
{
  Wire.begin();
  int dummy = (value/10.0)*4095;
  byte myBytes[2];
  myBytes[0] = dummy/256;
  myBytes[1] = dummy%256; 
  Wire.beginTransmission(dac_id);
  Wire.write(myBytes[0]);  
  Wire.write(myBytes[1]);            // sends instruction byte  
  Wire.endTransmission();    // stop transmitting
    return;
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

struct Sample{
  float m_voltage;
  float m_current;
  float m_time_interval;
};

struct Flags{
   Flags():m_taskStarted(0),m_taskFinished(0){}
   Flags(int m1, int m2):m_taskStarted(m1),m_taskFinished(m2){}
   void reset()
   {
      m_taskStarted=0;
      m_taskFinished=0;
      return;
   }
  int m_taskStarted;
  int m_taskFinished;
};

struct BaseProc{
  BaseProc ( StaticJsonDocument<120> j_instruction_):j_instruction(j_instruction_){}
  BaseProc () {}
   StaticJsonDocument<120> j_status;
   StaticJsonDocument<120> j_instruction;
   Sample m_sample;
   Flags  m_flags;
   int sgn_appliedField, sgn_cutoffField;     
   virtual void apply(){;}
   virtual void check_it(){;}
   virtual void cellOff(){;}
   virtual void cellOn(){;}
   void setInstruction( StaticJsonDocument<120> j_instruction_)
   {
     j_instruction=j_instruction_;
       float tmp1 =  j_instruction["applied_field"];     
       float tmp2 =  j_instruction["field_cutoff"];
     sgn_appliedField = sgn(tmp1);
     sgn_cutoffField  = sgn(tmp2);     
     return;
   }
 };

//
struct cv{};
struct cc{};
template <typename PROCTYPE>
struct Procedure;

