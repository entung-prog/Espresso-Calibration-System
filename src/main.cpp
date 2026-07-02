#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <WebServer.h>
#include <WiFi.h>

#define TDS_PIN 34
#define PH_PIN 32
#define ONE_WIRE_BUS 4

// Isi jika ingin ESP32 join ke WiFi. Jika kosong/gagal, ESP32 membuat AP sendiri.
const char *WIFI_SSID = "";
const char *WIFI_PASSWORD = "";
const char *AP_SSID = "Espresso-Calibrator";
const char *AP_PASSWORD = "espresso123";

const float VREF = 3.3f;
const int ADC_RESOLUTION = 4095;
const unsigned long SENSOR_INTERVAL_MS = 1000;
const unsigned long WIFI_TIMEOUT_MS = 15000;

// Sesuaikan setelah kalibrasi sensor pH.
const float PH_NEUTRAL_VOLTAGE = 2.50f;
const float PH_SLOPE = 0.18f;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
WebServer server(80);

struct SensorReading {
    float temperature;
    float ph;
    float tds;
    float tdsPpm;
    int tdsAdc;
    int phAdc;
    float phVoltage;
};

SensorReading latestReading = {};
unsigned long lastSensorRead = 0;

String sensorJson(const SensorReading &reading) {
    String json = "{";
    json += "\"temperature\":" + String(reading.temperature, 2) + ",";
    json += "\"ph\":" + String(reading.ph, 2) + ",";
    json += "\"tds\":" + String(reading.tds, 2) + ",";
    json += "\"tdsPpm\":" + String(reading.tdsPpm, 0) + ",";
    json += "\"raw\":{";
    json += "\"tdsAdc\":" + String(reading.tdsAdc) + ",";
    json += "\"phAdc\":" + String(reading.phAdc) + ",";
    json += "\"phVoltage\":" + String(reading.phVoltage, 3);
    json += "}";
    json += "}";

    return json;
}

void sendCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void sendJson(int code, const String &json) {
    sendCorsHeaders();
    server.send(code, "application/json", json);
}

SensorReading readSensors() {
    ds18b20.requestTemperatures();
    float temperature = ds18b20.getTempCByIndex(0);
    if (temperature <= -100.0f || isnan(temperature)) {
        temperature = 25.0f;
    }

    int tdsAdc = analogRead(TDS_PIN);
    float tdsVoltage = tdsAdc * VREF / ADC_RESOLUTION;
    float compensationCoefficient = 1.0f + 0.02f * (temperature - 25.0f);
    float compensationVoltage = tdsVoltage / compensationCoefficient;
    float tdsPpm =
        (133.42f * compensationVoltage * compensationVoltage * compensationVoltage
            - 255.86f * compensationVoltage * compensationVoltage
            + 857.39f * compensationVoltage)
        * 0.5f;
    float tds = max(0.0f, tdsPpm / 100.0f);

    int phAdc = analogRead(PH_PIN);
    float phVoltage = phAdc * VREF / ADC_RESOLUTION;
    float ph = 7.0f + ((PH_NEUTRAL_VOLTAGE - phVoltage) / PH_SLOPE);

    SensorReading reading = {
        temperature,
        ph,
        tds,
        max(0.0f, tdsPpm),
        tdsAdc,
        phAdc,
        phVoltage,
    };

    return reading;
}

void printReading(const SensorReading &reading) {
    Serial.println("------------------------------------------");
    Serial.print("Temperature : ");
    Serial.print(reading.temperature, 2);
    Serial.println(" C");
    Serial.print("TDS         : ");
    Serial.print(reading.tds, 2);
    Serial.print(" % / ");
    Serial.print(reading.tdsPpm, 0);
    Serial.println(" ppm");
    Serial.print("PH          : ");
    Serial.println(reading.ph, 2);
}

void updateSensorsIfNeeded() {
    unsigned long now = millis();
    if (now - lastSensorRead < SENSOR_INTERVAL_MS) {
        return;
    }

    latestReading = readSensors();
    lastSensorRead = now;
    printReading(latestReading);
}

void handleOptions() {
    sendCorsHeaders();
    server.send(204);
}

void handleSensor() {
    updateSensorsIfNeeded();
    sendJson(200, sensorJson(latestReading));
}

void handleNotFound() {
    sendJson(404, "{\"error\":\"Not found\"}");
}

void configureRoutes() {
    server.on("/api/sensor", HTTP_GET, handleSensor);
    server.on("/api/sensor", HTTP_OPTIONS, handleOptions);
    server.onNotFound(handleNotFound);
}

void startAccessPoint() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.print("AP SSID     : ");
    Serial.println(AP_SSID);
    Serial.print("AP IP       : ");
    Serial.println(WiFi.softAPIP());
}

void connectWiFi() {
    if (strlen(WIFI_SSID) == 0) {
        startAccessPoint();
        return;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting WiFi");

    unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < WIFI_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WiFi SSID   : ");
        Serial.println(WIFI_SSID);
        Serial.print("IP Address  : ");
        Serial.println(WiFi.localIP());
        return;
    }

    Serial.println("WiFi failed, starting AP fallback.");
    startAccessPoint();
}

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
    ds18b20.begin();
    connectWiFi();
    configureRoutes();
    server.begin();
    latestReading = readSensors();
    lastSensorRead = millis();

    Serial.println();
    Serial.println("==========================================");
    Serial.println(" ESPRESSO SENSOR NODE");
    Serial.println(" REST API: /api/sensor");
    Serial.println(" Fuzzy evaluation runs in the backend.");
    Serial.println("==========================================");
    printReading(latestReading);
}

void loop() {
    server.handleClient();
    updateSensorsIfNeeded();
}
