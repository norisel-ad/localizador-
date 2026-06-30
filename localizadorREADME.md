# Proyecto: Localizador Inteligente para Ganado

## Objetivo
Desarrollar un dispositivo portátil basado en ESP32 para monitorear ganado bovino.

## Hardware
- ESP32 HiLetgo
- GPS GY-NEO6MV2
- MPU6050
- SIM800C
- Batería Li-Ion 4.2V
- Módulo MH-CD42

## Funciones
- Detectar movimiento mediante el MPU6050.
- Obtener coordenadas GPS.
- Enviar SMS con la ubicación usando el SIM800C.
- Optimizar el consumo de energía.

## Arquitectura del software
- main.cpp
- gps.cpp / gps.h
- gsm.cpp / gsm.h
- mpu.cpp / mpu.h
- battery.cpp / battery.h
- config.h

## Flujo del sistema
1. Inicializar módulos.
2. Esperar movimiento.
3. Detectar movimiento.
4. Obtener ubicación GPS.
5. Enviar SMS con enlace a Google Maps.
6. Volver al estado de espera.

## Requisitos
- Código en C++ para ESP32.
- Framework Arduino.
- Código modular y documentado.
- Evitar bloqueos (`delay()` largos).
- Manejo de errores y reconexión de módulos.