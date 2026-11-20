#pragma once

// Copy this file to gridmind_secrets.h and fill in the values locally.
// Leave these blank to configure Wi-Fi through the ESP32 portal.
constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = "";
// Replace the example with your own password of 8 to 63 characters.
constexpr char WIFI_SETUP_PASSWORD[] = "CAMBIA_ESTA_CLAVE_DEL_PORTAL";
constexpr char DEVICE_TOKEN[] = "GENERA_UN_SECRETO_ALEATORIO_DE_32_CARACTERES_O_MAS";

// Para pruebas locales, usa la IP LAN del computador que ejecuta GridMind.
// For cloud deployments, use the public HTTPS backend URL and configure its root CA below.
constexpr char TELEMETRY_URL[] = "http://192.168.1.20:8080/api/telemetry/readings";

// For HTTPS, paste the root CA certificate in PEM format that validates the server certificate.
// Keep server certificate verification enabled. Do not use setInsecure().
// The ESP32 synchronizes time through NTP before checking certificate validity dates.
constexpr char TELEMETRY_ROOT_CA[] = "";
