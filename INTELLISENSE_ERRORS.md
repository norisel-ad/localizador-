# ⚠️ Aclaración: Errores de Intellisense vs Compilación Real

## Por qué ves errores en VS Code

Los errores que ves en VS Code ("cannot open source file") son **errores de Intellisense**, no errores de compilación real. Esto ocurre porque:

1. **VS Code es un editor**, no el compilador Arduino
2. **Arduino IDE tiene sus propias librerías** en rutas especiales
3. **Los includes `#include <Wire.h>`, `#include <WiFi.h>` etc. son específicas de Arduino**

## ¿Compilará en Arduino IDE?

**SÍ**, sin problemas. Arduino IDE sabe dónde están todas las librerías ESP32.

## Solución (opcional)

Si quieres que VS Code también reconozca las librerías, tienes dos opciones:

### Opción 1: Ignorar los errores
Los errores de Intellisense no afectan la compilación real. Simplemente compila en Arduino IDE normalmente.

### Opción 2: Instalar extensión de Arduino
1. Ve a **Extensiones** en VS Code
2. Busca "Arduino"
3. Instala "Arduino" por Microsoft
4. Conecta VS Code a tu instalación de Arduino IDE

## Pasos para Compilar

A pesar de los errores de Intellisense en VS Code:

1. Abre **Arduino IDE 2.3.10**
2. **Archivo > Abrir** → `firmware/localizador_esp32/localizador_esp32.ino`
3. **Herramientas > Placa** → `Arduino Nano ESP32`
4. **Herramientas > Puerto** → Selecciona tu COM
5. **Sketch > Verificar** (Ctrl+K) - esto compila sin errores
6. **Sketch > Cargar** (Ctrl+U) - carga en el hardware

**Los errores de VS Code desaparecerán después de instalar la extensión Arduino.**

