#include "WiFiS3.h"
#include "WiFiSSLClient.h"
#include <ArduinoJson.h>
#include "HttpServerUtils.h"


const int MAX_HEADERS = 10;  // Maximum number of headers to store
String headerKeys[MAX_HEADERS];  // Array for header keys
String headerValues[MAX_HEADERS];  // Array for header values
int headerCount = 0;  // Keep track of the number of headers

const char* getHttpRespHeader() {
  // Add "\r\n\r\n" to ensure proper header-body separation
  return "HTTP/1.1 200 OK\r\n"
         "Content-Type: text/html\r\n"
         "\r\n";  // This marks the end of headers
}

const char* getHTMLPage(String ipAddress) {
  const char* page = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="icon" href="data:," />
    <title>Heater Threshold Control</title>
    <style>
        table {
            width: 50%;
            margin: 20px auto;
            border-collapse: collapse;
        }
        table, th, td {
            border: 1px solid black;
        }
        th, td {
            padding: 10px;
            text-align: center;
        }
        form {
            text-align: center;
            margin-top: 20px;
        }
    </style>
</head>
<body>

    <h2 style="text-align:center;">Heater Control System</h2>

    <table>
        <tr>
            <th>Data</th>
            <th>Value</th>
        </tr>
        <tr>
            <td>Temperature</td>
            <td id="currentTemp">Loading...</td>
        </tr>
        <tr>
            <td>Gravity</td>
            <td id="currentGravity">Loading...</td>
        </tr>
        <tr>
            <td>Heater Status</td>
            <td id="heaterStatus">Loading...</td>
        </tr>
        <tr>
            <td>Heater Threshold</td>
            <td id="heaterThreshold">Loading...</td>
        </tr>
    </table>

    <form id="updateForm">
        <label for="newThreshold">Update Heater Threshold: </label>
        <input type="number" id="newThreshold" name="newThreshold" min="0" step="0.1" required> °C
        <button type="submit">Update</button>
    </form>

    <p id="message" style="text-align:center; color:green;"></p>

    <script>
        // Fetch data from the Arduino server
        function fetchData() {
            fetch('http://<arduino_ip>/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('currentTemp').textContent = data.currentTemp + '°C';
                    document.getElementById('currentGravity').textContent = data.currentGravity.toFixed(3);
                    document.getElementById('heaterStatus').textContent = data.heaterStatus;
                    document.getElementById('heaterThreshold').textContent = data.heaterThreshold + '°C';
                })
                .catch(error => {
                    console.error('Error fetching data:', error);
                });
        }

        // Call fetchData on page load
        fetchData();

        // Handle form submission to update heaterThreshold
        document.getElementById('updateForm').addEventListener('submit', function(event) {
            event.preventDefault();  // Prevent form submission

            // Get the new threshold value
            const newThreshold = document.getElementById('newThreshold').value;

            // Send the updated threshold to the Arduino server
            fetch('http://<arduino_ip>/updateThreshold', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/x-www-form-urlencoded',
                },
                body: 'heaterThreshold=' + encodeURIComponent(newThreshold)
            })
            .then(response => response.text())
            .then(data => {
                document.getElementById('message').textContent = data;
                // Update the displayed table after submission
                fetchData();
            })
            .catch(error => {
                console.error('Error updating threshold:', error);
            });
        });
    </script>

</body>
</html>
)";
  String pageString = String(page);
  pageString.replace(String("<arduino_ip>"), ipAddress);
  return pageString.c_str();
}

void sendJSONData(WiFiClient& client, ControllerData& ctrlData) {
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["currentTemp"] = ctrlData.currentTemp;
    jsonDoc["currentGravity"] = ctrlData.currentGravity;
    jsonDoc["heaterStatus"] = ctrlData.heaterStatus;
    jsonDoc["heaterThreshold"] = ctrlData.heaterThreshold;

    String jsonResponse;
    serializeJson(jsonDoc, jsonResponse);

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(jsonResponse);
}

void updateThreshold(WiFiClient& client, String request, ControllerData& ctrlData) {
    // Parse the incoming request for heaterThreshold

    if (request.indexOf("heaterThreshold=") != -1) {
        int pos = request.indexOf("heaterThreshold=") + strlen("heaterThreshold=");
        String thresholdValue = request.substring(pos);

        updateHeaterThreshold(ctrlData, thresholdValue.toFloat());

        // Respond with success message
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Threshold updated to: " + String(ctrlData.heaterThreshold));
    } 
}

void parseHTTPHeaders(WiFiClient& client) {
  while (client.available()) {
    String headerLine = client.readStringUntil('\r');
    headerLine.trim();  // Trim newline characters
    if (headerLine.length() == 0) {
        return;  // Blank line indicates the end of headers
    }

    // Parse the header line into key and value
    int separatorIndex = headerLine.indexOf(':');
    if (separatorIndex > 0 && headerCount < MAX_HEADERS) {
        String key = headerLine.substring(0, separatorIndex);
        String value = headerLine.substring(separatorIndex + 2);  // Skip ": " part
        addHeader(key, value);  // Store the header key-value pair
    }
  }
}

// Function to store header key-value pair in the arrays
void addHeader(String key, String value) {
    if (headerCount < MAX_HEADERS) {
        headerKeys[headerCount] = key;
        headerValues[headerCount] = value;
        headerCount++;
    } else {
        Serial.println("Header storage limit reached.");
    }
}

// Function to clear stored headers
void clearHeaders() {
    for (int i = 0; i < MAX_HEADERS; i++) {
        headerKeys[i] = "";
        headerValues[i] = "";
    }
    headerCount = 0;
}

// Function to get a header value by key
String getHeaderValue(String key) {
    for (int i = 0; i < headerCount; i++) {
        if (headerKeys[i] == key) {
            return headerValues[i];
        }
    }
    return "";
}
