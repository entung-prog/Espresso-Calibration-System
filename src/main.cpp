#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <ctype.h>

#define TDS_PIN 34
#define PH_PIN 32
#define ONE_WIRE_BUS 4

// Default WiFi kosong. Ubah lewat POST /api/wifi atau isi konstanta ini jika perlu.
const char *DEFAULT_WIFI_SSID = "tantraa";
const char *DEFAULT_WIFI_PASSWORD = "88888888";
const char *AP_SSID = "Espresso-Calibrator";
const char *AP_PASSWORD = "espresso123";

const float VREF = 3.3f;
const int ADC_RESOLUTION = 4095;
const unsigned long SENSOR_INTERVAL_MS = 1000;
const unsigned long WIFI_TIMEOUT_MS = 15000;
const unsigned long WIFI_RECONNECT_INTERVAL_MS = 10000;

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
String wifiSsid = DEFAULT_WIFI_SSID;
String wifiPassword = DEFAULT_WIFI_PASSWORD;
unsigned long lastWiFiReconnectAttempt = 0;
bool wifiReconnectRequested = false;

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

String wifiModeLabel() {
    wifi_mode_t mode = WiFi.getMode();
    if (mode == WIFI_AP_STA) {
        return "AP_STA";
    }

    if (mode == WIFI_STA) {
        return "STA";
    }

    if (mode == WIFI_AP) {
        return "AP";
    }

    return "OFF";
}

String networkJson() {
    String json = "{";
    json += "\"mode\":\"" + wifiModeLabel() + "\",";
    json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    json += "\"ssid\":\"" + jsonEscape(WiFi.status() == WL_CONNECTED ? WiFi.SSID() : wifiSsid) + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"apSsid\":\"" + String(AP_SSID) + "\",";
    json += "\"apIp\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0);
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
    while (valueStart < body.length() && isspace(static_cast<unsigned char>(body[valueStart]))) {
        valueStart++;
    }

    int valueEnd = valueStart;
    while (valueEnd < body.length()) {
        char current = body[valueEnd];
        if (!isdigit(static_cast<unsigned char>(current)) && current != '-' && current != '+'
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

bool extractJsonString(const String &body, const String &key, String &target) {
    int keyIndex = body.indexOf("\"" + key + "\"");
    if (keyIndex < 0) {
        return false;
    }

    int colonIndex = body.indexOf(':', keyIndex);
    if (colonIndex < 0) {
        return false;
    }

    int quoteStart = body.indexOf('"', colonIndex + 1);
    if (quoteStart < 0) {
        return false;
    }

    String value;
    for (int index = quoteStart + 1; index < body.length(); index++) {
        char current = body[index];
        if (current == '\\' && index + 1 < body.length()) {
            value += body[index + 1];
            index++;
            continue;
        }

        if (current == '"') {
            target = value;
            return true;
        }

        value += current;
    }

    return false;
}

bool readStringArg(const String &key, String &target) {
    if (server.hasArg(key)) {
        target = server.arg(key);
        return true;
    }

    if (server.hasArg("plain")) {
        return extractJsonString(server.arg("plain"), key, target);
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

void loadWiFiCredentials() {
    preferences.begin("espresso", true);
    wifiSsid = preferences.getString("wifiSsid", DEFAULT_WIFI_SSID);
    wifiPassword = preferences.getString("wifiPass", DEFAULT_WIFI_PASSWORD);
    preferences.end();
}

void saveWiFiCredentials() {
    preferences.begin("espresso", false);
    preferences.putString("wifiSsid", wifiSsid);
    preferences.putString("wifiPass", wifiPassword);
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

    // TDS dan pH adalah dasar klasifikasi hasil ekstraksi. Suhu tetap dibaca
    // untuk kompensasi perhitungan TDS dan ditampilkan sebagai informasi, tetapi
    // tidak boleh sendirian mengubah shot ideal menjadi under/over extract.
    float underExtract = max(tdsLow, phAcid);
    float idealEspresso = min(tdsIdeal, phIdeal);
    float overExtract = max(tdsHigh, phBase);

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
            + calibrationJson() + "}]}"
    );
}

void handleNetworkGet() {
    sendJson(200, networkJson());
}

void handleWiFiWrite() {
    String nextSsid = wifiSsid;
    String nextPassword = wifiPassword;
    readStringArg("ssid", nextSsid);
    readStringArg("password", nextPassword);
    nextSsid.trim();

    if (nextSsid.length() == 0) {
        sendJson(400, "{\"error\":\"ssid is required\"}");
        return;
    }

    wifiSsid = nextSsid;
    wifiPassword = nextPassword;
    saveWiFiCredentials();
    wifiReconnectRequested = true;
    lastWiFiReconnectAttempt = 0;
    WiFi.disconnect();
    sendJson(200, "{\"saved\":true,\"message\":\"WiFi saved. ESP32 will reconnect.\"}");
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
    server.on("/api/network", HTTP_GET, handleNetworkGet);
    server.on("/api/network", HTTP_OPTIONS, handleOptions);
    server.on("/api/wifi", HTTP_POST, handleWiFiWrite);
    server.on("/api/wifi", HTTP_OPTIONS, handleOptions);
    server.onNotFound(handleNotFound);
}

void startAccessPoint() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.print("AP SSID     : ");
    Serial.println(AP_SSID);
    Serial.print("AP IP       : ");
    Serial.println(WiFi.softAPIP());
}

bool connectStation(unsigned long timeoutMs) {
    if (wifiSsid.length() == 0) {
        return false;
    }

    WiFi.mode(WIFI_AP_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
    Serial.print("Connecting WiFi");

    unsigned long startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < timeoutMs) {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
        return false;
    }

    Serial.print("WiFi SSID   : ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address  : ");
    Serial.println(WiFi.localIP());
    Serial.print("RSSI        : ");
    Serial.println(WiFi.RSSI());
    return true;
}

void connectWiFi() {
    startAccessPoint();

    if (wifiSsid.length() == 0) {
        Serial.println("WiFi SSID empty, AP fallback only.");
        return;
    }

    wifiReconnectRequested = false;
    if (!connectStation(WIFI_TIMEOUT_MS)) {
        Serial.println("WiFi failed. AP fallback remains active.");
        wifiReconnectRequested = true;
    }
}

void maintainWiFi() {
    if (wifiSsid.length() == 0) {
        return;
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifiReconnectRequested = false;
        return;
    }

    unsigned long now = millis();
    if (!wifiReconnectRequested && now - lastWiFiReconnectAttempt < WIFI_RECONNECT_INTERVAL_MS) {
        return;
    }

    lastWiFiReconnectAttempt = now;
    wifiReconnectRequested = true;
    Serial.println("Retrying WiFi connection...");
    if (connectStation(3000)) {
        wifiReconnectRequested = false;
    }
}

void printNetworkHint() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" Dashboard device URL: http://" + WiFi.localIP().toString());
    } else {
        Serial.println(" Connect to AP, then open: http://" + WiFi.softAPIP().toString());
        Serial.println(" Set WiFi with: POST /api/wifi {\"ssid\":\"...\",\"password\":\"...\"}");
    }
}

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
    ds18b20.begin();
    loadCalibration();
    loadWiFiCredentials();
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
    printNetworkHint();
    printReading(latestReading);
}

void loop() {
    server.handleClient();
    maintainWiFi();
    updateSensorsIfNeeded();
}
