#include <OneWire.h>
#include <DallasTemperature.h>
#include <string.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "gridmind_secrets.h"
#include "wifi_setup.h"

constexpr uint8_t DS18B20_PIN = 25;
constexpr uint8_t MPU_SDA_PIN = 21;
constexpr uint8_t MPU_SCL_PIN = 22;
constexpr uint8_t POT1_PIN = 34;
constexpr uint8_t POT2_PIN = 35;
constexpr uint8_t LED_GREEN_PIN = 4;
constexpr uint8_t LED_YELLOW_PIN = 5;
constexpr uint8_t LED_RED_PIN = 2;

constexpr uint8_t MPU6050_ADDRESS_1 = 0x68;
constexpr uint8_t MPU6050_ADDRESS_2 = 0x69;
constexpr uint8_t MPU6050_WHO_AM_I = 0x75;
constexpr uint8_t MPU6050_PWR_MGMT_1 = 0x6B;
constexpr uint8_t MPU6050_ACCEL_CONFIG = 0x1C;
constexpr uint8_t MPU6050_ACCEL_XOUT_H = 0x3B;

constexpr char LEGACY_SENSOR_CODE[] = "SN-001-TMP";
constexpr char SENSOR_UNIT[] = "°C";

OneWire oneWire(DS18B20_PIN);
DallasTemperature temperatureSensor(&oneWire);

uint8_t mpuAddress = 0;
unsigned long lastReadAt = 0;
unsigned long lastTelemetryAt = 0;
constexpr unsigned long READ_INTERVAL_MS = 2000;
constexpr unsigned long TELEMETRY_INTERVAL_MS = 10000;

bool writeMpuRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(mpuAddress);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool readMpuRegister(uint8_t reg, uint8_t &value) {
  Wire.beginTransmission(mpuAddress);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(mpuAddress, 1) != 1) {
    return false;
  }
  value = Wire.read();
  return true;
}

bool readMpuMeasurements(int16_t &ax, int16_t &ay, int16_t &az,
                          int16_t &temperature, int16_t &gx,
                          int16_t &gy, int16_t &gz) {
  Wire.beginTransmission(mpuAddress);
  Wire.write(MPU6050_ACCEL_XOUT_H);
  if (Wire.endTransmission(false) != 0 || Wire.requestFrom(mpuAddress, 14) != 14) {
    return false;
  }

  ax = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  ay = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  az = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  temperature = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  gx = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  gy = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  gz = static_cast<int16_t>((Wire.read() << 8) | Wire.read());
  return true;
}

void setLeds(bool green, bool yellow, bool red) {
  digitalWrite(LED_GREEN_PIN, green ? HIGH : LOW);
  digitalWrite(LED_YELLOW_PIN, yellow ? HIGH : LOW);
  digitalWrite(LED_RED_PIN, red ? HIGH : LOW);
}

void testLeds() {
  const uint8_t ledPins[] = {LED_GREEN_PIN, LED_YELLOW_PIN, LED_RED_PIN};
  for (uint8_t pin : ledPins) {
    digitalWrite(pin, HIGH);
    delay(400);
    digitalWrite(pin, LOW);
    delay(150);
  }
}

void findAndInitializeMpu() {
  const uint8_t addresses[] = {MPU6050_ADDRESS_1, MPU6050_ADDRESS_2};
  for (uint8_t address : addresses) {
    mpuAddress = address;
    uint8_t identity = 0;
    if (readMpuRegister(MPU6050_WHO_AM_I, identity)) {
      Serial.printf("MPU6050 found at 0x%02X; WHO_AM_I=0x%02X\n",
                    mpuAddress, identity);
      if (!writeMpuRegister(MPU6050_PWR_MGMT_1, 0x00) ||
          !writeMpuRegister(MPU6050_ACCEL_CONFIG, 0x00)) {
        Serial.println("ERROR: Could not initialize the MPU6050.");
        mpuAddress = 0;
      }
      return;
    }
  }
  mpuAddress = 0;
  Serial.println("ERROR: MPU6050 not detected. Check VCC, GND, SDA, and SCL.");
}

void printPotentiometers() {
  const int pot1 = analogRead(POT1_PIN);
  const int pot2 = analogRead(POT2_PIN);
  const float pot1Voltage = pot1 * (3.3f / 4095.0f);
  const float pot2Voltage = pot2 * (3.3f / 4095.0f);

  Serial.printf("Pot1 GPIO34: %d (approx. %.2f V) | Pot2 GPIO35: %d (approx. %.2f V)\n",
                pot1, pot1Voltage, pot2, pot2Voltage);

  if (pot1 < 1365) {
    setLeds(true, false, false);
    Serial.println("Test LED: GREEN (turn Pot1 to change)");
  } else if (pot1 < 2730) {
    setLeds(false, true, false);
    Serial.println("Test LED: YELLOW (turn Pot1 to change)");
  } else {
    setLeds(false, false, true);
    Serial.println("Test LED: RED (turn Pot1 to change)");
  }
}

bool printTemperature(float &celsius) {
  temperatureSensor.requestTemperatures();
  celsius = temperatureSensor.getTempCByIndex(0);
  if (celsius == DEVICE_DISCONNECTED_C) {
    Serial.println("DS18B20 not detected. Check VCC, GND, DATA, and the 4.7 kΩ resistor.");
    return false;
  }
  Serial.printf("DS18B20 GPIO25: %.2f °C\n", celsius);
  return true;
}

bool synchronizeSystemClock() {
  if (time(nullptr) >= 1700000000) {
    return true;
  }

  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeInfo {};
  if (!getLocalTime(&timeInfo, 10000)) {
    Serial.println("HTTPS telemetry skipped: Could not synchronize the clock for certificate validation.");
    return false;
  }
  return true;
}

void sendTemperature(float celsius) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Telemetry skipped: ESP32 is disconnected from Wi-Fi.");
    return;
  }
  if (strlen(DEVICE_TOKEN) < 32) {
    Serial.println("Telemetry skipped: DEVICE_TOKEN must be at least 32 characters long.");
    return;
  }

  WiFiClient client;
  WiFiClientSecure secureClient;
  HTTPClient http;
  http.setTimeout(5000);

  bool requestStarted = false;
  if (strncmp(TELEMETRY_URL, "https://", 8) == 0) {
    if (TELEMETRY_ROOT_CA[0] == '\0') {
      Serial.println("HTTPS telemetry skipped: Set the root CA in TELEMETRY_ROOT_CA.");
      return;
    }
    if (!synchronizeSystemClock()) {
      return;
    }
    secureClient.setCACert(TELEMETRY_ROOT_CA);
    requestStarted = http.begin(secureClient, TELEMETRY_URL);
  } else if (strncmp(TELEMETRY_URL, "http://", 7) == 0) {
    Serial.println("WARNING: HTTP sends the token without encryption. Use it only on an isolated local network.");
    requestStarted = http.begin(client, TELEMETRY_URL);
  } else {
    Serial.println("ERROR: TELEMETRY_URL must start with http:// or https://.");
    return;
  }

  if (!requestStarted) {
    Serial.println("ERROR: Could not start the HTTP request. Check TELEMETRY_URL.");
    return;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Token", DEVICE_TOKEN);
  String payload = "[{\"sensorCode\":\"";
  payload += GridMindWifi::sensorCode();
  payload += "\",\"value\":";
  payload += String(celsius, 2);
  payload += ",\"unit\":\"";
  payload += SENSOR_UNIT;
  payload += "\"}]";

  const int statusCode = http.POST(payload);
  if (statusCode > 0) {
    Serial.printf("GridMind returned HTTP %d: %s\n",
                  statusCode, http.getString().c_str());
  } else {
    Serial.printf("ERROR sending telemetry: %s\n",
                  http.errorToString(statusCode).c_str());
  }
  http.end();
}

void printMpuMeasurements() {
  if (mpuAddress == 0) {
    Serial.println("MPU6050: No reading (device not detected).");
    return;
  }

  int16_t ax, ay, az, rawTemperature, gx, gy, gz;
  if (!readMpuMeasurements(ax, ay, az, rawTemperature, gx, gy, gz)) {
    Serial.println("MPU6050: Could not read I2C data.");
    return;
  }

  Serial.printf("MPU6050 acceleration (g): X=%.3f Y=%.3f Z=%.3f | ",
                ax / 16384.0f, ay / 16384.0f, az / 16384.0f);
  Serial.printf("gyroscope (°/s): X=%.2f Y=%.2f Z=%.2f | ",
                gx / 131.0f, gy / 131.0f, gz / 131.0f);
  Serial.printf("internal temperature: %.2f °C\n",
                rawTemperature / 340.0f + 36.53f);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  setLeds(false, false, false);
  testLeds();

  analogReadResolution(12);
  temperatureSensor.begin();
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);

  Serial.println("\n=== ESP32 component test ===");
  findAndInitializeMpu();
  Serial.println("Readings every 2 seconds. Turn Pot1 to test the LEDs.");
  Serial.println("Analog voltage readings are approximate and uncalibrated.");
  Serial.println("GridMind telemetry sends temperature only, every 10 seconds.");
  Serial.println("Unencrypted HTTP is for isolated private networks only.");
  GridMindWifi::begin(LEGACY_SENSOR_CODE);
}

void loop() {
  // Open Wi-Fi setup from the USB serial monitor using the P command.
  while (Serial.available() > 0) {
    if (Serial.read() == 'P') {
      GridMindWifi::startPortal();
    }
  }

  const unsigned long now = millis();
  GridMindWifi::update(now);
  if (now - lastReadAt < READ_INTERVAL_MS) {
    return;
  }
  lastReadAt = now;

  Serial.println("\n--- Readings ---");
  float celsius = 0.0f;
  const bool temperatureAvailable = printTemperature(celsius);
  printMpuMeasurements();
  printPotentiometers();

  if (!GridMindWifi::isConfiguring() && now - lastTelemetryAt >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryAt = now;
    if (temperatureAvailable) {
      sendTemperature(celsius);
    } else {
      Serial.println("Telemetry skipped: No valid DS18B20 reading.");
    }
  }
}