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

// API credentials
const char* client_id = "rapt-user";
const char* grant_type = "password";
const char* api_username = API_USER_NAME; // replace with actual username
const char* api_password = API_SECRET; // replace with actual password

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
  Serial.println("Polling interval: " + String(polling_interval) + "ms. Heater threshold temp: " + String(ctrlData.heaterThreshold));

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
  String requestBody = "client_id=" + String(client_id) +
                       "&grant_type=" + String(grant_type) +
                       "&username=" + String(api_username) +
                       "&password=" + String(api_password);

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
   DynamicJsonDocument doc(1024);
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
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, apiJSON);

  updateFromAPI(ctrlData, doc[0]["temperature"].as<float>(), doc[0]["gravity"].as<float>());

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
void handleClient(WiFiClient client) {
  Serial.println("New client connected");
  // Wait until the client sends some data
  while(client.available()) {
    // Serial.println("Request all: " + client.readString());
    String requestHeader = client.readStringUntil('\r');
    String requestBody = client.readString();
    Serial.println("Request: " + requestHeader);
    Serial.println("request body: " + requestBody);
    client.flush();
    
    if (requestHeader.indexOf("GET /index") != -1) {
      Serial.println("/index");
      client.print(getHttpRespHeader());
      client.print(getHTMLPage(WiFi.localIP().toString()));
    } else if (requestHeader.indexOf("GET /data") != -1) {
      Serial.println("/data");
      // Serve the JSON data
      sendJSONData(client, ctrlData);
    } else if (requestHeader.indexOf("POST /updateThreshold") != -1) {
      Serial.println("/updateThreshold");
      // Receive and update the heaterThreshold
      updateThreshold(client, requestBody, ctrlData);
    }
    break;
  }

  // Close the connection
  client.stop();
  Serial.println("Client disconnected");
}

void loop() {
  checkWiFi();
  
  // Check for incoming HTTP clients
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  } 

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= polling_interval || previousMillis == 0) {
    previousMillis = currentMillis;
    // Step 1: Obtain bearer token
    obtainBearerToken();

    // Step 2: Make API request using the token
    if (bearerToken != "") {
      refreshDataFromAPI();
      switchPower(ctrlData.heaterStatus);
    }
  }
}
