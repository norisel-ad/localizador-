#include <HardwareSerial.h>
#include <Wire.h>
#include <MPU6050.h>
#include <TinyGPS++.h>

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

const long SERIAL_BAUD = 115200;
const long GPS_BAUD = 9600;

const unsigned long GPS_PRINT_INTERVAL_MS = 5000; // Mostrar ubicacion cada 5 segundos

MPU6050 mpu;
TinyGPSPlus gps;

HardwareSerial gpsSerial(2); // UART2 para NEO-6M

unsigned long lastPrintTime = 0;

bool mpuOK = false;
bool gpsOK = false;

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
  } else {
    gpsOK = false;
    Serial.println("Esperando senal GPS valida...");
  }

  delay(100);
}
