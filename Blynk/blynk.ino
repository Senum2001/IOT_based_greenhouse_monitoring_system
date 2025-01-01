#define BLYNK_TEMPLATE_ID "" // Replace with your actual Template ID
#define BLYNK_TEMPLATE_NAME ""         // Replace with your actual Template Name
#define BLYNK_AUTH_TOKEN "" 

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi credentials
const char *ssid = "yourssid";
const char *password = "yourpassword";

// Blynk Auth Token
char auth[] = BLYNK_AUTH_TOKEN;

// Weather API URL
const char *weatherApiUrl = "https://api.open-meteo.com/v1/forecast?latitude=6.7990722&longitude=79.8766778&current_weather=true";

// Pin Definitions
#define DHT_PIN 15 // DHT sensor pin
#define DHT_TYPE DHT22
#define RELAY1_PIN 17 // IO17 (Water pump relay)
#define RELAY2_PIN 16 // IO16 (Fan relay)
#define SOIL_MOISTURE_PIN 34 // Analog pin for soil moisture sensor

// Global Objects
DHT dht(DHT_PIN, DHT_TYPE);

// Variables for data
float temperature, humidity, outsideTemperature, windSpeed;
int soilMoisture;
bool relay1State = LOW, relay2State = LOW;

// Function to connect to Wi-Fi
void connectWiFi() {
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi.");
}

// Function to fetch weather data
void fetchWeatherData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(weatherApiUrl);
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200) {
            String response = http.getString();
            Serial.println("Weather Info (Raw JSON):");
            Serial.println(response);

            // Parse the JSON response
            DynamicJsonDocument doc(2048);
            DeserializationError error = deserializeJson(doc, response);

            if (error) {
                Serial.print("JSON Parsing Error: ");
                Serial.println(error.c_str());
                return;
            }

            // Extract values from JSON
            outsideTemperature = doc["current_weather"]["temperature"].as<float>();
            windSpeed = doc["current_weather"]["windspeed"].as<float>();
            String weatherTime = doc["current_weather"]["time"].as<String>();
            int weatherCode = doc["current_weather"]["weathercode"].as<int>();

            // Debugging: Serial print parsed values as strings
            Serial.println("Parsed Weather Data:");
            Serial.print("Temperature: ");
            Serial.println(String(outsideTemperature) + "°C");

            Serial.print("Wind Speed: ");
            Serial.println(String(windSpeed) + " km/h");

            Serial.print("Weather Time: ");
            Serial.println(weatherTime);

            Serial.print("Weather Code: ");
            Serial.println(String(weatherCode));
        } else {
            Serial.print("Error fetching weather data. HTTP Response Code: ");
            Serial.println(httpResponseCode);
        }
        http.end();
    } else {
        Serial.println("Wi-Fi is not connected. Cannot fetch weather data.");
    }
}

// Function to control relays based on button presses in Blynk
BLYNK_WRITE(V2) { // Virtual pin V2 for Relay 1 control
    relay1State = param.asInt(); // Get button state from Blynk app
    digitalWrite(RELAY1_PIN, relay1State);
}

BLYNK_WRITE(V3) { // Virtual pin V3 for Relay 2 control
    relay2State = param.asInt(); // Get button state from Blynk app
    digitalWrite(RELAY2_PIN, relay2State);
}

// Function to send data to Blynk app
void sendSensorData() {
    Blynk.virtualWrite(V0, temperature); // Send temperature to virtual pin V0
    Blynk.virtualWrite(V1, humidity);    // Send humidity to virtual pin V1
    Blynk.virtualWrite(V4, outsideTemperature); // Outside temperature on V4
    Blynk.virtualWrite(V5, windSpeed);   // Wind speed on V5
    Blynk.virtualWrite(V6, soilMoisture); // Soil moisture on V6
}

// Setup function
void setup() {
    Serial.begin(115200);
    dht.begin();
    
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    
    connectWiFi();
    
    Blynk.begin(auth, ssid, password); // Start Blynk connection
}

// Loop function
void loop() {
    Blynk.run(); // Run Blynk tasks

    fetchWeatherData(); // Fetch weather data

    temperature = 30; // Hardcoded temperature
    humidity = 60;    // Hardcoded humidity

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Failed to read from DHT sensor.");
        return;
    }

    soilMoisture = analogRead(SOIL_MOISTURE_PIN); // Read soil moisture sensor value

    sendSensorData(); // Send data to Blynk app

    delay(10000); // Delay before next reading
}
