# Cambiar la red del ESP32

Con el firmware cargado y sin una red configurada, el dispositivo abre una red temporal cuyo nombre empieza por `GridMind-Setup-`. También puedes abrirla manteniendo presionado durante tres segundos el pulsador entre GPIO27 y GND.

1. Conéctate desde tu computador o celular con la clave local `WIFI_SETUP_PASSWORD`.
2. Abre `http://192.168.4.1`. Si tu equipo avisa que la red temporal no tiene Internet, conserva esa conexión.
3. Indica el nombre y la contraseña de la red de 2,4 GHz. El ESP32 prueba la conexión antes de guardar el cambio.

Si la red temporal desaparece durante la prueba, inténtalo de nuevo desde la misma dirección. Si la conexión falla, el firmware conserva la última configuración válida.
