#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "IPAddress.h"
#include <ArduinoJson.h>

#include "arduino_secrets.h"
#include "HttpServerUtils.h"
#include "ControllerData.h"

#define POWER 7
#define HEATER_ON_LED 0
#define HEATER_OFF_LED 3

// WiFi credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// Server settings
WiFiServer server(80);  // HTTP server on port 80

// API URLs
const char* token_url = "https://id.rapt.io/connect/token";
const char* api_url = "https://api.rapt.io/api/Hydrometers/GetHydrometers";

// polling interval
const int polling_interval = 30 * 60 * 1000;
unsigned long previousMillis = 0;   // Stores the last time SSL request was made

// Global variable to store the bearer token
String bearerToken = "";

// WiFi client for HTTPS
WiFiSSLClient id_client;
WiFiSSLClient api_client;

ControllerData ctrlData;

void setup() {
  delay(1000); // 1 second delay to give time initialization after upload.
  Serial.begin(9600);

  pinMode(HEATER_ON_LED, OUTPUT);
  pinMode(HEATER_OFF_LED, OUTPUT);
  pinMode(POWER, OUTPUT);

  initCtrlData(ctrlData);

  char message[100]; // Create a buffer to hold the resulting string
  sprintf(message, "Polling interval: %d mins.  Heater threshold temp:  %f", polling_interval/1000, ctrlData.heaterThreshold);

  Serial.println(message);

  connectToWiFi();

  // Start the HTTP server
  server.begin();
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.print("Connected to WiFi! ");
  Serial.println(WiFi.localIP());
}

void checkWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.print(".");
        }
        Serial.println("Reconnected to WiFi.");
    }
}


void obtainBearerToken() {
  if (!id_client.connect("id.rapt.io", 443)) {
    Serial.println("Connection to token server failed!");
    return;
  }

  // Build the request body
  String requestBody = "client_id=rapt-user&grant_type=password&username=" API_USER_NAME "&password=" API_SECRET;

  // Send the POST request
  id_client.println("POST /connect/token HTTP/1.1");
  id_client.println("Host: id.rapt.io");
  id_client.println("Content-Type: application/x-www-form-urlencoded");
  id_client.println("Content-Length: " + String(requestBody.length()));
  id_client.println();
  id_client.println(requestBody);

  // Wait for the response
  while (id_client.connected()) {
    String line = id_client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  String jsonBegin = id_client.readStringUntil('{');
  String jsonBody = id_client.readStringUntil('}');

  String tokenJSON = "{" + jsonBody + "}";
  // Parse the JSON response
  DynamicJsonDocument doc(tokenJSON.length());
  deserializeJson(doc, tokenJSON);
  bearerToken = doc["access_token"].as<String>();

  id_client.stop();
}

void refreshDataFromAPI() {
  Serial.println("Connection to API server");

  if (!api_client.connect("api.rapt.io", 443)) {
    Serial.println("Connection to API server failed!");
  }

  // Send the GET request with bearer token
  api_client.println("GET /api/Hydrometers/GetHydrometers HTTP/1.1");
  api_client.println("Host: api.rapt.io");
  api_client.println("Accept: application/json");
  api_client.println("Authorization: Bearer " + bearerToken);
  api_client.println();

  // Wait for the response
  while (api_client.connected()) {
    String line = api_client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  String jsonBegin = api_client.readStringUntil('[');
  String jsonBody = api_client.readStringUntil(']');

  String apiJSON = "[" + jsonBody + "]";

  // Parse the JSON response
  DynamicJsonDocument doc(apiJSON.length());
  deserializeJson(doc, apiJSON);
  updateFromAPI(ctrlData, doc[0]["temperature"].as<float>(), doc[0]["gravity"].as<float>(), doc[0]["battery"].as<float>());

  api_client.stop();
}

void switchPower(bool heaterStatus) {
  if (heaterStatus) {
    heaterOn();
  }
  else {
    heaterOff();
  }
}

void heaterOn() {
  Serial.println("tuon ON heater");
  digitalWrite(POWER, HIGH);
  digitalWrite(HEATER_ON_LED, HIGH);
  digitalWrite(HEATER_OFF_LED, LOW);
}

void heaterOff() {
  Serial.println("tuon OFF heater");
  digitalWrite(POWER, LOW);
  digitalWrite(HEATER_ON_LED, LOW);
  digitalWrite(HEATER_OFF_LED, HIGH);
}

// Function to handle incoming HTTP requests
void handleClient(WiFiClient& client) {
  char requestLine[40];
  size_t lengthToRead1 = sizeof(requestLine) - 1;
  client.readBytesUntil('\r', requestLine, lengthToRead1);

  char skipRest[10];
  size_t lengthToRead2 = sizeof(skipRest) - 1;
  client.readBytesUntil('\r', skipRest, lengthToRead2);


  // Wait until the client sends some data
  while(client.available()) {
    if (strstr(requestLine, "GET /data") != NULL) {
      // Serve the JSON data
      sendJSONData(client, ctrlData);
     } else if (strstr(requestLine, "POST /updateThreshold") != NULL) {
      // Receive and update the heaterThreshold
      updateThreshold(client, ctrlData);
     } else if (strstr(requestLine, "POST /refreshNow") != NULL) {
      setRefreshNow(ctrlData, true); 
      sendEmptyResponse(client, ctrlData);
     } else {
      client.print(getHttpRespHeader());
      client.print(htmlPage);
    }
    client.flush();

    break;
  }

  // Close the connection
  client.stop();
}

void loop() {
  checkWiFi();

  // Check for incoming HTTP clients
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
    switchPower(ctrlData.heaterStatus);
    updateMemorySize(ctrlData);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= polling_interval || ctrlData.refreshNow) {
    Serial.println("Going to server");
    previousMillis = currentMillis;
    setRefreshNow(ctrlData, false);

    // Step 1: Obtain bearer token
    obtainBearerToken();

    // // Step 2: Make API request using the token
    if (bearerToken != "") {
       refreshDataFromAPI();
       switchPower(ctrlData.heaterStatus);
       updateMemorySize(ctrlData);
    }
  }
}
