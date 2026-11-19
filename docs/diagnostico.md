# Diagnóstico del ESP32

Abre el monitor serie en Arduino IDE a 115200 baudios y sigue el primer mensaje de error.

| Síntoma | Qué revisar |
| --- | --- |
| No se conecta al Wi-Fi | Comprueba el nombre de la red, la contraseña y que sea de 2,4 GHz. |
| La telemetría HTTPS se omite | Revisa el certificado raíz y que la red permita sincronizar la hora por NTP. |
| El backend no acepta la lectura | Comprueba el código vinculado, el token y que el simulador de datos esté pausado. |
| El DS18B20 no aparece | Revisa su alimentación, DATA en GPIO25 y la resistencia de 4,7 kΩ. |

En una prueba correcta, el monitor informa HTTP 201 y el panel recibe la lectura.
