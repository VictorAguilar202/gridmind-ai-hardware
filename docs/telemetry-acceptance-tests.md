# ESP32 telemetry acceptance tests

Use a USB-powered prototype and a test workspace. Pause the simulator before testing hardware ingestion. Record firmware revision, observed results and the time of the latest batch. The basic two-knob procedure is in [Device verification checklist](checklist-placa.md).

## HW-02 · Voltage scale endpoints

Turn only the voltage knob on the USB-powered prototype. Check that values near the two limits approach 0 and 15 kV in GridMind. Record the observed values as demonstration output.

## HW-03 · Current scale endpoints

Turn only the current knob on the USB-powered prototype. Check that values near the two limits approach 0 and 500 A. Record the observed values as demonstration output.

## HW-04 · Independent analog channels

Hold both knobs steady for two batches to establish normal variation. Move GPIO34, then GPIO35, one at a time. Confirm that the other channel stays within its baseline variation.

## HW-05 · DS18B20 detection

Boot the board and open the serial monitor at 115200 baud. Check for a DS18B20 temperature line and compare it with the latest temperature measurement in GridMind.

## HW-06 · Batch without an MPU-6050

Using a spare test board without an MPU-6050 connected, check the serial diagnostic and send a batch. Expect temperature, voltage and current with no vibration entry.
