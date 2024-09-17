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
    <script src="https://cdn.tailwindcss.com"></script>
</head>
<body class="">
    <div class="px-4 sm:px-6 md:px-10 lg:px-32">
        <h2 class="text-center">Heater Control System</h2>

        <div class="relative overflow-x-auto">
            <table class="min-w-full text-sm text-left text-gray-500 dark:text-gray-400">
                <thead class="text-xs text-gray-700 uppercase bg-gray-50 dark:bg-gray-700 dark:text-gray-400">
                    <tr>
                        <th scope="col" class="px-2 py-3 w-1/2">
                            Data
                        </th>
                        <th scope="col" class="px-2 py-3">
                            Value
                        </th>
                    </tr>
                </thead>
                <tbody>
                    <tr class="odd:bg-white odd:dark:bg-gray-900 even:bg-gray-50 even:dark:bg-gray-800 border-b dark:border-gray-700">
                        <th scope="row" class="px-2 py-4 font-medium text-gray-900 whitespace-nowrap dark:text-white">
                            Temperature
                        </th>
                        <td id="currentTemp" class="px-2 py-4">Loading...</td>
                    </tr>
                    <tr class="odd:bg-white odd:dark:bg-gray-900 even:bg-gray-50 even:dark:bg-gray-800 border-b dark:border-gray-700">
                        <th scope="row" class="px-2 py-4 font-medium text-gray-900 whitespace-nowrap dark:text-white">
                            Gravity
                        </th>
                        <td id="currentGravity" class="px-2 py-4">
                            Loading...
                        </td>
                    </tr>
                    <tr class="odd:bg-white odd:dark:bg-gray-900 even:bg-gray-50 even:dark:bg-gray-800 border-b dark:border-gray-700">
                        <th scope="row" class="px-2 py-4 font-medium text-gray-900 whitespace-nowrap dark:text-white">
                            Heater Status
                        </th>
                        <td id="heaterStatus" class="px-2 py-4">
                            Loading...
                        </td>
                    </tr>
                    <tr class="odd:bg-white odd:dark:bg-gray-900 even:bg-gray-50 even:dark:bg-gray-800 border-b dark:border-gray-700">
                        <th scope="row" class="px-2 py-4 font-medium text-gray-900 whitespace-nowrap dark:text-white">
                            Heater Threshold
                        </th>
                        <td id="heaterThreshold" class="px-2 py-4">
                            Loading...
                        </td>
                    </tr>
                    <tr class="odd:bg-white odd:dark:bg-gray-900 even:bg-gray-50 even:dark:bg-gray-800 border-b dark:border-gray-700">
                        <th scope="row" class="px-2 py-4 font-medium text-gray-900 whitespace-nowrap dark:text-white">
                            Battery
                        </th>
                        <td id="battery" class="px-2 py-4">
                            Loading...
                        </td>
                    </tr>
                </tbody>
            </table>

            <!-- Form -->
            <form id="updateForm" class="p">
                <div class="flex flex-col items-center mt-2 space-y-2 sm:flex-row sm:space-y-0 sm:space-x-4">
                    <label for="newThreshold" class="sm:mr-4">Update Heater Threshold</label>
                    <input type="number" id="newThreshold" name="newThreshold" class="text-blue-500 placeholder-gray-400 p-2 border border-gray-300 rounded" min="0" step="0.1" required>
                    <span>°C</span>
                    <button type="submit" class="mt-2 sm:mt-0 focus:outline-none text-white bg-green-700 hover:bg-green-800 focus:ring-4 focus:ring-green-300 font-medium rounded-lg text-sm px-5 py-2.5 dark:bg-green-600 dark:hover:bg-green-700 dark:focus:ring-green-800">Update</button>
                </div>
            </form>
            <p id="message" class="text-center text-green-500 mt-4"></p>
        </div>
    </div>

    <script>
        // Fetch data from the Arduino server
        function fetchData() {
            fetch('http://<arduino_ip>/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('currentTemp').textContent = data.currentTemp.toFixed(2) + '°C';
                    document.getElementById('currentGravity').textContent = data.currentGravity.toFixed(3);
                    document.getElementById('heaterStatus').textContent = data.heaterStatus ? '☀️ On' : '❄️ Off';
                    document.getElementById('heaterThreshold').textContent = data.heaterThreshold.toFixed(2) + '°C';
                    document.getElementById('battery').textContent = data.battery.toFixed(0) + '%';
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
                document.getElementById('newThreshold').value = '';
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
    jsonDoc["battery"] = ctrlData.battery;

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
