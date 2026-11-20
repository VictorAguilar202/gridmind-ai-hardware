# Link a physical sensor

Open the ESP32 portal at `http://192.168.4.1`. Under **Sensor de temperatura**, copy the saved code and paste it into **Activos → Pendiente de conexión → Vincular sensor**.

For an additional board, select **Usar código de esta placa** to generate and save its identifier, or enter your own. Keep the existing code if the board is already linked. Changing Wi-Fi preserves the sensor code.

If Wi-Fi was already set up, select **Cerrar portal y reanudar telemetría** when done. Each code belongs to one transformer. If it is already in use, check which transformer owns it before reconfiguring the board.

When the first hardware reading arrives, verify its value, unit, and timestamp.
