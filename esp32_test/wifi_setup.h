#pragma once

#include <Preferences.h>
#include <WebServer.h>
#include <esp_system.h>

namespace GridMindWifi {

constexpr uint8_t SETUP_PIN = 0;
constexpr unsigned long BUTTON_HOLD_MS = 3000;
constexpr unsigned long RETRY_MS = 10000;
constexpr unsigned long CONNECT_TIMEOUT_MS = 30000;
constexpr unsigned long PORTAL_CLOSE_MS = 12000;
constexpr char STORAGE_NAMESPACE[] = "gridmind-wifi";
constexpr char STORAGE_KEY[] = "credentials";
constexpr char SENSOR_CODE_KEY[] = "sensor-code";
constexpr unsigned int MAX_SENSOR_CODE_LENGTH = 50;
const IPAddress portalIp(192, 168, 4, 1);
WebServer portal(portalIp, 80);

struct Credentials {
  char ssid[33];
  char password[65];
};

Credentials saved {};
Credentials pending {};
String formToken;
String activeSensorCode;
String suggestedSensorCode;
const char *message = "";
const char *closeMessage = "";
unsigned long pressedAt = 0;
unsigned long lastRetryAt = 0;
unsigned long attemptAt = 0;
unsigned long savedAt = 0;
bool buttonPressed = false;
bool holdHandled = false;
bool portalOpen = false;
bool attemptQueued = false;
bool connecting = false;
bool closePending = false;

void sendPage(int code, const String &body) {
  portal.sendHeader("Cache-Control", "no-store");
  portal.sendHeader("Referrer-Policy", "no-referrer");
  portal.send(code, "text/html; charset=utf-8",
              String(F("<!doctype html><html lang='es'><meta charset='utf-8'>"
                       "<meta name='viewport' content='width=device-width, initial-scale=1'>"
                       "<title>Wi-Fi de GridMind</title><h1>Wi-Fi de GridMind</h1>")) + body);
}

bool validSensorCode(const String &code) {
  if (code.isEmpty() || code.length() > MAX_SENSOR_CODE_LENGTH) {
    return false;
  }
  for (unsigned int index = 0; index < code.length(); ++index) {
    const char value = code[index];
    if (!((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z')
        || (value >= '0' && value <= '9') || value == '-' || value == '_' || value == '.')) {
      return false;
    }
  }
  return true;
}

void redirectToPortal() {
  portal.sendHeader("Location", "/");
  portal.send(303, "text/plain", "");
}

bool canUpdatePortal() {
  if (portal.arg("token") != formToken) {
    portal.send(403, "text/plain", "Abre el formulario desde la red de configuracion.");
    return false;
  }
  if (connecting || attemptQueued || closePending) {
    portal.send(409, "text/plain", "Ya hay una configuracion en curso.");
    return false;
  }
  return true;
}

void showForm() {
  if (closePending) {
    sendPage(200, closeMessage);
    return;
  }
  if (attemptQueued || connecting) {
    sendPage(200, F("<meta http-equiv='refresh' content='2; url=/'>"
                    "<p>Probando la conexión. Si pierdes acceso a esta página, "
                    "vuelve a conectarte a la red temporal y abre 192.168.4.1.</p>"));
    return;
  }

  String body = message;
  body += F("<h2>Sensor de temperatura</h2>"
            "<p>Copia este código y pégalo en GridMind, en Activos → Vincular sensor.</p>"
            "<form method='post' action='/sensor'>"
            "<label>Código del sensor<br><input name='code' maxlength='50' required value='");
  body += activeSensorCode;
  body += F("'></label><input type='hidden' name='token' value='");
  body += formToken;
  body += F("'><p><button type='submit'>Guardar código</button> "
            "<button type='submit' name='automatic' value='1' formnovalidate>Usar código de esta placa</button></p></form>"
            "<h2>Red Wi-Fi</h2>"
            "<form method='post' action='/wifi'>"
            "<label>Nombre exacto de la red Wi-Fi (2,4 GHz)<br>"
            "<input name='ssid' maxlength='32' required></label><p>"
            "<label>Contraseña de la red<br>"
            "<input name='password' type='password' maxlength='64'></label></p>"
            "<input type='hidden' name='token' value='");
  body += formToken;
  body += F("'><button>Conectar y guardar Wi-Fi</button></form>");
  if (saved.ssid[0] != '\0') {
    body += F("<form method='post' action='/finish'><input type='hidden' name='token' value='");
    body += formToken;
    body += F("'><p><button>Cerrar portal y reanudar telemetría</button></p></form>");
  }
  sendPage(200, body);
}

void submitSensorCode() {
  if (!canUpdatePortal()) return;

  String code = portal.hasArg("automatic") ? suggestedSensorCode : portal.arg("code");
  code.trim();
  if (!validSensorCode(code)) {
    message = "<p>Usa de 1 a 50 letras, números, guiones, puntos o guiones bajos.</p>";
    redirectToPortal();
    return;
  }

  Preferences storage;
  if (!storage.begin(STORAGE_NAMESPACE, false)
      || storage.putString(SENSOR_CODE_KEY, code) != code.length()) {
    message = "<p>No se pudo guardar el código. Inténtalo nuevamente.</p>";
  } else {
    activeSensorCode = code;
    message = "<p>Código guardado. Cópialo antes de cerrar el portal.</p>";
  }
  redirectToPortal();
}

void finishSetup() {
  if (!canUpdatePortal()) return;
  if (saved.ssid[0] == '\0') {
    portal.send(409, "text/plain", "Configura la red Wi-Fi antes de cerrar el portal.");
    return;
  }
  closeMessage = "<p>El portal se cerrará en unos segundos. Ya puedes volver a tu red habitual.</p>";
  savedAt = millis();
  closePending = true;
  redirectToPortal();
}

void submitCredentials() {
  if (!canUpdatePortal()) return;

  const String ssid = portal.arg("ssid");
  const String password = portal.arg("password");
  if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 64
      || (!password.isEmpty() && password.length() < 8)
      || strlen(ssid.c_str()) != ssid.length()
      || strlen(password.c_str()) != password.length()) {
    sendPage(400, F("<p>Revisa el nombre y la contraseña de la red.</p><a href='/'>Volver</a>"));
    return;
  }

  pending = {};
  ssid.toCharArray(pending.ssid, sizeof(pending.ssid));
  password.toCharArray(pending.password, sizeof(pending.password));
  attemptQueued = true;
  attemptAt = millis();
  redirectToPortal();
}

void startPortal() {
  if (portalOpen) {
    return;
  }
  const uint32_t chipSuffix = static_cast<uint32_t>(ESP.getEfuseMac() >> 24) & 0xFFFFFF;
  char apName[24];
  snprintf(apName, sizeof(apName), "GridMind-Setup-%06lX", static_cast<unsigned long>(chipSuffix));
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAPConfig(portalIp, portalIp, IPAddress(255, 255, 255, 0))
      || !WiFi.softAP(apName, WIFI_SETUP_PASSWORD, 1, false, 2)) {
    Serial.println("Could not start the Wi-Fi setup portal.");
    WiFi.mode(WIFI_STA);
    return;
  }
  char token[33];
  snprintf(token, sizeof(token), "%08lx%08lx%08lx%08lx",
           static_cast<unsigned long>(esp_random()), static_cast<unsigned long>(esp_random()),
           static_cast<unsigned long>(esp_random()), static_cast<unsigned long>(esp_random()));
  formToken = token;
  message = "";
  portal.begin();
  portalOpen = true;
  Serial.printf("Wi-Fi setup: connect to %s and open http://192.168.4.1\n", apName);
}

void connectSaved() {
  if (saved.ssid[0] != '\0') {
    WiFi.begin(saved.ssid, saved.password);
    lastRetryAt = millis();
  }
}

void begin(const char *legacySensorCode) {
  char generated[24];
  snprintf(generated, sizeof(generated), "GM-%012llX-TMP",
           static_cast<unsigned long long>(ESP.getEfuseMac() & 0xFFFFFFFFFFFFULL));
  suggestedSensorCode = generated;
  activeSensorCode = legacySensorCode;
  if (!validSensorCode(activeSensorCode)) {
    activeSensorCode = suggestedSensorCode;
  }

  pinMode(SETUP_PIN, INPUT_PULLUP);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  Preferences storage;
  if (storage.begin(STORAGE_NAMESPACE, true)) {
    const String storedCode = storage.getString(SENSOR_CODE_KEY, activeSensorCode);
    if (validSensorCode(storedCode)) activeSensorCode = storedCode;
    if (storage.getBytesLength(STORAGE_KEY) == sizeof(saved)) {
      if (storage.getBytes(STORAGE_KEY, &saved, sizeof(saved)) != sizeof(saved)) {
        saved = {};
      }
    }
    storage.end();
  }
  saved.ssid[sizeof(saved.ssid) - 1] = '\0';
  saved.password[sizeof(saved.password) - 1] = '\0';

  // Preserve credentials compiled into earlier installations as a fallback.
  if (saved.ssid[0] == '\0' && WIFI_SSID[0] != '\0') {
    snprintf(saved.ssid, sizeof(saved.ssid), "%s", WIFI_SSID);
    snprintf(saved.password, sizeof(saved.password), "%s", WIFI_PASSWORD);
  }
  portal.on("/", HTTP_GET, showForm);
  portal.on("/wifi", HTTP_POST, submitCredentials);
  portal.on("/sensor", HTTP_POST, submitSensorCode);
  portal.on("/finish", HTTP_POST, finishSetup);
  if (saved.ssid[0] == '\0') {
    startPortal();
  } else {
    connectSaved();
  }
}

void update(unsigned long now) {
  if (digitalRead(SETUP_PIN) == LOW) {
    if (!buttonPressed) {
      buttonPressed = true;
      pressedAt = now;
    }
    if (!holdHandled && now - pressedAt >= BUTTON_HOLD_MS) {
      holdHandled = true;
      startPortal();
    }
  } else {
    buttonPressed = false;
    holdHandled = false;
  }

  if (portalOpen) {
    portal.handleClient();
  }
  if (attemptQueued && millis() - attemptAt >= 500) {
    attemptQueued = false;
    WiFi.disconnect();
    WiFi.begin(pending.ssid, pending.password);
    attemptAt = millis();
    connecting = true;
  }
  if (connecting) {
    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == pending.ssid) {
      Preferences storage;
      if (storage.begin(STORAGE_NAMESPACE, false)
          && storage.putBytes(STORAGE_KEY, &pending, sizeof(pending)) == sizeof(pending)) {
        saved = pending;
        savedAt = millis();
        closeMessage = "<p>Red guardada. El portal se cerrará automáticamente. Ya puedes volver a tu red habitual.</p>";
        closePending = true;
        Serial.println("New Wi-Fi network saved.");
      } else {
        message = "<p>No se pudo guardar la red. Inténtalo nuevamente.</p>";
        WiFi.disconnect();
        connectSaved();
      }
      pending = {};
      connecting = false;
    } else if (millis() - attemptAt >= CONNECT_TIMEOUT_MS) {
      connecting = false;
      pending = {};
      message = "<p>No se pudo conectar. Comprueba el nombre, la contraseña y la señal.</p>";
      WiFi.disconnect();
      connectSaved();
    }
  }
  if (closePending && millis() - savedAt >= PORTAL_CLOSE_MS) {
    portal.stop();
    WiFi.softAPdisconnect(false);
    WiFi.mode(WIFI_STA);
    closePending = false;
    portalOpen = false;
  }
  if (!attemptQueued && !connecting && saved.ssid[0] != '\0'
      && WiFi.status() != WL_CONNECTED && millis() - lastRetryAt >= RETRY_MS) {
    WiFi.reconnect();
    lastRetryAt = millis();
  }
}

const String &sensorCode() {
  return activeSensorCode;
}

bool isConfiguring() {
  return portalOpen;
}

}  // namespace GridMindWifi
