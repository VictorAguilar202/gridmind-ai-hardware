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
const IPAddress portalIp(192, 168, 4, 1);
WebServer portal(portalIp, 80);

struct Credentials {
  char ssid[33];
  char password[65];
};

Credentials saved {};
Credentials pending {};
String formToken;
const char *message = "";
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

void showForm() {
  if (closePending) {
    sendPage(200, F("<p>Red guardada. El portal se cerrará automáticamente. "
                    "Ya puedes volver a tu red habitual.</p>"));
    return;
  }
  if (attemptQueued || connecting) {
    sendPage(200, F("<meta http-equiv='refresh' content='2; url=/'>"
                    "<p>Probando la conexión. Si pierdes acceso a esta página, "
                    "vuelve a conectarte a la red temporal y abre 192.168.4.1.</p>"));
    return;
  }

  String body = message;
  body += F("<form method='post' action='/wifi'>"
            "<label>Nombre exacto de la red Wi-Fi (2,4 GHz)<br>"
            "<input name='ssid' maxlength='32' required></label><p>"
            "<label>Contraseña de la red<br>"
            "<input name='password' type='password' maxlength='64'></label></p>"
            "<input type='hidden' name='token' value='");
  body += formToken;
  body += F("'><button>Conectar y guardar</button></form>");
  sendPage(200, body);
}

void submitCredentials() {
  if (portal.arg("token") != formToken) {
    portal.send(403, "text/plain", "Abre el formulario desde la red de configuracion.");
    return;
  }
  if (connecting || attemptQueued || closePending) {
    portal.send(409, "text/plain", "Ya hay una configuracion en curso.");
    return;
  }

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
  portal.sendHeader("Location", "/");
  portal.send(303, "text/plain", "");
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
    Serial.println("No se pudo iniciar el portal Wi-Fi.");
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
  Serial.printf("Configuración Wi-Fi: conecta a %s y abre http://192.168.4.1\n", apName);
}

void connectSaved() {
  if (saved.ssid[0] != '\0') {
    WiFi.begin(saved.ssid, saved.password);
    lastRetryAt = millis();
  }
}

void begin() {
  pinMode(SETUP_PIN, INPUT_PULLUP);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  Preferences storage;
  if (storage.begin(STORAGE_NAMESPACE, true)) {
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
        closePending = true;
        Serial.println("Nueva red Wi-Fi guardada.");
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

bool isConfiguring() {
  return portalOpen;
}

}  // namespace GridMindWifi
