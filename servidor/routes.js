const express = require("express");
const db = require("./database");

const router = express.Router();

router.post("/locations", (req, res) => {
  const d = req.body;

  if (!d.deviceId || d.lat === undefined || d.lng === undefined) {
    return res.status(400).json({
      ok: false,
      error: "deviceId, lat y lng son obligatorios",
    });
  }

  db.serialize(() => {
    db.run(
      `
        INSERT INTO animals (deviceId, name, phone)
        VALUES (?, ?, ?)
        ON CONFLICT(deviceId) DO UPDATE SET
          name = COALESCE(excluded.name, animals.name),
          phone = COALESCE(excluded.phone, animals.phone)
      `,
      [d.deviceId, d.name || defaultName(d.deviceId), d.phone || null],
      (animalErr) => {
        if (animalErr) {
          return res.status(500).json({ ok: false, error: animalErr.message });
        }

        db.run(
          `
            INSERT INTO locations
              (deviceId, lat, lng, battery, moving, gpsValid, gpsTime)
            VALUES (?, ?, ?, ?, ?, ?, ?)
          `,
          [
            d.deviceId,
            Number(d.lat),
            Number(d.lng),
            normalizeInteger(d.battery, 100),
            normalizeInteger(d.moving, 0),
            normalizeInteger(d.gpsValid, 1),
            d.gpsTime || d.timestamp || null,
          ],
          (locationErr) => {
            if (locationErr) {
              return res.status(500).json({ ok: false, error: locationErr.message });
            }

            return res.json({ ok: true });
          },
        );
      },
    );
  });
});

router.get("/animals", (req, res) => {
  db.all(
    `
      SELECT
        a.id AS animalId,
        a.deviceId,
        a.name,
        a.phone,
        l.id AS locationId,
        l.lat,
        l.lng,
        l.battery,
        l.moving,
        l.gpsValid,
        l.gpsTime,
        l.createdAt
      FROM animals a
      INNER JOIN locations l ON l.deviceId = a.deviceId
      INNER JOIN (
        SELECT deviceId, MAX(id) AS latestId
        FROM locations
        GROUP BY deviceId
      ) latest ON latest.latestId = l.id
      ORDER BY l.id DESC
    `,
    (err, rows) => {
      if (err) {
        return res.status(500).json({ ok: false, error: err.message });
      }

      return res.json(rows);
    },
  );
});

router.get("/location/latest", (req, res) => {
  db.get(
    `
      SELECT
        a.id AS animalId,
        a.deviceId,
        a.name,
        a.phone,
        l.id AS locationId,
        l.lat,
        l.lng,
        l.battery,
        l.moving,
        l.gpsValid,
        l.gpsTime,
        l.createdAt
      FROM animals a
      INNER JOIN locations l ON l.deviceId = a.deviceId
      ORDER BY l.id DESC
      LIMIT 1
    `,
    (err, row) => {
      if (err) {
        return res.status(500).json({ ok: false, error: err.message });
      }

      return res.json(row || null);
    },
  );
});

router.get("/device/:deviceId", (req, res) => {
  db.get(
    `
      SELECT
        a.id AS animalId,
        a.deviceId,
        a.name,
        a.phone,
        l.id AS locationId,
        l.lat,
        l.lng,
        l.battery,
        l.moving,
        l.gpsValid,
        l.gpsTime,
        l.createdAt
      FROM animals a
      INNER JOIN locations l ON l.deviceId = a.deviceId
      WHERE a.deviceId = ?
      ORDER BY l.id DESC
      LIMIT 1
    `,
    [req.params.deviceId],
    (err, row) => {
      if (err) {
        return res.status(500).json({ ok: false, error: err.message });
      }

      if (!row) {
        return res.status(404).json({ ok: false, error: "Dispositivo no encontrado" });
      }

      return res.json(row);
    },
  );
});

router.get("/animals/:deviceId/locations", (req, res) => {
  db.all(
    `
      SELECT *
      FROM locations l
      WHERE l.deviceId = ?
      ORDER BY l.id DESC
      LIMIT 25
    `,
    [req.params.deviceId],
    (err, rows) => {
      if (err) {
        return res.status(500).json({ ok: false, error: err.message });
      }

      return res.json(rows);
    },
  );
});

router.post("/animals", (req, res) => {
  const d = req.body;

  if (!d.deviceId) {
    return res.status(400).json({ ok: false, error: "deviceId es obligatorio" });
  }

  db.run(
    `
      INSERT INTO animals (deviceId, name, phone)
      VALUES (?, ?, ?)
      ON CONFLICT(deviceId) DO UPDATE SET
        name = COALESCE(excluded.name, animals.name),
        phone = COALESCE(excluded.phone, animals.phone)
    `,
    [d.deviceId, d.name || defaultName(d.deviceId), d.phone || null],
    (err) => {
      if (err) {
        return res.status(500).json({ ok: false, error: err.message });
      }

      return res.json({ ok: true });
    },
  );
});

router.delete("/animals/:deviceId", (req, res) => {
  const deviceId = req.params.deviceId;

  db.serialize(() => {
    db.run(
      `DELETE FROM locations WHERE deviceId = ?`,
      [deviceId],
      (locationErr) => {
        if (locationErr) {
          return res.status(500).json({ ok: false, error: locationErr.message });
        }

        db.run(
          `DELETE FROM animals WHERE deviceId = ?`,
          [deviceId],
          (animalErr) => {
            if (animalErr) {
              return res.status(500).json({ ok: false, error: animalErr.message });
            }

            return res.json({ ok: true });
          },
        );
      },
    );
  });
});

function normalizeInteger(value, fallback) {
  if (value === undefined || value === null || value === "") {
    return fallback;
  }

  return Number.parseInt(value, 10);
}

function defaultName(deviceId) {
  const suffix = String(deviceId).split("-").pop();
  return `Vaca ${suffix || deviceId}`;
}

module.exports = router;
