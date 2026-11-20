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

constexpr char SENSOR_CODE[] = "SN-001-TMP";
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
      Serial.printf("MPU6050 encontrado en 0x%02X; WHO_AM_I=0x%02X\n",
                    mpuAddress, identity);
      if (!writeMpuRegister(MPU6050_PWR_MGMT_1, 0x00) ||
          !writeMpuRegister(MPU6050_ACCEL_CONFIG, 0x00)) {
        Serial.println("ERROR: no se pudo inicializar el MPU6050.");
        mpuAddress = 0;
      }
      return;
    }
  }
  mpuAddress = 0;
  Serial.println("ERROR: MPU6050 no detectado. Revisa VCC, GND, SDA y SCL.");
}

void printPotentiometers() {
  const int pot1 = analogRead(POT1_PIN);
  const int pot2 = analogRead(POT2_PIN);
  const float pot1Voltage = pot1 * (3.3f / 4095.0f);
  const float pot2Voltage = pot2 * (3.3f / 4095.0f);

  Serial.printf("Pot1 GPIO34: %d (aprox. %.2f V) | Pot2 GPIO35: %d (aprox. %.2f V)\n",
                pot1, pot1Voltage, pot2, pot2Voltage);

  if (pot1 < 1365) {
    setLeds(true, false, false);
    Serial.println("LED de prueba: VERDE (gira Pot1 para cambiar)");
  } else if (pot1 < 2730) {
    setLeds(false, true, false);
    Serial.println("LED de prueba: AMARILLO (gira Pot1 para cambiar)");
  } else {
    setLeds(false, false, true);
    Serial.println("LED de prueba: ROJO (gira Pot1 para cambiar)");
  }
}

bool printTemperature(float &celsius) {
  temperatureSensor.requestTemperatures();
  celsius = temperatureSensor.getTempCByIndex(0);
  if (celsius == DEVICE_DISCONNECTED_C) {
    Serial.println("DS18B20: no detectado. Revisa VCC, GND, DATA y la resistencia de 4.7k.");
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
    Serial.println("Telemetría HTTPS no enviada: no se pudo sincronizar el reloj para validar el certificado.");
    return false;
  }
  return true;
}

void sendTemperature(float celsius) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Telemetría no enviada: ESP32 sin conexión Wi-Fi.");
    return;
  }
  if (strlen(DEVICE_TOKEN) < 32) {
    Serial.println("Telemetría no enviada: DEVICE_TOKEN debe tener al menos 32 caracteres.");
    return;
  }

  WiFiClient client;
  WiFiClientSecure secureClient;
  HTTPClient http;
  http.setTimeout(5000);

  bool requestStarted = false;
  if (strncmp(TELEMETRY_URL, "https://", 8) == 0) {
    if (TELEMETRY_ROOT_CA[0] == '\0') {
      Serial.println("Telemetría HTTPS no enviada: configura el certificado raíz en TELEMETRY_ROOT_CA.");
      return;
    }
    if (!synchronizeSystemClock()) {
      return;
    }
    secureClient.setCACert(TELEMETRY_ROOT_CA);
    requestStarted = http.begin(secureClient, TELEMETRY_URL);
  } else if (strncmp(TELEMETRY_URL, "http://", 7) == 0) {
    Serial.println("ADVERTENCIA: HTTP transmite el token sin cifrar; úsalo solo en una red local aislada.");
    requestStarted = http.begin(client, TELEMETRY_URL);
  } else {
    Serial.println("ERROR: TELEMETRY_URL debe comenzar con http:// o https://.");
    return;
  }

  if (!requestStarted) {
    Serial.println("ERROR: no se pudo iniciar la petición HTTP. Revisa TELEMETRY_URL.");
    return;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Token", DEVICE_TOKEN);
  String payload = "[{\"sensorCode\":\"";
  payload += SENSOR_CODE;
  payload += "\",\"value\":";
  payload += String(celsius, 2);
  payload += ",\"unit\":\"";
  payload += SENSOR_UNIT;
  payload += "\"}]";

  const int statusCode = http.POST(payload);
  if (statusCode > 0) {
    Serial.printf("GridMind respondió HTTP %d: %s\n",
                  statusCode, http.getString().c_str());
  } else {
    Serial.printf("ERROR al enviar telemetría: %s\n",
                  http.errorToString(statusCode).c_str());
  }
  http.end();
}

void printMpuMeasurements() {
  if (mpuAddress == 0) {
    Serial.println("MPU6050: sin lectura (no detectado).");
    return;
  }

  int16_t ax, ay, az, rawTemperature, gx, gy, gz;
  if (!readMpuMeasurements(ax, ay, az, rawTemperature, gx, gy, gz)) {
    Serial.println("MPU6050: error al leer datos I2C.");
    return;
  }

  Serial.printf("MPU6050 aceleracion (g): X=%.3f Y=%.3f Z=%.3f | ",
                ax / 16384.0f, ay / 16384.0f, az / 16384.0f);
  Serial.printf("giroscopio (°/s): X=%.2f Y=%.2f Z=%.2f | ",
                gx / 131.0f, gy / 131.0f, gz / 131.0f);
  Serial.printf("temperatura interna: %.2f °C\n",
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

  Serial.println("\n=== Prueba de componentes ESP32 ===");
  findAndInitializeMpu();
  Serial.println("Lecturas cada 2 segundos. Gira Pot1 para probar los LEDs.");
  Serial.println("Los voltajes analogicos son aproximados; no son una calibracion.");
  Serial.println("Telemetria GridMind: envía solo temperatura cada 10 segundos.");
  Serial.println("HTTP sin cifrar: úsalo únicamente en una red privada aislada.");
  GridMindWifi::begin();
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

  Serial.println("\n--- Lecturas ---");
  float celsius = 0.0f;
  const bool temperatureAvailable = printTemperature(celsius);
  printMpuMeasurements();
  printPotentiometers();

  if (!GridMindWifi::isConfiguring() && now - lastTelemetryAt >= TELEMETRY_INTERVAL_MS) {
    lastTelemetryAt = now;
    if (temperatureAvailable) {
      sendTemperature(celsius);
    } else {
      Serial.println("Telemetría omitida: no hay lectura válida del DS18B20.");
    }
  }
}