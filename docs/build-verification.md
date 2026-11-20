# ESP32 build and bench verification

Use an isolated test board and environment. Record observed results separately.

## Clean build

Open `esp32_test/esp32_test.ino` in an Arduino IDE profile with the ESP32 core, OneWire and DallasTemperature installed. Select the board model and create the local configuration from the example. Use **Verify** to compile. Record the board profile and dependency versions, and save the first error if the build fails.
