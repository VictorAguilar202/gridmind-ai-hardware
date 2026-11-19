# Conectar el ESP32 al backend publicado

- Configura `TELEMETRY_URL` con la URL HTTPS del backend y la ruta `/api/telemetry/readings`.
- Copia el certificado raíz PEM correspondiente en `TELEMETRY_ROOT_CA`.
- Usa el token de telemetría configurado en el backend. Escribe estos valores en el archivo local ignorado por Git.
- Carga el firmware una vez. Los cambios posteriores de Wi-Fi se pueden guardar desde el portal local.

Para probarlo fuera de tu red, conecta la placa al hotspot de un celular y abre el panel desde otro acceso a Internet. Busca una lectura nueva con origen **Hardware**.
