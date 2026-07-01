#include <HardwareSerial.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "../MPU6050.h"
#include "../TinyGPS++.h"

/*
  Localizador ESP32 - version anotada para ensamblaje

  Componentes:
  - ESP-32D
  - GPS NEO-6M con antena GPS
  - Sensor MPU6050
  - Bateria recargable 18650
  - Regulador LM2596

  TODO ANTES DE ENSAMBLAR:
  - Confirmar los pines reales segun como armes el circuito.
  - Ajustar el LM2596 antes de conectar la ESP32. La ESP32 debe recibir 5V por VIN/5V
    o 3.3V regulados por el pin 3V3, segun tu placa. No conectes voltajes sin medir.
  - Unir todas las tierras: ESP32 GND, GPS GND, MPU6050 GND y regulador GND.

  Notas de conexion:
  - En UART, TX del GPS va al RX de la ESP32, y RX del GPS va al TX de la ESP32.
  - El MPU6050 usa I2C: SDA y SCL.
  - El GPS NEO-6M normalmente trabaja a 9600 baudios.
*/

// TODO PINES: cambia estos valores cuando definas el cableado final.
const int GPS_RX_PIN = 26;  // ESP32 RX conectado al TX del NEO-6M
const int GPS_TX_PIN = 27;  // ESP32 TX conectado al RX del NEO-6M
const int MPU_SDA_PIN = 21; // SDA del MPU6050
const int MPU_SCL_PIN = 22; // SCL del MPU6050
const int BATTERY_PIN = 34;  // ADC para lectura de bateria

const long SERIAL_BAUD = 115200;
const long GPS_BAUD = 9600;

const unsigned long GPS_PRINT_INTERVAL_MS = 5000; // Mostrar ubicacion cada 5 segundos
const unsigned long UPLOAD_INTERVAL_MS = 15000;  // Enviar datos al servidor cada 15 segundos

// WiFi
const char* WIFI_SSID = "TU_SSID";
const char* WIFI_PASSWORD = "TU_PASSWORD";
const char* API_SERVER = "http://192.168.1.53:3000/api/locations";  // IP local del servidor
const char* DEVICE_ID = "GANADO-ESP32-001";

MPU6050 mpu;
TinyGPSPlus gps;

HardwareSerial gpsSerial(2); // UART2 para NEO-6M

unsigned long lastPrintTime = 0;
unsigned long lastUploadTime = 0;

bool mpuOK = false;
bool gpsOK = false;
bool wifiOK = false;

void setup() {
  Serial.begin(SERIAL_BAUD);
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);
  delay(500);

  Serial.println("Iniciando sistema...");

  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU6050 conectado correctamente.");
    mpuOK = true;
  } else {
    Serial.println("Error: no se detecto el MPU6050.");
  }
  
  // Conectar a WiFi
  iniciarWiFi();

  Serial.println("Esperando datos de GPS...");
}

void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isValid() && gps.satellites.value() >= 3) {
    gpsOK = true;
    float lat = gps.location.lat();
    float lng = gps.location.lng();

    if (millis() - lastPrintTime >= GPS_PRINT_INTERVAL_MS) {
      Serial.print("GPS OK | Lat: ");
      Serial.print(lat, 6);
      Serial.print(" Lng: ");
      Serial.println(lng, 6);

      int16_t ax = 0;
      int16_t ay = 0;
      int16_t az = 0;
      if (mpuOK) {
        mpu.getAcceleration(&ax, &ay, &az);
        Serial.print("Accel - X: ");
        Serial.print(ax);
        Serial.print(" Y: ");
        Serial.print(ay);
        Serial.print(" Z: ");
        Serial.println(az);
      }

      lastPrintTime = millis();
    }
    
    // Enviar datos al servidor cada UPLOAD_INTERVAL_MS
    if (wifiOK && millis() - lastUploadTime >= UPLOAD_INTERVAL_MS) {
      enviarDatos();
    }
  } else {
    gpsOK = false;
    if (millis() - lastPrintTime >= GPS_PRINT_INTERVAL_MS) {
      Serial.println("Esperando senal GPS valida...");
      lastPrintTime = millis();
    }
  }

  delay(100);
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
    wifiOK = true;
    Serial.print("WiFi: OK | IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiOK = false;
    Serial.println("WiFi: no se pudo conectar. Revisa SSID y password.");
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

void enviarDatos() {
  if (!wifiOK || WiFi.status() != WL_CONNECTED) {
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
  json += "\"moving\":0,";
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
