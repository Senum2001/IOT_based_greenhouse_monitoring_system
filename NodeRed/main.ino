#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Wi-Fi credentials
const char *ssid = "yourssid";
const char *password = "yourpassword";

// MQTT broker details
const char *mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char *mqttTopicTemperature = "temperatureTSN";
const char *mqttTopicHumidity = "humidityTSN"; 
const char *mqttTopicApiTemperature = "apiTemperatureTSN";
const char *mqttTopicWindSpeed = "windSpeedTSN";
const char *mqttTopicSoilMoisture = "soilMoistureTSN";
const char *mqttTopicRelay1 = "relay1TSN"; 
const char *mqttTopicRelay2 = "relay2TSN"; 

// Weather API URL
const char *weatherApiUrl = "https://api.open-meteo.com/v1/forecast?latitude=6.7990722&longitude=79.8766778&current_weather=true";

// Pin Definitions
#define DHT_PIN 15 // DHT sensor pin
#define DHT_TYPE DHT22
#define RELAY1_PIN 17 // IO17 (Water pump relay)
#define RELAY2_PIN 16 // IO16 (Fan relay)
#define BUTTON1_PIN 41 // IO41 (Button for Relay 1)
#define BUTTON2_PIN 42 // IO42 (Button for Relay 2)
#define SOIL_MOISTURE_PIN 34 // Analog pin for soil moisture sensor
#define LED_PIN 2 // System status LED
#define SOIL_LED1_PIN 6  // Soil moisture indicator LED 1
#define SOIL_LED2_PIN 7  // Soil moisture indicator LED 2
#define SOIL_LED3_PIN 8  // Soil moisture indicator LED 3
#define MQTT_LED_PIN 9   // MQTT connectivity indicator LED

// Thresholds
const int soilMoistureThresholdLow = 300;  // Dry threshold
const int soilMoistureThresholdMedium = 500;  // Medium threshold
const int soilMoistureThresholdHigh = 700;  // High moisture threshold
const float temperatureThreshold = 30.0; // Fan turns on above this temperature

// Global Objects
WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHT_PIN, DHT_TYPE);

// Variables for data
float temperature, humidity, outsideTemperature, windSpeed;
int soilMoisture;
bool relay1State = LOW, relay2State = LOW;
bool button1LastState = HIGH, button2LastState = HIGH;

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

// Function to connect to MQTT broker
void connectMQTT() {
    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");
        if (client.connect("ESP32Client")) {
            Serial.println("Connected to MQTT.");
            digitalWrite(MQTT_LED_PIN, HIGH); // Turn on MQTT LED
            // Subscribe to relay topics
            client.subscribe(mqttTopicRelay1);
            client.subscribe(mqttTopicRelay2);
        } else {
            Serial.println("Failed MQTT connection, retrying in 5 seconds...");
            digitalWrite(MQTT_LED_PIN, LOW); // Turn off MQTT LED
            delay(5000);
        }
    }
}

// Callback function to handle incoming MQTT messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Message received on topic ");
    Serial.print(topic);
    Serial.print(": ");
    Serial.println(message);

    // Check which topic the message belongs to
    if (String(topic) == mqttTopicRelay1) {
        if (message == "ON") {
            relay1State = HIGH;
        } else if (message == "OFF") {
            relay1State = LOW;
        }
        digitalWrite(RELAY1_PIN, relay1State);
        Serial.print("Relay 1 is now ");
        Serial.println(relay1State ? "ON" : "OFF");
    } else if (String(topic) == mqttTopicRelay2) {
        if (message == "ON") {
            relay2State = HIGH;
        } else if (message == "OFF") {
            relay2State = LOW;
        }
        digitalWrite(RELAY2_PIN, relay2State);
        Serial.print("Relay 2 is now ");
        Serial.println(relay2State ? "ON" : "OFF");
    }
}

// Function to fetch weather data
void fetchWeatherData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(weatherApiUrl);
        int httpResponseCode = http.GET();

        if (httpResponseCode == 200) {
            String response = http.getString();
            Serial.println("Weather Info:");
            Serial.println(response);

            // Parse the JSON response
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, response);

            outsideTemperature = doc["current_weather"]["temperature"];
            windSpeed = doc["current_weather"]["windspeed"];
        } else {
            Serial.println("Error fetching weather data.");
        }
        http.end();
    }
}

// Function to control LEDs for soil moisture levels
void updateSoilMoistureLEDs() {
    if (soilMoisture < soilMoistureThresholdLow) {
        digitalWrite(SOIL_LED1_PIN, HIGH);
        digitalWrite(SOIL_LED2_PIN, LOW);
        digitalWrite(SOIL_LED3_PIN, LOW);
    } else if (soilMoisture < soilMoistureThresholdMedium) {
        digitalWrite(SOIL_LED1_PIN, LOW);
        digitalWrite(SOIL_LED2_PIN, HIGH);
        digitalWrite(SOIL_LED3_PIN, LOW);
    } else {
        digitalWrite(SOIL_LED1_PIN, LOW);
        digitalWrite(SOIL_LED2_PIN, LOW);
        digitalWrite(SOIL_LED3_PIN, HIGH);
    }
}

// Function to control relays automatically based on conditions
void automaticControl() {
    // Soil moisture-based watering system
    if (soilMoisture < soilMoistureThresholdLow) {
        relay1State = HIGH;
    } else {
        relay1State = LOW;
    }
    digitalWrite(RELAY1_PIN, relay1State);

    // Temperature-based fan control
    if (temperature > temperatureThreshold) {
        relay2State = HIGH;
    } else {
        relay2State = LOW;
    }
    digitalWrite(RELAY2_PIN, relay2State);
}

// Setup function
void setup() {
    Serial.begin(115200);
    dht.begin();

    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(SOIL_LED1_PIN, OUTPUT);
    pinMode(SOIL_LED2_PIN, OUTPUT);
    pinMode(SOIL_LED3_PIN, OUTPUT);
    pinMode(MQTT_LED_PIN, OUTPUT);

    digitalWrite(LED_PIN, HIGH); // Turn on system status LED

    connectWiFi();
    client.setServer(mqttServer, mqttPort);
    client.setCallback(mqttCallback);
}

// Loop function
void loop() {
    if (!client.connected()) {
        connectMQTT();
    }

    client.loop();

    // Read DHT sensor data
    temperature = dht.readTemperature();
    humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Failed to read from DHT sensor.");
        return;
    }

    // Read soil moisture sensor value
    soilMoisture = analogRead(SOIL_MOISTURE_PIN);


    // Fetch weather data
    fetchWeatherData();
/*
    // Simulate data for testing
    temperature = 30;
    humidity = 65.2;
    soilMoisture = 100;
*/
    // Log data to serial and MQTT
    String tempPayload = String(temperature);
    String humidityPayload = String(humidity);
    String apiTempPayload = String(outsideTemperature);
    String windSpeedPayload = String(windSpeed);
    String soilMoisturePayload = String(soilMoisture);

    client.publish(mqttTopicTemperature, tempPayload.c_str());
    client.publish(mqttTopicHumidity, humidityPayload.c_str());
    client.publish(mqttTopicApiTemperature, apiTempPayload.c_str());
    client.publish(mqttTopicWindSpeed, windSpeedPayload.c_str());
    client.publish(mqttTopicSoilMoisture, soilMoisturePayload.c_str());
    String payload = "Temperature: " + tempPayload + "°C, Humidity: " + humidityPayload + "%, "
                     + "Outside Temperature: " + apiTempPayload + "°C, Wind Speed: " + windSpeedPayload + " km/h, "
                     + "Soil Moisture: " + soilMoisturePayload;
    Serial.println(payload);

    // Update soil moisture LEDs
    updateSoilMoistureLEDs();

    // Automatic control for relays
    automaticControl();

    delay(10000); // Delay 10 seconds before next reading
}
