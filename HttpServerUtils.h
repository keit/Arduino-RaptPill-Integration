#ifndef HTTPSERVERUTILS_H
#define HTTPSERVERUTILS_H

#include "ControllerData.h"

const char* getHttpRespHeader();
const char* getHTMLPage(String ipAddress);

void sendJSONData(WiFiClient& client, ControllerData& ctrlData);
void updateThreshold(WiFiClient& client, String request, ControllerData& ctrlData);

void parseHTTPHeaders(WiFiClient& client);
void addHeader(String key, String value);
void clearHeaders();
String getHeaderValue(String key);
#endif
