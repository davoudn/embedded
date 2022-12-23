// add the FreeRTOS functions for Semaphores (or Flags).
#include "proc_handle.h"
ProcHandle* ph;
void setup() {
  Serial.begin(9600);
  while (!Serial) {;}
  ph = new ProcHandle();
  ph->run();
}

void loop(){}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/


