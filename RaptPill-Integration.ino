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

void setup() {
  Serial.begin(115200);
  pinMode(HEATER_ON_LED, OUTPUT);
  pinMode(HEATER_OFF_LED, OUTPUT);
  pinMode(POWER, OUTPUT);

  Serial.println("Polling interval: " + String(polling_interval) + "ms. Heater threshold temp: " + String(TEMP_THRESHOLD));
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
  if (temp < TEMP_THRESHOLD) {
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
  while (client.available()) {
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();
    
    // Respond to the client (basic HTML response)
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();
    client.println("<html><body><h1>Settings page</h1></body></html>");
    client.println();
    break;
  }

  // Close the connection
  client.stop();
  Serial.println("Client disconnected");
}

void loop() {
    // Check for incoming HTTP clients
  WiFiClient client = server.available();
  if (client) {
    handleClient(client);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= polling_interval) {
    previousMillis = currentMillis;
    // Step 1: Obtain bearer token
    obtainBearerToken();

    // Step 2: Make API request using the token
    if (bearerToken != "") {
      float temp = getTempAPI();
      switchPower(temp);
    }
  }
}
