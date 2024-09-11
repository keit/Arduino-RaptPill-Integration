#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "IPAddress.h"
#include <ArduinoJson.h>

#include "arduino_secrets.h"

#define TEMP_THRESHOLD 18.0
#define POWER 7
#define HEATER_ON_LED 0
#define HEATER_OFF_LED 3

// WiFi credentials
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

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

// Global variable to store the bearer token
String bearerToken = "";

// WiFi client for HTTPS
WiFiSSLClient id_client;
WiFiSSLClient api_client;

void setup() {
  Serial.begin(115200);
  pinMode(HEATER_ON_LED, OUTPUT);
  pinMode(HEATER_OFF_LED, OUTPUT);
  pinMode(POWER, OUTPUT);

  connectToWiFi();
}

void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected to WiFi!");
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

  Serial.println("Token Response: " + tokenJSON);

  // Parse the JSON response
   DynamicJsonDocument doc(1024);
   deserializeJson(doc, tokenJSON);
   bearerToken = doc["access_token"].as<String>();
   Serial.println("Bearer Token: " + bearerToken);

   id_client.stop();
}

float getTempAPI() {
  Serial.println("Connection to API server");

  if (!api_client.connect("api.rapt.io", 443)) {
    Serial.println("Connection to API server failed!");
    return 0.0;
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

  Serial.println("API Response: " + apiJSON);

  // Parse the JSON response
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, apiJSON);

  String temp = doc[0]["temperature"].as<String>();
  String gravity = doc[0]["gravity"].as<String>();
  Serial.println("temperature: " + temp);
  Serial.println("gravity: " + gravity);

  api_client.stop();

  return temp.toFloat();
}

void switchPower(float temp) {
  Serial.println("Current temp: " + String(temp, 2) + " Threshold: " + TEMP_THRESHOLD);
  if (temp < TEMP_THRESHOLD) {
    Serial.println("tuon ON heater");
    digitalWrite(POWER, HIGH);
    digitalWrite(HEATER_ON_LED, HIGH);
    digitalWrite(HEATER_OFF_LED, LOW);
  }
  else {
    Serial.println("tuon OFF heater");
    digitalWrite(POWER, LOW);
    digitalWrite(HEATER_ON_LED, LOW);
    digitalWrite(HEATER_OFF_LED, HIGH);
  }
}

void loop() {
  // Step 1: Obtain bearer token
  obtainBearerToken();


  // Step 2: Make API request using the token
  if (bearerToken != "") {
    float temp = getTempAPI();
    switchPower(temp);
  }


  delay(polling_interval);
  // No need for loop in this case
}
