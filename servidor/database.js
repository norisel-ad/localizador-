const path = require("path");
const sqlite3 = require("sqlite3").verbose();

const dbPath = path.join(__dirname, "database.sqlite");
const db = new sqlite3.Database(dbPath);

db.serialize(() => {
  db.run(`
    CREATE TABLE IF NOT EXISTS animals (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      deviceId TEXT UNIQUE,
      name TEXT,
      createdAt DATETIME DEFAULT CURRENT_TIMESTAMP
    )
  `);

  db.run(`
    CREATE TABLE IF NOT EXISTS locations (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      deviceId TEXT,
      lat REAL,
      lng REAL,
      battery INTEGER,
      moving INTEGER,
      gpsValid INTEGER,
      gpsTime TEXT,
      createdAt DATETIME DEFAULT CURRENT_TIMESTAMP
    )
  `);

  db.run("ALTER TABLE animals ADD COLUMN createdAt DATETIME DEFAULT CURRENT_TIMESTAMP", () => {});
  db.run("ALTER TABLE locations ADD COLUMN gpsTime TEXT", () => {});
});

module.exports = db;
