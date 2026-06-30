#include <HardwareSerial.h>
#include <Wire.h>
#include <MPU6050.h>
#include <TinyGPS++.h>

/*
  Localizador ESP32 - version anotada para ensamblaje

  Componentes actuales:
  - ESP-32D
  - GPS NEO-6M con antena GPS
  - GSM SIM900 con antena GSM
  - Sensor MPU6050
  - Bateria recargable 18650
  - Regulador LM2596

  TODO ANTES DE ENSAMBLAR:
  - Confirmar los pines reales segun como armes el circuito.
  - Cambiar el numero de telefono en PHONE_NUMBER.
  - Ajustar el LM2596 antes de conectar la ESP32. La ESP32 debe recibir 5V por VIN/5V
    o 3.3V regulados por el pin 3V3, segun tu placa. No conectes voltajes sin medir.
  - Verificar que el SIM900 tenga alimentacion suficiente. Suele necesitar picos altos
    de corriente al transmitir; no conviene alimentarlo directo desde el 3V3 de la ESP32.
  - Unir todas las tierras: ESP32 GND, GPS GND, SIM900 GND, MPU6050 GND y regulador GND.

  Notas de conexion:
  - En UART, TX de un modulo va al RX de la ESP32, y RX del modulo va al TX de la ESP32.
  - El MPU6050 usa I2C: SDA y SCL.
  - El GPS NEO-6M normalmente trabaja a 9600 baudios.
  - El SIM900 normalmente trabaja a 9600 baudios, pero algunos modulos pueden venir con
    otra velocidad configurada.
*/

// TODO PINES: cambia estos valores cuando definas el cableado final.
const int GSM_RX_PIN = 16;  // ESP32 RX conectado al TX del SIM900
const int GSM_TX_PIN = 17;  // ESP32 TX conectado al RX del SIM900
const int GPS_RX_PIN = 26;  // ESP32 RX conectado al TX del NEO-6M
const int GPS_TX_PIN = 27;  // ESP32 TX conectado al RX del NEO-6M, si tu modulo lo requiere
const int MPU_SDA_PIN = 21; // SDA del MPU6050
const int MPU_SCL_PIN = 22; // SCL del MPU6050

const long SERIAL_BAUD = 115200;
const long GSM_BAUD = 9600;
const long GPS_BAUD = 9600;

// TODO TELEFONO: cambia este numero por el numero real que recibira los SMS.
const char PHONE_NUMBER[] = "+1234567890";

const unsigned long SMS_INTERVAL_MS = 60000; // 60 segundos para evitar spam de SMS

MPU6050 mpu;
TinyGPSPlus gps;

HardwareSerial gsmSerial(1); // UART1 para SIM900
HardwareSerial gpsSerial(2); // UART2 para NEO-6M

unsigned long lastSMSTime = 0;

bool mpuOK = false;
bool gsmOK = false;
bool gpsOK = false;

void setup() {
  Serial.begin(SERIAL_BAUD);

  gsmSerial.begin(GSM_BAUD, SERIAL_8N1, GSM_RX_PIN, GSM_TX_PIN);
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

  gsmOK = enviarComandoGSM("AT", "OK", 2000);
  if (gsmOK) {
    enviarComandoGSM("AT+CMGF=1", "OK", 1000); // Modo texto SMS
    Serial.println("SIM900 conectado correctamente.");
  } else {
    Serial.println("Error: SIM900 no responde.");
  }

  Serial.println("Esperando datos de GPS...");
}

void loop() {
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  if (gps.location.isUpdated() && gps.location.isValid() && gps.satellites.value() >= 3) {
    gpsOK = true;
    float lat = gps.location.lat();
    float lng = gps.location.lng();

    Serial.print("Latitud: ");
    Serial.println(lat, 6);
    Serial.print("Longitud: ");
    Serial.println(lng, 6);

    int16_t ax = 0;
    int16_t ay = 0;
    int16_t az = 0;
    if (mpuOK) {
      mpu.getAcceleration(&ax, &ay, &az);
    }

    if (millis() - lastSMSTime >= SMS_INTERVAL_MS && gsmOK) {
      String mensaje = "Ubicacion:\nLat: " + String(lat, 6) + "\nLng: " + String(lng, 6);
      if (mpuOK) {
        mensaje += "\nAccel:\nX: " + String(ax) + "\nY: " + String(ay) + "\nZ: " + String(az);
      }

      bool enviado = enviarSMS(PHONE_NUMBER, mensaje);

      if (enviado) {
        Serial.println("SMS enviado correctamente.");
        lastSMSTime = millis();
      } else {
        Serial.println("Error al enviar SMS.");
      }
    }
  } else {
    gpsOK = false;
    Serial.println("Esperando senal GPS valida...");
  }

  delay(1000);
}

bool enviarSMS(String numero, String mensaje) {
  gsmSerial.println("AT+CMGF=1");
  delay(500);

  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(numero);
  gsmSerial.println("\"");
  delay(1000);

  gsmSerial.print(mensaje);
  gsmSerial.write(26); // Ctrl+Z para enviar el SMS
  delay(3000);

  unsigned long timeout = millis() + 5000;
  while (millis() < timeout) {
    if (gsmSerial.available()) {
      String respuesta = gsmSerial.readString();
      if (respuesta.indexOf("OK") >= 0 || respuesta.indexOf("+CMGS:") >= 0) {
        return true;
      }
    }
  }

  return false;
}

bool enviarComandoGSM(String comando, String esperado, int tiempoEspera) {
  gsmSerial.println(comando);
  unsigned long start = millis();

  while (millis() - start < tiempoEspera) {
    if (gsmSerial.available()) {
      String respuesta = gsmSerial.readString();
      if (respuesta.indexOf(esperado) >= 0) {
        return true;
      }
    }
  }

  return false;
}
