#ifndef HTTPSERVERUTILS_H
#define HTTPSERVERUTILS_H

#include "ControllerData.h"

const char* getHttpRespHeader();
extern const char* htmlPage;

void sendJSONData(WiFiClient& client, ControllerData& ctrlData);
void updateThreshold(WiFiClient& client, ControllerData& ctrlData);
void sendEmptyResponse(WiFiClient& client, ControllerData& ctrlData);
bool seek(WiFiClient& client, char* seekString);
#endif
