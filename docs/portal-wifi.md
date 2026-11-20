# Change the ESP32 Wi-Fi network

If no network is configured, the firmware opens a temporary network named `GridMind-Setup-…`. On a classic ESP32 with BOOT wired to GPIO0, hold **BOOT** for three seconds after the board starts. You can also send uppercase `P` over USB at 115200 baud to open the portal.

1. Connect from a computer or phone with the local `WIFI_SETUP_PASSWORD`.
2. Open `http://192.168.4.1`. If prompted, stay connected to the temporary network.
3. Enter the 2.4 GHz network name and password. The ESP32 tests the connection before saving.

If the temporary network disappears during testing, reconnect and open the same address. A failed connection preserves the last working configuration.
