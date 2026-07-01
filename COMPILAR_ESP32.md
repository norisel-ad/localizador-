# Compilar y Cargar Código en ESP32

## Pasos para Arduino IDE 2.3.10

### 1. Verificar Configuración de Placa
1. Abre Arduino IDE
2. Ve a **Herramientas > Placa > ESP32 Arduino**
3. Selecciona: **Arduino Nano ESP32**
4. Configuración recomendada:
   - Placa: Arduino Nano ESP32
   - CPU Frequency: 80 MHz
   - Flash Size: 4 MB
   - Partition Scheme: Default 4 MB with spiffs

### 2. Verificar Puerto COM
1. Ve a **Herramientas > Puerto**
2. Busca el puerto de la ESP32 (COM3, COM5, etc.)
3. Selecciona ese puerto

### 3. Verificar Librerías Requeridas
Instala si no las tienes:
- **TinyGPS++** (por Mikal Hart)
- **WiFi** (incluida en ESP32 board package)
- **HTTPClient** (incluida en ESP32 board package)
- **Wire** (incluida en ESP32 board package)

Ir a **Herramientas > Gestionar librerías** para instalar TinyGPS++

### 4. Configurar WiFi en el Código

Antes de compilar, edita `localizador_esp32.ino` y configura:

```cpp
const char* WIFI_SSID = "TU_SSID";           // Cambiar a tu red
const char* WIFI_PASSWORD = "TU_PASSWORD";  // Cambiar a tu password
const char* API_SERVER = "http://192.168.1.53:3000/api/locations";  // IP de tu servidor
const char* DEVICE_ID = "GANADO-ESP32-001";
```

### 5. Compilar y Cargar

1. **Abrir archivo:** `firmware/localizador_esp32/localizador_esp32.ino`
2. Click en **Compilar** (marca)
3. Si compila sin errores, click en **Cargar** (flecha)
4. Espera a que termine (normalmente 30-60 segundos)

### 6. Verificar en Monitor Serial

1. Abre **Herramientas > Monitor Serial**
2. Establece velocidad en **115200 baud**
3. Deberías ver:
   ```
   === Localizador ESP32 ===
   Iniciando diagnostico...
   Puertos iniciados.
   MPU6050: OK
   Conectando WiFi: TU_SSID
   ...
   WiFi: OK | IP: 192.168.X.X
   GPS: esperando posicion valida
   ```

## Datos que Envía

Cada 15 segundos (si GPS tiene señal válida + 3 satélites):

```json
{
  "deviceId": "GANADO-ESP32-001",
  "lat": 12.135642,
  "lng": -86.251458,
  "battery": 82,
  "moving": 0,
  "gpsValid": 1,
  "gpsTime": "2026-06-30T14:30:45Z"
}
```

## Pinificación

| Componente | Pin ESP32 | Función |
|-----------|----------|---------|
| GPS RX | 26 | Recibe datos del GPS |
| GPS TX | 27 | Envía datos al GPS |
| MPU6050 SDA | 21 | I2C Data |
| MPU6050 SCL | 22 | I2C Clock |
| Batería | 34 | ADC lectura |

## Solución de Problemas

**"Error: Placa no responde"**
- Verifica puerto COM
- Reinicia ESP32 (desconectar/conectar USB)
- Instala drivers CH340 si es necesario

**"Error: MPU6050 no detectado"**
- Verifica cables I2C (SDA pin 21, SCL pin 22)
- Verifica alimentación (3.3V)
- Verifica resistencias pull-up en I2C

**"No se conecta WiFi"**
- Verifica SSID y contraseña
- Verifica que el router esté disponible
- Revisa mensaje en monitor serial

**"Servidor: error -1"**
- Verifica que el servidor Node.js esté corriendo
- Revisa la IP en `API_SERVER`
- Verifica firewall

## Datos en la App

Una vez cargado, los datos aparecerán en:
- **Web App**: `http://localhost:3000`
- Mapa de ubicación
- Estado de batería
- Historial de ubicaciones
