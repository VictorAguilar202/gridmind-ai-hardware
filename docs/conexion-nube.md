# Connect the ESP32 to a deployed backend

- Set `TELEMETRY_URL` to the public HTTPS backend URL with the path `/api/telemetry/readings`.
- Put the matching PEM root CA in `TELEMETRY_ROOT_CA`.
- Use the same telemetry token as the backend. Keep these values in the local file excluded from Git.
- Upload the firmware once. Subsequent Wi-Fi changes can be made through the portal.

To verify connectivity across networks, connect the board to a phone hotspot and open GridMind from another internet connection. Look for a recent reading labeled **Hardware**.
