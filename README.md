# GridMind AI · Hardware

Firmware Arduino para el ESP32. Envía la temperatura del DS18B20 al backend de GridMind.

1. Abre `esp32_test/esp32_test.ino` en Arduino IDE con el núcleo Arduino para ESP32 y las bibliotecas OneWire y DallasTemperature.
2. Copia `esp32_test/gridmind_secrets.example.h` a `esp32_test/gridmind_secrets.h`. Define la clave del portal Wi-Fi, el token de telemetría, la URL HTTPS pública del backend y su certificado raíz. Mantén el archivo de secretos en tu equipo.
3. Carga el firmware. Con las credenciales Wi-Fi vacías, conéctate a la red temporal `GridMind-Setup-XXXXXX` y abre `http://192.168.4.1` para registrar la red de 2,4 GHz. Para volver a abrir el portal, presiona durante tres segundos el pulsador conectado entre GPIO27 y GND.
4. Vincula el código `SENSOR_CODE` a un transformador real en **Activos** y comprueba que la lectura aparezca con origen **Hardware**. El monitor serie a 115200 baudios muestra el resultado del envío.

El firmware envía lecturas a `POST /api/telemetry/readings`. PostgreSQL y las migraciones de Flyway forman parte del repositorio del backend.

## Guías

- [explicar el cambio de Wi-Fi del ESP32](docs/portal-wifi.md)
- [explicar la conexión HTTPS del ESP32](docs/conexion-nube.md)
- [explicar cómo vincular el código del sensor](docs/codigo-sensor.md)
- [registrar diagnóstico de conexión y lecturas](docs/diagnostico.md)
- [añadir checklist de prueba física del ESP32](docs/checklist-placa.md)
