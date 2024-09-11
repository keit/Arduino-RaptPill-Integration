#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include "IPAddress.h"
#include <ArduinoJson.h>

#include "arduino_secrets.h"

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

// Global variable to store the bearer token
String bearerToken = "";

// WiFi client for HTTPS
WiFiSSLClient client;

void setup() {
  Serial.begin(115200);
  connectToWiFi();

  // Step 1: Obtain bearer token
  obtainBearerToken();

  // Step 2: Make API request using the token
  if (bearerToken != "") {
    apiRequest();
  }
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
  if (!client.connect("id.rapt.io", 443)) {
    Serial.println("Connection to token server failed!");
    return;
  }

  // Build the request body
  String requestBody = "client_id=" + String(client_id) +
                       "&grant_type=" + String(grant_type) +
                       "&username=" + String(api_username) +
                       "&password=" + String(api_password);

  // Send the POST request
  client.println("POST /connect/token HTTP/1.1");
  client.println("Host: id.rapt.io");
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.println("Content-Length: " + String(requestBody.length()));
  client.println();
  client.println(requestBody);

  // Wait for the response
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  // Get the response body
  String response = client.readString();
  Serial.println("Token Response: " + response);

  // Parse the JSON response
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, response);
  bearerToken = doc["access_token"].as<String>();
  Serial.println("Bearer Token: " + bearerToken);
}

void apiRequest() {
  if (!client.connect("api.rapt.io", 443)) {
    Serial.println("Connection to API server failed!");
    return;
  }

  // Send the GET request with bearer token
  client.println("GET /api/Hydrometers/GetHydrometers HTTP/1.1");
  client.println("Host: api.rapt.io");
  client.println("Accept: application/json");
  client.println("Authorization: Bearer " + bearerToken);
  client.println();

  // Wait for the response
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }

  // Get the response body
  String response = client.readString();
  Serial.println("API Response: " + response);
}

void loop() {
  // No need for loop in this case
}
