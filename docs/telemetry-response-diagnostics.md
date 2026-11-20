# ESP32 telemetry response diagnostics

Use an isolated test board and environment. Record observed results separately.

## Rejected device token

An HTTP 401 response means the request reached the backend but the device credential was rejected. Compare the device and backend configuration privately. Re-test with the expected credential and record only the status code.

## Unknown or unowned sensor

If the backend returns HTTP 404 with a sensor-related message, compare the exact portal sensor code with the code linked under **Activos**. Confirm that the asset is active, real and in the telemetry owner's account.

## Conflict while the simulator is active

If the HTTP 409 message mentions the simulator, open **Operaciones → Entrada de telemetría** and pause simulation for that account. After another batch, look for HTTP 201. A generic conflict message warrants checking the backend logs.
