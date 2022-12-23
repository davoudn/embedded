#include "ArduinoJson.h"
#include <ArxContainer.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>  
#include <Wire.h>

void write_to_dac(int dac_id, float value)
{
  // transmit to device #4
  int dummy = (value/10.0)*4095;
  byte myBytes[2];
  myBytes[0] = dummy/256;
  myBytes[1] = dummy%256; 
  Wire.beginTransmission(96);
  Wire.write(myBytes[0]);  
  Wire.write(myBytes[1]);            // sends instruction byte  
  Wire.endTransmission();    // stop transmitting
    return;
}


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

struct BaseProc{
  BaseProc( StaticJsonDocument<120> j_instruction_):j_instruction(j_instruction_){}
   StaticJsonDocument<120> j_status;
   StaticJsonDocument<120> j_instruction;
   Sample m_sample;
   Flags  m_flags;
   virtual void apply(){;}
   virtual void check_it(){;}
   virtual void cellOff(){;}
 };

//
struct cv{};
struct cc{};
template <typename PROCTYPE>
struct Procedure;

