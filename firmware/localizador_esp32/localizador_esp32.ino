#include <HardwareSerial.h>
#include <Wire.h>
#include "MPU6050.h"
#include "TinyGPS++.h"

/*
  Localizador ESP32 - version recomendada para primeras pruebas

  Componentes:
  - ESP-32D
  - GPS NEO-6M con antena GPS
  - GSM SIM900 con antena GSM
  - MPU6050
  - Bateria 18650
  - Regulador LM2596

  Antes de conectar:
  - Ajusta el LM2596 con multimetro antes de alimentar la ESP32.
  - No alimentes el SIM900 desde el pin 3V3 de la ESP32.
  - Une todas las tierras: ESP32, GPS, SIM900, MPU6050 y regulador.
  - Cambia los pines de esta seccion cuando definas tu cableado final.
*/


// CONFIGURACION A CAMBIAR

const char PHONE_NUMBER[] = "+50582553697"; // Numero SIM del modem
const char DEVICE_ID[] = "GANADO-ESP32-001"; // Identificador unico del dispositivo
const char APN[] = "internet"; // Cambia al APN de tu operador
const char API_HOST[] = "192.168.1.100"; // IP local donde corre tu servidor
const int API_PORT = 3000;
const char API_PATH[] = "/api/locations";

const int GSM_RX_PIN = 16;  // ESP32 RX <- SIM900 TX
const int GSM_TX_PIN = 17;  // ESP32 TX -> SIM900 RX
const int GPS_RX_PIN = 26;  // ESP32 RX <- NEO-6M TX
const int GPS_TX_PIN = 27;  // ESP32 TX -> NEO-6M RX, opcional en muchos GPS
const int MPU_SDA_PIN = 21; // MPU6050 SDA
const int MPU_SCL_PIN = 22; // MPU6050 SCL

const long SERIAL_BAUD = 115200;
const long GSM_BAUD = 9600;
const long GPS_BAUD = 9600;

const unsigned long SMS_INTERVAL_MS = 60000;
const unsigned long GPS_STATUS_INTERVAL_MS = 5000;
const unsigned long GSM_COMMAND_TIMEOUT_MS = 3000;

// OBJETOS Y ESTADO

HardwareSerial gsmSerial(1);
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;
MPU6050 mpu;

bool mpuReady = false;
bool gsmReady = false;

unsigned long lastSmsTime = 0;
unsigned long lastGpsStatusTime = 0;

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
  iniciarGSM();

  if (gsmReady) {
    gsmReady = iniciarGPRS();
  }

  Serial.println("Sistema listo para buscar GPS.");
  Serial.println("Nota: el NEO-6M puede tardar varios minutos en obtener posicion al aire libre.");
}

void loop() {
  leerGPS();
  reportarEstadoGPS();

  if (!ubicacionLista()) {
    delay(100);
    return;
  }

  if (!gsmReady) {
    delay(1000);
    return;
  }

  if (millis() - lastSmsTime < SMS_INTERVAL_MS) {
    delay(100);
    return;
  }

  enviarUbicacionActual();
}

// =========================
// INICIALIZACION
// =========================

void iniciarPuertos() {
  gsmSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
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

bool ubicacionLista() {
  return gps.location.isValid() &&
         gps.location.age() < 5000 &&
         gps.satellites.isValid() &&
         gps.satellites.value() >= 3;
}

void reportarEstadoGPS() {
  if (millis() - lastGpsStatusTime < GPS_STATUS_INTERVAL_MS) {
    return;
  }

  lastGpsStatusTime = millis();

  Serial.print("GPS: ");
  if (ubicacionLista()) {
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

// =========================
// SMS
// =========================

void enviarUbicacionActual() {
  String payload = crearJsonUbicacion();

  Serial.println("Enviando HTTP POST...");
  if (enviarUbicacionHTTP(payload)) {
    Serial.println("Ubicacion enviada correctamente.");
    lastSmsTime = millis();
  } else {
    Serial.println("Error al enviar ubicacion por HTTP.");
  }
}

String crearJsonUbicacion() {
  String json = "{";
  json += "\"deviceId\":\"" + String(DEVICE_ID) + "\",";
  json += "\"name\":\"ESP32\",";
  json += "\"phone\":\"" + String(PHONE_NUMBER) + "\",";
  json += "\"lat\":" + String(gps.location.lat(), 6) + ",";
  json += "\"lng\":" + String(gps.location.lng(), 6) + ",";
  json += "\"battery\":100,";
  json += "\"moving\":0,";
  json += "\"gpsValid\":1,";
  json += "\"gpsTime\":\"" + String(millis()) + "\"";
  json += "}";
  return json;
}

bool enviarUbicacionHTTP(const String &payload) {
  if (!enviarComandoGSM("AT+HTTPTERM", "OK", GSM_COMMAND_TIMEOUT_MS)) {
    // ignore failure, just ensure HTTP is not active
  }

  if (!enviarComandoGSM("AT+HTTPINIT", "OK", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  if (!enviarComandoGSM("AT+HTTPPARA=\"CID\",1", "OK", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  String url = String("http://") + API_HOST + ":" + String(API_PORT) + API_PATH;
  if (!enviarComandoGSM(String("AT+HTTPPARA=\"URL\",\"") + url + "\"", "OK", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  if (!enviarComandoGSM("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  String dataCmd = String("AT+HTTPDATA=") + payload.length() + ",10000";
  if (!enviarComandoGSM(dataCmd.c_str(), "DOWNLOAD", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  gsmSerial.print(payload);
  gsmSerial.write(26);
  delay(100);

  if (!enviarComandoGSM("AT+HTTPACTION=1", "+HTTPACTION:", 20000)) {
    return false;
  }

  if (!enviarComandoGSM("AT+HTTPREAD", "OK", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  enviarComandoGSM("AT+HTTPTERM", "OK", GSM_COMMAND_TIMEOUT_MS);
  return true;
}

bool iniciarGPRS() {
  if (!enviarComandoGSM("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  String cmdAPN = String("AT+SAPBR=3,1,\"APN\",\"") + APN + "\"";
  if (!enviarComandoGSM(cmdAPN, "OK", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  if (!enviarComandoGSM("AT+SAPBR=3,1,\"USER\",\"\"", "OK", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  if (!enviarComandoGSM("AT+SAPBR=3,1,\"PWD\",\"\"", "OK", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  if (!enviarComandoGSM("AT+SAPBR=1,1", "OK", 15000)) {
    return false;
  }

  if (!enviarComandoGSM("AT+SAPBR=2,1", "+SAPBR:", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  return true;
}

bool enviarSMS(const char *numero, const String &mensaje) {
  if (!enviarComandoGSM("AT+CMGF=1", "OK", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(numero);
  gsmSerial.println("\"");

  if (!esperarRespuestaGSM(">", GSM_COMMAND_TIMEOUT_MS)) {
    return false;
  }

  gsmSerial.print(mensaje);
  gsmSerial.write(26);

  return esperarRespuestaGSM("+CMGS:", 10000) || esperarRespuestaGSM("OK", 10000);
}

bool enviarComandoGSM(const char *comando, const char *esperado, unsigned long timeoutMs) {
  limpiarBufferGSM();
  gsmSerial.println(comando);
  return esperarRespuestaGSM(esperado, timeoutMs);
}

bool esperarRespuestaGSM(const char *esperado, unsigned long timeoutMs) {
  String respuesta = "";
  unsigned long start = millis();

  while (millis() - start < timeoutMs) {
    while (gsmSerial.available()) {
      char c = gsmSerial.read();
      respuesta += c;
      Serial.write(c);

      if (respuesta.indexOf(esperado) >= 0) {
        return true;
      }

      if (respuesta.indexOf("ERROR") >= 0) {
        return false;
      }
    }
  }

  return false;
}

void limpiarBufferGSM() {
  while (gsmSerial.available()) {
    gsmSerial.read();
  }
}
