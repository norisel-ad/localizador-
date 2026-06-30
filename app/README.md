# GanadoTrack App

Prototipo web/PWA para visualizar el ganado en un mapa, revisar estado de bateria, movimiento e historial de ubicaciones.

## Como abrir

Desde la carpeta del proyecto ejecuta:

```bash
python -m http.server 5173 --bind 127.0.0.1 --directory app
```

Luego abre:

```text
http://127.0.0.1:5173/index.html
```

El mapa usa OpenStreetMap mediante internet.

## Que incluye

- Mapa con ubicaciones del ganado.
- Lista de animales/dispositivos.
- Estado de movimiento y bateria.
- Historial visual del animal seleccionado.
- Boton para abrir la ubicacion en Google Maps.
- Simulador de alerta para demostrar el flujo final.
- Base de datos local en IndexedDB.

## Conexion futura con el dispositivo

El ESP32 puede enviar datos a una API cuando tenga conexion GPRS con el SIM800C. El formato recomendado es:

```json
{
  "deviceId": "GANADO-001",
  "animalName": "Vaca 01",
  "lat": 12.135642,
  "lng": -86.251458,
  "battery": 82,
  "moving": true,
  "gpsValid": true,
  "timestamp": "2026-06-25T22:50:00-06:00"
}
```

Endpoint recomendado:

```text
POST /api/locations
```

Luego la app consultaria:

```text
GET /api/animals
GET /api/animals/:id/locations
```

## Base de datos incluida

La app crea una base local del navegador llamada `ganadotrack` con estas colecciones:

```text
animals
locations
alerts
```

Estructura principal:

```sql
animals(id, name, phone, deviceId, lat, lng, battery, moving, lastSeen, createdAt)
locations(id, animalId, lat, lng, battery, moving, gpsValid, time, createdAt)
alerts(id, animalId, type, message, createdAt, resolvedAt)
```

Los datos quedan guardados en el navegador aunque se cierre o recargue la app.

## Base de datos recomendada para produccion

Para el prototipo final del proyecto se recomienda Firebase o Supabase:

- Firebase: rapido para app movil/PWA, autenticacion y datos en tiempo real.
- Supabase: base PostgreSQL, API limpia e historial facil de consultar.

Tabla minima:

```sql
animals(id, name, phone, device_id, created_at)
locations(id, animal_id, lat, lng, battery, moving, gps_valid, created_at)
alerts(id, animal_id, type, message, created_at, resolved_at)
```
