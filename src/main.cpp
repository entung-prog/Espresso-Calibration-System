#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Preferences.h>
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
Preferences preferences;

struct CalibrationSetting {
    float tdsMin;
    float tdsMax;
    float phMin;
    float phMax;
    float tempMin;
    float tempMax;
    float tolerance;
};

struct SensorReading {
    float temperature;
    float ph;
    float tds;
    float tdsPpm;
    int tdsAdc;
    int phAdc;
    float phVoltage;
    const char *status;
};

CalibrationSetting calibration = {
    8.5f,
    9.5f,
    5.1f,
    5.4f,
    88.0f,
    96.0f,
    0.2f,
};

SensorReading latestReading = {};
unsigned long lastSensorRead = 0;

float clamp01(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }

    if (value > 1.0f) {
        return 1.0f;
    }

    return value;
}

float rising(float value, float start, float end) {
    if (value <= start) {
        return 0.0f;
    }

    if (value >= end) {
        return 1.0f;
    }

    return clamp01((value - start) / (end - start));
}

float falling(float value, float start, float end) {
    if (value <= start) {
        return 1.0f;
    }

    if (value >= end) {
        return 0.0f;
    }

    return clamp01((end - value) / (end - start));
}

float trapezoid(float value, float a, float b, float c, float d) {
    return min(rising(value, a, b), falling(value, c, d));
}

String jsonEscape(const String &value) {
    String escaped;
    escaped.reserve(value.length() + 4);

    for (size_t index = 0; index < value.length(); index++) {
        char current = value[index];
        if (current == '"' || current == '\\') {
            escaped += '\\';
        }
        escaped += current;
    }

    return escaped;
}

String calibrationJson() {
    String json = "{";
    json += "\"tdsMin\":" + String(calibration.tdsMin, 2) + ",";
    json += "\"tdsMax\":" + String(calibration.tdsMax, 2) + ",";
    json += "\"phMin\":" + String(calibration.phMin, 2) + ",";
    json += "\"phMax\":" + String(calibration.phMax, 2) + ",";
    json += "\"tempMin\":" + String(calibration.tempMin, 2) + ",";
    json += "\"tempMax\":" + String(calibration.tempMax, 2) + ",";
    json += "\"tolerance\":" + String(calibration.tolerance, 2);
    json += "}";

    return json;
}

String sensorJson(const SensorReading &reading) {
    String json = "{";
    json += "\"temperature\":" + String(reading.temperature, 2) + ",";
    json += "\"ph\":" + String(reading.ph, 2) + ",";
    json += "\"tds\":" + String(reading.tds, 2) + ",";
    json += "\"tdsPpm\":" + String(reading.tdsPpm, 0) + ",";
    json += "\"status\":\"" + jsonEscape(reading.status) + "\",";
    json += "\"raw\":{";
    json += "\"tdsAdc\":" + String(reading.tdsAdc) + ",";
    json += "\"phAdc\":" + String(reading.phAdc) + ",";
    json += "\"phVoltage\":" + String(reading.phVoltage, 3);
    json += "},";
    json += "\"calibration\":" + calibrationJson();
    json += "}";

    return json;
}

void sendCorsHeaders() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET,POST,PUT,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void sendJson(int code, const String &json) {
    sendCorsHeaders();
    server.send(code, "application/json", json);
}

bool extractJsonNumber(const String &body, const String &key, float &target) {
    int keyIndex = body.indexOf("\"" + key + "\"");
    if (keyIndex < 0) {
        return false;
    }

    int colonIndex = body.indexOf(':', keyIndex);
    if (colonIndex < 0) {
        return false;
    }

    int valueStart = colonIndex + 1;
    while (valueStart < body.length() && isspace(body[valueStart])) {
        valueStart++;
    }

    int valueEnd = valueStart;
    while (valueEnd < body.length()) {
        char current = body[valueEnd];
        if (!isdigit(current) && current != '-' && current != '+'
            && current != '.') {
            break;
        }
        valueEnd++;
    }

    if (valueEnd == valueStart) {
        return false;
    }

    target = body.substring(valueStart, valueEnd).toFloat();
    return true;
}

bool readFloatArg(const String &key, float &target) {
    if (server.hasArg(key)) {
        target = server.arg(key).toFloat();
        return true;
    }

    if (server.hasArg("plain")) {
        return extractJsonNumber(server.arg("plain"), key, target);
    }

    return false;
}

void loadCalibration() {
    preferences.begin("espresso", true);
    calibration.tdsMin = preferences.getFloat("tdsMin", calibration.tdsMin);
    calibration.tdsMax = preferences.getFloat("tdsMax", calibration.tdsMax);
    calibration.phMin = preferences.getFloat("phMin", calibration.phMin);
    calibration.phMax = preferences.getFloat("phMax", calibration.phMax);
    calibration.tempMin = preferences.getFloat("tempMin", calibration.tempMin);
    calibration.tempMax = preferences.getFloat("tempMax", calibration.tempMax);
    calibration.tolerance = preferences.getFloat("tolerance", calibration.tolerance);
    preferences.end();
}

void saveCalibration() {
    preferences.begin("espresso", false);
    preferences.putFloat("tdsMin", calibration.tdsMin);
    preferences.putFloat("tdsMax", calibration.tdsMax);
    preferences.putFloat("phMin", calibration.phMin);
    preferences.putFloat("phMax", calibration.phMax);
    preferences.putFloat("tempMin", calibration.tempMin);
    preferences.putFloat("tempMax", calibration.tempMax);
    preferences.putFloat("tolerance", calibration.tolerance);
    preferences.end();
}

bool isCalibrationValid() {
    return calibration.tdsMin < calibration.tdsMax
        && calibration.phMin < calibration.phMax
        && calibration.tempMin < calibration.tempMax
        && calibration.tolerance > 0.0f;
}

const char *classifyExtraction(float tds, float ph, float temperature) {
    float tdsTolerance = calibration.tolerance;
    float phTolerance = calibration.tolerance;
    float tempTolerance = max(1.0f, calibration.tolerance * 5.0f);

    float tdsLow = falling(tds, calibration.tdsMin - tdsTolerance, calibration.tdsMin);
    float tdsIdeal = trapezoid(
        tds,
        calibration.tdsMin - tdsTolerance,
        calibration.tdsMin,
        calibration.tdsMax,
        calibration.tdsMax + tdsTolerance);
    float tdsHigh = rising(tds, calibration.tdsMax, calibration.tdsMax + tdsTolerance);

    float phAcid = falling(ph, calibration.phMin - phTolerance, calibration.phMin);
    float phIdeal = trapezoid(
        ph,
        calibration.phMin - phTolerance,
        calibration.phMin,
        calibration.phMax,
        calibration.phMax + phTolerance);
    float phBase = rising(ph, calibration.phMax, calibration.phMax + phTolerance);

    float tempLow = falling(
        temperature,
        calibration.tempMin - tempTolerance,
        calibration.tempMin);
    float tempIdeal = trapezoid(
        temperature,
        calibration.tempMin - tempTolerance,
        calibration.tempMin,
        calibration.tempMax,
        calibration.tempMax + tempTolerance);
    float tempHigh = rising(
        temperature,
        calibration.tempMax,
        calibration.tempMax + tempTolerance);

    float underExtract = max(tdsLow, max(phAcid, tempLow));
    float idealEspresso = min(tdsIdeal, min(phIdeal, tempIdeal));
    float overExtract = max(tdsHigh, max(phBase, tempHigh));

    float totalWeight = underExtract + idealEspresso + overExtract;
    if (totalWeight <= 0.0f) {
        return "Unknown";
    }

    float score = ((-1.0f * underExtract) + (1.0f * overExtract)) / totalWeight;
    if (score < -0.25f) {
        return "Under Extract";
    }

    if (score > 0.25f) {
        return "Over Extract";
    }

    return "Ideal Espresso";
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
        classifyExtraction(tds, ph, temperature),
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
    Serial.print("Status      : ");
    Serial.println(reading.status);
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

void handleCalibrationGet() {
    sendJson(200, calibrationJson());
}

void handleCalibrationWrite() {
    CalibrationSetting next = calibration;
    readFloatArg("tdsMin", next.tdsMin);
    readFloatArg("tdsMax", next.tdsMax);
    readFloatArg("phMin", next.phMin);
    readFloatArg("phMax", next.phMax);
    readFloatArg("tempMin", next.tempMin);
    readFloatArg("tempMax", next.tempMax);
    readFloatArg("tolerance", next.tolerance);

    CalibrationSetting previous = calibration;
    calibration = next;
    if (!isCalibrationValid()) {
        calibration = previous;
        sendJson(400, "{\"error\":\"Invalid calibration range\"}");
        return;
    }

    saveCalibration();
    latestReading.status = classifyExtraction(
        latestReading.tds,
        latestReading.ph,
        latestReading.temperature);
    sendJson(200, calibrationJson());
}

void handleCafeGet() {
    sendJson(
        200,
        "{\"items\":[{\"id\":1,\"name\":\"Default Cafe\",\"calibration\":"
            + calibrationJson() + "}]}");
}

void handleNotFound() {
    sendJson(404, "{\"error\":\"Not found\"}");
}

void configureRoutes() {
    server.on("/api/sensor", HTTP_GET, handleSensor);
    server.on("/api/sensor", HTTP_OPTIONS, handleOptions);
    server.on("/api/calibration", HTTP_GET, handleCalibrationGet);
    server.on("/api/calibration", HTTP_POST, handleCalibrationWrite);
    server.on("/api/calibration", HTTP_PUT, handleCalibrationWrite);
    server.on("/api/calibration", HTTP_OPTIONS, handleOptions);
    server.on("/api/cafe", HTTP_GET, handleCafeGet);
    server.on("/api/cafe", HTTP_OPTIONS, handleOptions);
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
    loadCalibration();
    connectWiFi();
    configureRoutes();
    server.begin();
    latestReading = readSensors();
    lastSensorRead = millis();

    Serial.println();
    Serial.println("==========================================");
    Serial.println(" ESPRESSO CALIBRATION SYSTEM");
    Serial.println(" REST API: /api/sensor /api/calibration");
    Serial.println("==========================================");
    printReading(latestReading);
}

void loop() {
    server.handleClient();
    updateSensorsIfNeeded();
}
