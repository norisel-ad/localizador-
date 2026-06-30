#include <HardwareSerial.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "MPU6050.h"
#include "TinyGPS++.h"

/*
  Localizador ESP32 - version recomendada para primeras pruebas

  Componentes:
  - ESP-32D
  - GPS NEO-6M con antena GPS
  - MPU6050
  - Bateria 18650
  - Regulador LM2596

  Antes de conectar:
  - Ajusta el LM2596 con multimetro antes de alimentar la ESP32.
  - Une todas las tierras: ESP32, GPS, MPU6050 y regulador.
  - Cambia los pines de esta seccion cuando definas tu cableado final.
*/


// CONFIGURACION

const int GPS_RX_PIN = 26;  // ESP32 RX <- NEO-6M TX
const int GPS_TX_PIN = 27;  // ESP32 TX -> NEO-6M RX
const int MPU_SDA_PIN = 21; // MPU6050 SDA
const int MPU_SCL_PIN = 22; // MPU6050 SCL
const int BATTERY_PIN = 34;  // ADC para lectura de bateria

const long SERIAL_BAUD = 115200;
const long GPS_BAUD = 9600;

const unsigned long GPS_PRINT_INTERVAL_MS = 5000;
const unsigned long UPLOAD_INTERVAL_MS = 15000;  // Enviar datos cada 15 segundos

// WiFi
const char* WIFI_SSID = "TU_SSID";
const char* WIFI_PASSWORD = "TU_PASSWORD";
const char* API_SERVER = "http://192.168.1.100:3000/api/locations";  // Cambia la IP del servidor
const char* DEVICE_ID = "GANADO-ESP32-001";

// OBJETOS Y ESTADO

HardwareSerial gpsSerial(2);
TinyGPSPlus gps;
MPU6050 mpu;

bool mpuReady = false;
bool wifiReady = false;

unsigned long lastGpsPrintTime = 0;
unsigned long lastUploadTime = 0;

// =========================
// SETUP
// =========================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);

  Serial.println();
  Serial.println("=== Localizador ESP32 ===");
  Serial.println("Iniciando diagnostico...");

  iniciarPuertos();
  iniciarMPU();
  iniciarWiFi();

  Serial.println("Sistema listo para recibir datos de GPS.");
  Serial.println("Nota: el NEO-6M puede tardar varios minutos en obtener posicion al aire libre.");
}

void loop() {
  leerGPS();
  reportarEstadoGPS();
  
  // Enviar datos al servidor cada UPLOAD_INTERVAL_MS
  if (wifiReady && millis() - lastUploadTime >= UPLOAD_INTERVAL_MS) {
    if (gps.location.isValid() && gps.satellites.value() >= 3) {
      enviarDatos();
    }
  }
  
  delay(100);
}

// =========================
// INICIALIZACION
// =========================

void iniciarPuertos() {
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  delay(100);

  Serial.println("Puertos iniciados.");
}

void iniciarMPU() {
  Serial.print("MPU6050: ");
  mpu.initialize();
  delay(200);

  if (mpu.testConnection()) {
    mpuReady = true;
    Serial.println("OK");
  } else {
    mpuReady = false;
    Serial.println("no detectado. Revisa SDA (pin 21), SCL (pin 22), VCC y GND.");
  }
}

void iniciarWiFi() {
  Serial.print("Conectando WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiReady = true;
    Serial.print("WiFi: OK | IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiReady = false;
    Serial.println("WiFi: no se pudo conectar. Revisa SSID y password.");
  }
}

// =========================
// GPS
// =========================

void leerGPS() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }
}

void reportarEstadoGPS() {
  if (millis() - lastGpsPrintTime < GPS_PRINT_INTERVAL_MS) {
    return;
  }

  lastGpsPrintTime = millis();

  Serial.print("GPS: ");
  if (gps.location.isValid() && gps.satellites.value() >= 3) {
    Serial.print("OK | sats=");
    Serial.print(gps.satellites.value());
    Serial.print(" | lat=");
    Serial.print(gps.location.lat(), 6);
    Serial.print(" | lng=");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.print("esperando posicion valida");
    if (gps.satellites.isValid()) {
      Serial.print(" | sats=");
      Serial.print(gps.satellites.value());
    }
    Serial.println();
  }
}

int leerBateria() {
  // Leer el ADC del pin de bateria (0-4095, corresponde a 0-3.3V)
  int adc = analogRead(BATTERY_PIN);
  
  // Convertir a porcentaje (ajusta estos valores segun tu divisor de tension)
  // Si usas divisor de tension 2:1 con la bateria 18650
  // 4095 = 3.3V (voltaje maximo del ADC) = 6.6V en la bateria = 100%
  // 2048 = 1.65V (voltaje minimo) = 3.3V en la bateria = 0%
  int porcentaje = map(adc, 2048, 4095, 0, 100);
  return constrain(porcentaje, 0, 100);
}

bool detectarMovimiento() {
  if (!mpuReady) {
    return false;
  }
  
  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);
  
  // Calcular magnitud de aceleración
  // Si supera cierto umbral, significa movimiento
  long accelMag = (long)ax * ax + (long)ay * ay + (long)az * az;
  
  // Umbral: 2000000 (aproximadamente 0.5g de aceleración)
  return (accelMag > 2000000);
}

void enviarDatos() {
  if (!wifiReady || WiFi.status() != WL_CONNECTED) {
    return;
  }

  lastUploadTime = millis();

  HTTPClient http;
  
  // Crear JSON con los datos
  String json = "{";
  json += "\"deviceId\":\"" + String(DEVICE_ID) + "\",";
  json += "\"lat\":" + String(gps.location.lat(), 8) + ",";
  json += "\"lng\":" + String(gps.location.lng(), 8) + ",";
  json += "\"battery\":" + String(leerBateria()) + ",";
  json += "\"moving\":" + String(detectarMovimiento() ? 1 : 0) + ",";
  json += "\"gpsValid\":1,";
  json += "\"gpsTime\":\"" + String(gps.date.year()) + "-" + 
          (gps.date.month() < 10 ? "0" : "") + String(gps.date.month()) + "-" + 
          (gps.date.day() < 10 ? "0" : "") + String(gps.date.day()) + "T" +
          (gps.time.hour() < 10 ? "0" : "") + String(gps.time.hour()) + ":" +
          (gps.time.minute() < 10 ? "0" : "") + String(gps.time.minute()) + ":" +
          (gps.time.second() < 10 ? "0" : "") + String(gps.time.second()) + "Z\"";
  json += "}";

  Serial.print("Enviando: ");
  Serial.println(json);

  http.begin(API_SERVER);
  http.addHeader("Content-Type", "application/json");
  
  int httpCode = http.POST(json);
  
  if (httpCode == 200 || httpCode == 201) {
    Serial.println("Servidor: OK");
  } else {
    Serial.print("Servidor: error ");
    Serial.println(httpCode);
  }
  
  http.end();
}
