#include <Arduino.h>
#include "ControllerData.h"

extern "C" char *sbrk(int incr);

int freeMemory() {
  char stack_pointer;
  return &stack_pointer - sbrk(0);
}

void print(ControllerData &ctrlData) {
  Serial.println("CurrentTemp: " + String(ctrlData.currentTemp) + 
  " Heater Threshold: " + String(ctrlData.heaterThreshold) + 
  " Current Gravity: " + String(ctrlData.currentGravity) + 
  " Heater Status: " + String(ctrlData.heaterStatus) +
  " Battery: " + String(ctrlData.battery) + 
  " Memory: " + String(ctrlData.memory)
  );
}

void initCtrlData(ControllerData &ctrlData) {
  ctrlData.currentGravity = -1;
  ctrlData.currentTemp = -1;
  ctrlData.heaterStatus = HEATER_OFF;
  ctrlData.heaterThreshold = TEMP_THRESHOLD_DEFAULT;
  ctrlData.battery = -1;
  ctrlData.memory = -1;
}

void updateFromAPI(ControllerData &ctrlData, float temp, float gravity, float battery) {
  Serial.println("Before updateFromAPI");
  print(ctrlData);

  ctrlData.currentTemp = temp;
  ctrlData.currentGravity = gravity/1000;
  ctrlData.heaterStatus = temp < ctrlData.heaterThreshold;
  ctrlData.battery = battery;

  Serial.println("After updateFromAPI");
  print(ctrlData);
}

void updateHeaterThreshold(ControllerData &ctrlData, float heaterThreashold) {
  Serial.println("Before updateHeaterThreshold");
  print(ctrlData);
  ctrlData.heaterThreshold = heaterThreashold;
  ctrlData.heaterStatus = ctrlData.currentTemp < ctrlData.heaterThreshold;
  Serial.println("After updateHeaterThreshold");
  print(ctrlData);
}

void updateMemorySize(ControllerData &ctrlData) {
  ctrlData.memory = freeMemory();
  print(ctrlData);
}
