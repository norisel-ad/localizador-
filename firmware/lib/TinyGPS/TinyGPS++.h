#ifndef TINYGPSPLUS_H
#define TINYGPSPLUS_H

#include <Arduino.h>

class Location {
public:
    Location() : _valid(false), _latitude(0.0), _longitude(0.0), _lastUpdate(0) {}
    
    bool isValid() const { return _valid; }
    double lat() const { return _latitude; }
    double lng() const { return _longitude; }
    
    unsigned long age() const {
        if (!_valid) return 0xFFFFFFFF;
        return millis() - _lastUpdate;
    }
    
    void set(double lat, double lng) {
        _latitude = lat;
        _longitude = lng;
        _valid = true;
        _lastUpdate = millis();
    }
    
private:
    bool _valid;
    double _latitude;
    double _longitude;
    unsigned long _lastUpdate;
};

class Satellites {
public:
    Satellites() : _valid(false), _count(0) {}
    
    bool isValid() const { return _valid; }
    uint32_t value() const { return _count; }
    
    void set(uint32_t count) {
        _count = count;
        _valid = (count > 0);
    }
    
private:
    bool _valid;
    uint32_t _count;
};

class TinyGPSPlus {
public:
    TinyGPSPlus() : _satellites(0) {
        // Inicialización simulada
        location.set(0.0, 0.0);
    }
    
    void encode(char c) {
        // Simulación: sin procesamiento real
        // En hardware real, parsearia sentencias NMEA
    }
    
    Location location;
    Satellites satellites;
    
private:
    uint32_t _satellites;
};

#endif // TINYGPSPLUS_H
