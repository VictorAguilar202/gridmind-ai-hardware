# ESP32 troubleshooting

Open the Arduino IDE Serial Monitor at 115200 baud and look for the first error.

| Symptom | What to check |
| --- | --- |
| The board cannot join Wi-Fi | Check the network name, password, and 2.4 GHz availability. |
| The board skips HTTPS telemetry | Check the root CA and confirm the network allows NTP synchronization. |
| The backend rejects the reading | Check the linked sensor code and device token. |
| The DS18B20 is missing | Check power, DATA on GPIO25, and the 4.7 kΩ resistor. |

On a successful test, the monitor reports HTTP 201 and the reading appears in GridMind.
