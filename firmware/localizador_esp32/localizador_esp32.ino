#include <HardwareSerial.h>
#include <Wire.h>
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

const long SERIAL_BAUD = 115200;
const long GPS_BAUD = 9600;

const unsigned long GPS_PRINT_INTERVAL_MS = 5000;

// OBJETOS Y ESTADO

HardwareSerial gpsSerial(2);
TinyGPSPlus gps;
MPU6050 mpu;

bool mpuReady = false;

unsigned long lastGpsPrintTime = 0;

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

  Serial.println("Sistema listo para recibir datos de GPS.");
  Serial.println("Nota: el NEO-6M puede tardar varios minutos en obtener posicion al aire libre.");
}

void loop() {
  leerGPS();
  reportarEstadoGPS();
  delay(100);
}

// =========================
// INICIALIZACION
// =========================

void iniciarPuertos() {
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Wire.begin(MPU_SDA_PIN, MPU_SCL_PIN);

  Serial.println("Puertos iniciados.");
}

void iniciarMPU() {
  mpu.initialize();

  if (mpu.testConnection()) {
    mpuReady = true;
    Serial.println("MPU6050: OK");
  } else {
    mpuReady = false;
    Serial.println("MPU6050: no detectado. Revisa SDA, SCL, VCC y GND.");
  }
}

void iniciarGSM() {
  Serial.println("Probando SIM900...");

  gsmReady = enviarComandoGSM("AT", "OK", GSM_COMMAND_TIMEOUT_MS);
  if (!gsmReady) {
    Serial.println("SIM900: no responde. Revisa alimentacion, antena, RX/TX y GND comun.");
    return;
  }

  enviarComandoGSM("ATE0", "OK", GSM_COMMAND_TIMEOUT_MS);
  enviarComandoGSM("AT+CMGF=1", "OK", GSM_COMMAND_TIMEOUT_MS);
  Serial.println("SIM900: OK, modo SMS texto activo.");
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
