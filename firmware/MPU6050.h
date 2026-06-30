#ifndef MPU6050_H
#define MPU6050_H

class MPU6050 {
public:
    MPU6050() : initialized(false), connected(false) {}
    
    void initialize() {
        initialized = true;
    }
    
    bool testConnection() {
        // En hardware real, esto verifica la comunicación I2C
        // Por ahora retorna true para permitir compilación
        connected = true;
        return connected;
    }
    
private:
    bool initialized;
    bool connected;
};

#endif // MPU6050_H
