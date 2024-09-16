#ifndef CONTROLLERDATA_H
#define CONTROLLERDATA_H

#define HEATER_OFF false
#define HEATER_ON true
#define TEMP_THRESHOLD_DEFAULT 18.0

struct ControllerData {
  float currentTemp;
  float currentGravity;
  bool heaterStatus;
  float heaterThreshold;
};

void initCtrlData(ControllerData &ctrlData);
void updateFromAPI(ControllerData &ctrlData, float temp, float gravity);
void updateHeaterThreshold(ControllerData &ctrlData, float heaterThreashold);
#endif