# Device verification checklist

- [ ] Compile the sketch with the ESP32 Arduino core and the OneWire and DallasTemperature libraries.
- [ ] Upload the firmware and confirm detection of the DS18B20.
- [ ] Save Wi-Fi settings through the portal and restart the board. Verify reconnection.
- [ ] Verify HTTPS telemetry while the board and computer are on different networks.
- [ ] Confirm the **Hardware** source and the reading timestamp in GridMind.

Record the board model, test date, and result. Keep credentials out of photos and serial-monitor screenshots.

## Verify the potentiometer channels

1. Pause the data simulator in **Operaciones → Entrada de telemetría**. In **Activos → Transformadores reales**, open **Ver lecturas** for the board's transformer.
2. Record the current and voltage values. Turn only the current knob and wait for the next telemetry batch, normally within 10 seconds. Check that the current changes while voltage stays near its previous value.
3. Turn only the voltage knob and wait for the next batch. Check that voltage changes while current stays near its previous value. Small variations are expected on both analog inputs.

The knobs provide demonstration values. In the current firmware, the voltage knob is connected to GPIO34 and the current knob to GPIO35.
