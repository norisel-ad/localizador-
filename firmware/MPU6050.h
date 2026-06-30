#ifndef MPU6050_H
#define MPU6050_H

#include <Wire.h>

// Dirección I2C del MPU6050
#define MPU6050_ADDR 0x68

class MPU6050 {
public:
    // Constructor
    MPU6050(uint8_t address = MPU6050_ADDR) : 
        deviceAddress(address), 
        initialized(false), 
        connected(false) {}
    
    // Inicializar el sensor MPU6050
    void initialize() {
        // Intentar comunicación con el sensor
        Wire.beginTransmission(deviceAddress);
        Wire.write(0x6B); // PWR_MGMT_1 register
        Wire.write(0x00); // Despertar el sensor (reset de sleep mode)
        int error = Wire.endTransmission();
        
        initialized = (error == 0);
        if (initialized) {
            connected = true;
        }
    }
    
    // Verificar conexión con el sensor
    bool testConnection() {
        Wire.beginTransmission(deviceAddress);
        Wire.write(0x75); // WHO_AM_I register
        Wire.endTransmission();
        
        Wire.requestFrom(deviceAddress, 1);
        if (Wire.available()) {
            uint8_t whoami = Wire.read();
            // El WHO_AM_I del MPU6050 debe retornar 0x68
            connected = (whoami == 0x68);
        } else {
            connected = false;
        }
        
        return connected;
    }
    
    // Leer aceleración en los 3 ejes (valores raw)
    void getAcceleration(int16_t* x, int16_t* y, int16_t* z) {
        if (!initialized) {
            *x = 0;
            *y = 0;
            *z = 0;
            return;
        }
        
        Wire.beginTransmission(deviceAddress);
        Wire.write(0x3B); // ACCEL_XOUT_H register
        Wire.endTransmission();
        
        Wire.requestFrom(deviceAddress, 6);
        
        if (Wire.available() >= 6) {
            *x = (Wire.read() << 8) | Wire.read();
            *y = (Wire.read() << 8) | Wire.read();
            *z = (Wire.read() << 8) | Wire.read();
        } else {
            *x = 0;
            *y = 0;
            *z = 0;
        }
    }
    
    // Obtener temperatura (en grados C)
    float getTemperature() {
        if (!initialized) return 0.0f;
        
        Wire.beginTransmission(deviceAddress);
        Wire.write(0x41); // TEMP_OUT_H register
        Wire.endTransmission();
        
        Wire.requestFrom(deviceAddress, 2);
        
        if (Wire.available() >= 2) {
            int16_t rawTemp = (Wire.read() << 8) | Wire.read();
            return (rawTemp / 340.0f) + 36.53f;
        }
        return 0.0f;
    }
    
    // Verificar si está inicializado
    bool isInitialized() const {
        return initialized;
    }
    
    // Verificar si está conectado
    bool isConnected() const {
        return connected;
    }

private:
    uint8_t deviceAddress;
    bool initialized;
    bool connected;
};

#endif // MPU6050_H
