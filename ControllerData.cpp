#include <Arduino.h>
#include "ControllerData.h"

void print(ControllerData &ctrlData) {
  Serial.println("CurrentTemp: " + String(ctrlData.currentTemp) + " Heater Threshold: " + String(ctrlData.heaterThreshold) + " Current Gravity: " + String(ctrlData.currentGravity) + " Heater Status: " + String(ctrlData.heaterStatus));
}

void initCtrlData(ControllerData &ctrlData) {
  ctrlData.currentGravity = -1;
  ctrlData.currentTemp = -1;
  ctrlData.heaterStatus = HEATER_OFF;
  ctrlData.heaterThreshold = TEMP_THRESHOLD_DEFAULT;
}

void updateFromAPI(ControllerData &ctrlData, float temp, float gravity) {
  Serial.println("Before updateFromAPI");
  print(ctrlData);

  ctrlData.currentTemp = temp;
  ctrlData.currentGravity = gravity;
  ctrlData.heaterStatus = temp < TEMP_THRESHOLD_DEFAULT;

  Serial.println("After updateFromAPI");
  print(ctrlData);
}
