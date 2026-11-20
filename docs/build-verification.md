# ESP32 build and bench verification

Use an isolated test board and environment. Record observed results separately.

## Clean build

Open `esp32_test/esp32_test.ino` in an Arduino IDE profile with the ESP32 core, OneWire and DallasTemperature installed. Select the board model and create the local configuration from the example. Use **Verify** to compile. Record the board profile and dependency versions, and save the first error if the build fails.

## Flash and memory budget

After Verify succeeds, record the reported program storage and global memory usage. Compare both with the selected board's limits. A compile that fits is the expected result. Keep the board profile fixed when comparing builds.

## Serial boot output

Open the Serial Monitor at 115200 baud and press EN once. Check for the `ESP32 component test` banner and at least one reading cycle. If the port is missing, verify the USB data connection. If text is unreadable, recheck the baud rate.
