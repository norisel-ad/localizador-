const DB_NAME = "ganadotrack";
const DB_VERSION = 1;

let dbPromise;

function openDatabase() {
  if (dbPromise) {
    return dbPromise;
  }

  dbPromise = new Promise((resolve, reject) => {
    const request = indexedDB.open(DB_NAME, DB_VERSION);

    request.addEventListener("upgradeneeded", () => {
      const db = request.result;

      if (!db.objectStoreNames.contains("animals")) {
        const animals = db.createObjectStore("animals", { keyPath: "id" });
        animals.createIndex("deviceId", "deviceId", { unique: false });
        animals.createIndex("createdAt", "createdAt", { unique: false });
      }

      if (!db.objectStoreNames.contains("locations")) {
        const locations = db.createObjectStore("locations", { keyPath: "id" });
        locations.createIndex("animalId", "animalId", { unique: false });
        locations.createIndex("createdAt", "createdAt", { unique: false });
      }

      if (!db.objectStoreNames.contains("alerts")) {
        const alerts = db.createObjectStore("alerts", { keyPath: "id" });
        alerts.createIndex("animalId", "animalId", { unique: false });
        alerts.createIndex("resolvedAt", "resolvedAt", { unique: false });
      }
    });

    request.addEventListener("success", () => resolve(request.result));
    request.addEventListener("error", () => reject(request.error));
  });

  return dbPromise;
}

function runStore(storeName, mode, action) {
  return openDatabase().then((db) => new Promise((resolve, reject) => {
    const transaction = db.transaction(storeName, mode);
    const store = transaction.objectStore(storeName);
    const result = action(store);

    transaction.addEventListener("complete", () => resolve(result));
    transaction.addEventListener("error", () => reject(transaction.error));
    transaction.addEventListener("abort", () => reject(transaction.error));
  }));
}

function requestToPromise(request) {
  return new Promise((resolve, reject) => {
    request.addEventListener("success", () => resolve(request.result));
    request.addEventListener("error", () => reject(request.error));
  });
}

function getAll(storeName) {
  return runStore(storeName, "readonly", (store) => requestToPromise(store.getAll()));
}

function put(storeName, value) {
  return runStore(storeName, "readwrite", (store) => {
    store.put(value);
    return value;
  });
}

function deleteRecord(storeName, id) {
  return runStore(storeName, "readwrite", (store) => {
    store.delete(id);
    return id;
  });
}

async function getLocationsForAnimal(animalId) {
  const db = await openDatabase();

  return new Promise((resolve, reject) => {
    const transaction = db.transaction("locations", "readonly");
    const index = transaction.objectStore("locations").index("animalId");
    const request = index.getAll(animalId);

    request.addEventListener("success", () => {
      const locations = request.result.sort((a, b) => new Date(b.createdAt) - new Date(a.createdAt));
      resolve(locations);
    });
    request.addEventListener("error", () => reject(request.error));
  });
}

async function getAnimalsWithHistory() {
  const animals = await getAll("animals");
  const enriched = await Promise.all(
    animals.map(async (animal) => ({
      ...animal,
      history: await getLocationsForAnimal(animal.id)
    }))
  );

  return enriched.sort((a, b) => new Date(b.createdAt) - new Date(a.createdAt));
}

async function seedAnimalsIfEmpty(defaultAnimals) {
  const animals = await getAll("animals");
  if (animals.length) {
    return;
  }

  const db = await openDatabase();

  await new Promise((resolve, reject) => {
    const transaction = db.transaction(["animals", "locations"], "readwrite");
    const animalStore = transaction.objectStore("animals");
    const locationStore = transaction.objectStore("locations");

    defaultAnimals.forEach((animal) => {
      const createdAt = new Date().toISOString();
      const normalizedAnimal = {
        ...animal,
        deviceId: animal.deviceId || animal.id,
        createdAt
      };

      animalStore.put(normalizedAnimal);
      animal.history.forEach((point, index) => {
        locationStore.put({
          id: crypto.randomUUID(),
          animalId: animal.id,
          lat: point.lat,
          lng: point.lng,
          battery: animal.battery,
          moving: animal.moving,
          gpsValid: true,
          time: point.time,
          createdAt: new Date(Date.now() - index * 600000).toISOString()
        });
      });
    });

    transaction.addEventListener("complete", resolve);
    transaction.addEventListener("error", () => reject(transaction.error));
  });
}

window.GanadoTrackDb = {
  deleteAnimal: (id) => deleteRecord("animals", id),
  getAnimalsWithHistory,
  putAnimal: (animal) => put("animals", animal),
  putLocation: (location) => put("locations", location),
  seedAnimalsIfEmpty
};
