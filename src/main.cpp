
// Import required libraries
#include <Arduino.h>
#include "WiFi.h"
#include "Wire.h"
#include "DHT.h"
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include "SPIFFS.h"
#include "ArduinoJson.h"

// Replace with your network credentials
const char *ssid = "Home001";
const char *password = "Cdknmjb0pl";

float temperature_Celsius;
float temperature_Fahrenheit;
float Humidity;

AsyncWebServer server(80);
AsyncEventSource events("/events");
DynamicJsonDocument readings(1024);

unsigned long lastTime = 0;
unsigned long timerDelay = 10000; // send readings timer

#define DHTTYPE DHT22 // DHT 22 (AM2302), AM2321
uint8_t DHTPin = 4;
DHT dht(DHTPin, DHTTYPE);

void initDHT()
{
    pinMode(DHTPin, INPUT);
    dht.begin();
    Serial.println("DHT connect success!");
}

String getDHTReadings()
{
    readings["temperature"] = String(dht.readTemperature());
    readings["humidity"] = String(dht.readHumidity());
    String jsonString;
    serializeJson(readings, jsonString);
    return jsonString;
}

void initSPIFFS()
{
    if (!SPIFFS.begin())
    {
        Serial.println("An error has occurred while mounting SPIFFS");
    }
    Serial.println("SPIFFS mounted successfully");
}

void initWiFi()
{
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Setting as a Wi-Fi Station..");
    }
    Serial.print("Station IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

void setup()
{
    Serial.begin(115200);
    initDHT();
    initWiFi();
    initSPIFFS();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/index.html", "text/html"); });

    server.serveStatic("/", SPIFFS, "/");

    // Request for the latest sensor readings
    server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request)
              {
    String json = getDHTReadings();
    request->send(200, "application/json", json);
    json = String(); });

    events.onConnect([](AsyncEventSourceClient *client)
                     {
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
   
    client->send("hello!", NULL, millis(), 10000); });
    server.addHandler(&events);

    // Start server
    server.begin();
}

void loop()
{
    if ((millis() - lastTime) > timerDelay)
    {
        // Send Events to the client with the Sensor Readings Every 10 seconds
        events.send("ping", NULL, millis());
        events.send(getDHTReadings().c_str(), "new_readings", millis());
        lastTime = millis();
    }
}