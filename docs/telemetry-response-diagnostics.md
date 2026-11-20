# ESP32 telemetry response diagnostics

Use an isolated test board and environment. Record observed results separately.

## Rejected device token

An HTTP 401 response means the request reached the backend but the device credential was rejected. Compare the device and backend configuration privately. Re-test with the expected credential and record only the status code.
