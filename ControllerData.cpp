#include <Arduino.h>
#include "ControllerData.h"

extern "C" char *sbrk(int incr);

int freeMemory() {
  char stack_pointer;
  return &stack_pointer - sbrk(0);
}

void print(ControllerData &ctrlData) {
  char buffer[128];
  snprintf(buffer, sizeof(buffer),
    "CurrentTemp: %.2f Heater Threshold: %.2f Current Gravity: %.2f Heater Status: %i Battery: %.2f  Memory: %i RefreshNow: %i",
    ctrlData.currentTemp,
    ctrlData.heaterThreshold,
    ctrlData.currentGravity,
    ctrlData.heaterStatus,
    ctrlData.battery,
    ctrlData.memory,
    ctrlData.refreshNow);
  Serial.println(buffer);
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

void setRefreshNow(ControllerData &ctrlData, bool flag) {
  ctrlData.refreshNow = flag;
}

// void printMemorySize(String message) {
//   int memorySize = freeMemory();
//   Serial.println(message + " " + String(memorySize));
// }
