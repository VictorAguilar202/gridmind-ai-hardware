#pragma once

// Copia este archivo como gridmind_secrets.h y completa los valores localmente.
// Déjalos vacíos para configurar el Wi-Fi desde el portal del ESP32.
constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = "";
// Reemplaza el ejemplo por una contraseña propia de 8 a 63 caracteres.
constexpr char WIFI_SETUP_PASSWORD[] = "CAMBIA_ESTA_CLAVE_DEL_PORTAL";
constexpr char DEVICE_TOKEN[] = "GENERA_UN_SECRETO_ALEATORIO_DE_32_CARACTERES_O_MAS";

// Para pruebas locales, usa la IP LAN del computador que ejecuta GridMind.
// En la nube, usa la URL HTTPS pública del backend y configura su CA raíz abajo.
constexpr char TELEMETRY_URL[] = "http://192.168.1.20:8080/api/telemetry/readings";

// Para HTTPS, pega el certificado raíz PEM que valida el certificado del servidor.
// No uses setInsecure(): eso desactiva la validación del servidor.
// El ESP32 sincroniza la hora por NTP antes de validar las fechas del certificado.
constexpr char TELEMETRY_ROOT_CA[] = "";
