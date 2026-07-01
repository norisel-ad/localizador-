const db = require("./database");

const animal = {
  deviceId: "GANADO-ESP32-002",
  name: "Vaca 02",
  lat: 12.131972,  // 12°07'55.1"N
  lng: -86.269167, // 86°16'09.0"W
  battery: 100,
  moving: 0,
  gpsValid: 1,
  gpsTime: new Date().toISOString()
};

db.serialize(() => {
  db.run(
    `
      INSERT INTO animals (deviceId, name)
      VALUES (?, ?)
      ON CONFLICT(deviceId) DO UPDATE SET
        name = COALESCE(excluded.name, animals.name)
    `,
    [animal.deviceId, animal.name],
    (animalErr) => {
      if (animalErr) {
        console.error("Error al agregar animal:", animalErr.message);
        process.exit(1);
      }

      db.run(
        `
          INSERT INTO locations
            (deviceId, lat, lng, battery, moving, gpsValid, gpsTime)
          VALUES (?, ?, ?, ?, ?, ?, ?)
        `,
        [
          animal.deviceId,
          animal.lat,
          animal.lng,
          animal.battery,
          animal.moving,
          animal.gpsValid,
          animal.gpsTime,
        ],
        (locationErr) => {
          if (locationErr) {
            console.error("Error al agregar ubicación:", locationErr.message);
            process.exit(1);
          }

          console.log("✅ Vaca agregada correctamente:");
          console.log(`   Nombre: ${animal.name}`);
          console.log(`   ID: ${animal.deviceId}`);
          console.log(`   Latitud: ${animal.lat}`);
          console.log(`   Longitud: ${animal.lng}`);
          process.exit(0);
        },
      );
    },
  );
});
