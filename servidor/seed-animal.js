const db = require("./database");

const animal = {
  deviceId: "GANADO-ESP32-TEST",
  name: "Vaca de prueba",
  lat: 12.13197,
  lng: -86.26917,
  battery: 100,
  moving: 0,
  gpsValid: 1,
  gpsTime: new Date().toISOString()
};

db.serialize(() => {
  db.run("DELETE FROM locations", (locationsErr) => {
    if (locationsErr) {
      console.error("Error al borrar ubicaciones:", locationsErr.message);
      process.exit(1);
    }

    db.run("DELETE FROM animals", (animalsErr) => {
      if (animalsErr) {
        console.error("Error al borrar animales:", animalsErr.message);
        process.exit(1);
      }

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

              console.log("✅ Base de datos reiniciada y vaca de prueba agregada:");
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
  });
});
