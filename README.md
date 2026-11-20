# GridMind AI · Hardware

Arduino firmware for the ESP32 sends DS18B20 temperature readings to GridMind.

1. Open `esp32_test/esp32_test.ino` in Arduino IDE with the ESP32 Arduino core and the OneWire and DallasTemperature libraries.
2. Copy `esp32_test/gridmind_secrets.example.h` to `esp32_test/gridmind_secrets.h`. Set the Wi-Fi portal password, telemetry token, public HTTPS backend URL, and root CA. Keep this file local.
3. Upload the firmware and [configure Wi-Fi through the portal](docs/portal-wifi.md).
4. [Link the temperature sensor](docs/codigo-sensor.md) under **Activos**. Check that the new reading is labeled **Hardware**. The Serial Monitor at 115200 baud reports the result.

The firmware sends readings to `POST /api/telemetry/readings`. PostgreSQL and the Flyway migrations belong in the backend repository.

## Guides

- [Configure Wi-Fi](docs/portal-wifi.md)
- [Connect to the public HTTPS backend](docs/conexion-nube.md)
- [Link the temperature sensor](docs/codigo-sensor.md)
- [Troubleshoot connectivity and readings](docs/diagnostico.md)
- [Verify the physical device](docs/checklist-placa.md)
