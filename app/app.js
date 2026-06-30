const defaultAnimals = [
  {
    id: crypto.randomUUID(),
    deviceId: "GANADO-001",
    name: "Vaca 01",
    lat: 12.135642,
    lng: -86.251458,
    battery: 82,
    moving: false,
    lastSeen: "Hace 2 min",
    history: [
      { lat: 12.135642, lng: -86.251458, time: "Hace 2 min" },
      { lat: 12.1345, lng: -86.2509, time: "Hace 12 min" },
      { lat: 12.1339, lng: -86.2501, time: "Hace 24 min" }
    ]
  },
  {
    id: crypto.randomUUID(),
    deviceId: "GANADO-003",
    name: "Toro 03",
    lat: 12.13921,
    lng: -86.24793,
    battery: 28,
    moving: true,
    lastSeen: "Hace 1 min",
    history: [
      { lat: 12.13921, lng: -86.24793, time: "Hace 1 min" },
      { lat: 12.1387, lng: -86.2484, time: "Hace 8 min" }
    ]
  },
  {
    id: crypto.randomUUID(),
    deviceId: "GANADO-008",
    name: "Vaca 08",
    lat: 12.13174,
    lng: -86.2553,
    battery: 64,
    moving: false,
    lastSeen: "Hace 7 min",
    history: [
      { lat: 12.13174, lng: -86.2553, time: "Hace 7 min" },
      { lat: 12.1321, lng: -86.2549, time: "Hace 17 min" }
    ]
  }
];

let animals = [];
let selectedAnimalId = null;
let map;
let markers = new Map();
let routeLayer;
let resizeTimer;

const API_URL = "http://localhost:3000/api";

const animalList = document.querySelector("#animalList");
const totalAnimals = document.querySelector("#totalAnimals");
const activeAlerts = document.querySelector("#activeAlerts");
const lowBattery = document.querySelector("#lowBattery");
const lastEvent = document.querySelector("#lastEvent");
const dialog = document.querySelector("#animalDialog");
const animalForm = document.querySelector("#animalForm");
const nameInput = document.querySelector("#animalNameInput");

startApp();

async function loadAnimalsFromServer() {
  try {
    const response = await fetch(`${API_URL}/animals`);
    if (!response.ok) throw new Error("Server error");
    const data = await response.json();
    
    if (data && data.length > 0) {
      animals = data.map((animal) => ({
        id: animal.deviceId,
        deviceId: animal.deviceId,
        name: animal.name || animal.deviceId,
        lat: animal.lat,
        lng: animal.lng,
        battery: animal.battery,
        moving: animal.moving === 1,
        lastSeen: "Ahora",
        history: []
      }));
      return;
    }
  } catch (err) {
    console.warn("No se pudo conectar al servidor:", err);
  }
  
  // Usar datos de prueba si el servidor no está disponible
  animals = defaultAnimals.map((animal) => ({
    ...animal,
    lastSeen: "Ahora"
  }));
}

async function startApp() {
  await loadAnimalsFromServer();
  selectedAnimalId = animals[0]?.id ?? null;

  initMap();
  bindEvents();
  render();

  setInterval(async () => {
    await loadAnimalsFromServer();
    render();
  }, 5000);
}

function bindEvents() {
  document.querySelector("#centerMapBtn").addEventListener("click", fitMapToAnimals);
  document.querySelector("#simulateAlertBtn").addEventListener("click", simulateAlert);
  window.addEventListener("resize", refreshMapLayout);
  document.querySelector("#addAnimalBtn").addEventListener("click", () => {
    nameInput.value = "";
    dialog.showModal();
    nameInput.focus();
  });
  document.querySelector("#cancelDialogBtn").addEventListener("click", () => dialog.close());
  animalForm.addEventListener("submit", async (event) => {
    event.preventDefault();
    await addAnimal(nameInput.value.trim());
    dialog.close();
  });
}

function initMap() {
  map = L.map("map", {
    zoomControl: true
  }).setView([12.135642, -86.251458], 15);

  L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
    maxZoom: 19,
    attribution: "&copy; OpenStreetMap"
  }).addTo(map);

  refreshMapLayout();
}

function render() {
  renderStats();
  renderList();
  renderMarkers();
  renderRoute();
}

function renderStats() {
  totalAnimals.textContent = animals.length;
  activeAlerts.textContent = animals.filter((animal) => animal.moving).length;
  lowBattery.textContent = animals.filter((animal) => animal.battery <= 30).length;
}

function renderList() {
  animalList.innerHTML = "";

  animals.forEach((animal) => {
    const status = getStatus(animal);
    const card = document.createElement("article");
    card.className = "animal-card";
    card.innerHTML = `
      <div class="animal-head">
        <div>
          <div class="animal-name"><span class="animal-emoji" aria-hidden="true">🐮</span>${escapeHtml(animal.name)}</div>
        </div>
        <span class="tag ${status.kind}">${status.label}</span>
      </div>
      <div class="animal-meta">
        <span>Ubicacion: ${animal.lat.toFixed(6)}, ${animal.lng.toFixed(6)}</span>
        <span>Bateria: ${animal.battery}%</span>
        <span>Ultimo reporte: ${escapeHtml(animal.lastSeen)}</span>
      </div>
      <div class="animal-actions">
        <button type="button" data-action="view" data-id="${animal.id}">Ver</button>
        <button type="button" data-action="maps" data-id="${animal.id}">Maps</button>
        <button type="button" data-action="delete" data-id="${animal.id}">Eliminar</button>
      </div>
    `;
    animalList.appendChild(card);
  });

  animalList.querySelectorAll("button").forEach((button) => {
    button.addEventListener("click", handleAnimalAction);
  });
}

function renderMarkers() {
  const currentIds = new Set(animals.map((animal) => animal.id));

  markers.forEach((marker, id) => {
    if (!currentIds.has(id)) {
      map.removeLayer(marker);
      markers.delete(id);
    }
  });

  animals.forEach((animal) => {
    const marker = markers.get(animal.id);
    const label = `<strong>🐮 ${escapeHtml(animal.name)}</strong><br>${animal.lat.toFixed(6)}, ${animal.lng.toFixed(6)}<br>Bateria: ${animal.battery}%`;

    if (marker) {
      marker.setLatLng([animal.lat, animal.lng]).setPopupContent(label);
      return;
    }

    const newMarker = L.marker([animal.lat, animal.lng], {
      icon: L.divIcon({
        className: "animal-map-marker",
        html: "🐮",
        iconSize: [36, 36],
        iconAnchor: [18, 18],
        popupAnchor: [0, -18]
      })
    }).addTo(map);
    newMarker.bindPopup(label);
    markers.set(animal.id, newMarker);
  });

  refreshMapLayout();
}

function renderRoute() {
  if (routeLayer) {
    map.removeLayer(routeLayer);
    routeLayer = null;
  }

  const selected = animals.find((animal) => animal.id === selectedAnimalId);
  if (!selected || selected.history.length < 2) {
    return;
  }

  routeLayer = L.polyline(
    selected.history.map((point) => [point.lat, point.lng]),
    { color: "#b33a2f", weight: 4, opacity: 0.8 }
  ).addTo(map);
}

async function handleAnimalAction(event) {
  const id = event.currentTarget.dataset.id;
  const action = event.currentTarget.dataset.action;
  const animal = animals.find((item) => item.id === id);
  if (!animal) {
    return;
  }

  selectedAnimalId = id;

  if (action === "delete") {
    const confirmed = confirm(`¿Eliminar ${animal.name}? Esta acción no se puede deshacer.`);
    if (!confirmed) {
      return;
    }

    await deleteAnimal(animal.deviceId);
    return;
  }

  if (action === "maps") {
    window.open(`https://maps.google.com/?q=${animal.lat},${animal.lng}`, "_blank", "noopener");
    return;
  }

  markers.get(id)?.openPopup();
  map.setView([animal.lat, animal.lng], 17);
  lastEvent.innerHTML = `Seleccionado <strong>${escapeHtml(animal.name)}</strong><br>Ultima ubicacion registrada: ${animal.lat.toFixed(6)}, ${animal.lng.toFixed(6)}`;
  renderRoute();
}

async function deleteAnimal(deviceId) {
  await fetch(`${API_URL}/animals/${encodeURIComponent(deviceId)}`, {
    method: "DELETE"
  });

  await loadAnimalsFromServer();
  selectedAnimalId = animals[0]?.id ?? null;
  render();
}

function getLocationFallback() {
  if (selectedAnimalId) {
    const selected = animals.find((item) => item.id === selectedAnimalId);
    if (selected) {
      return { lat: selected.lat, lng: selected.lng };
    }
  }

  if (animals.length > 0) {
    const randomAnimal = animals[Math.floor(Math.random() * animals.length)];
    return { lat: randomAnimal.lat + randomOffset(0.0002), lng: randomAnimal.lng + randomOffset(0.0002) };
  }

  return { lat: defaultAnimals[0].lat, lng: defaultAnimals[0].lng };
}

async function addAnimal(name) {
  if (!name) {
    return;
  }

  const fallback = getLocationFallback();
  const deviceId = `GANADO-${String(Date.now()).slice(-6)}`;
  const animalPayload = {
    deviceId,
    name,
    lat: fallback.lat,
    lng: fallback.lng,
    battery: 100,
    moving: 0,
    gpsValid: 1,
    gpsTime: new Date().toISOString()
  };

  await fetch(`${API_URL}/locations`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json"
    },
    body: JSON.stringify(animalPayload)
  });

  await loadAnimalsFromServer();
  selectedAnimalId = animals[0]?.id ?? null;
  render();
}

async function simulateAlert() {
  const animal = animals.find((item) => item.id === selectedAnimalId) ?? animals[0];
  if (!animal) {
    return;
  }

  animal.lat += randomOffset();
  animal.lng += randomOffset();
  animal.battery = Math.max(5, animal.battery - Math.floor(Math.random() * 7));
  animal.moving = true;
  animal.lastSeen = "Ahora";
  const location = {
    id: crypto.randomUUID(),
    animalId: animal.id,
    lat: animal.lat,
    lng: animal.lng,
    battery: animal.battery,
    moving: animal.moving,
    gpsValid: true,
    time: "Ahora",
    createdAt: new Date().toISOString()
  };

  animal.history.unshift(location);
  animal.history = animal.history.slice(0, 12);

  lastEvent.innerHTML = `
    <strong>Movimiento detectado</strong><br>
    ${escapeHtml(animal.name)} reporto nueva ubicacion.<br>
    ${animal.lat.toFixed(6)}, ${animal.lng.toFixed(6)}<br>
    SMS/API listo para enviar.
  `;

  await GanadoTrackDb.putAnimal(animal);
  await GanadoTrackDb.putLocation(location);
  render();
  markers.get(animal.id)?.openPopup();
}

function fitMapToAnimals() {
  if (!animals.length) {
    return;
  }

  map.invalidateSize();
  const bounds = L.latLngBounds(animals.map((animal) => [animal.lat, animal.lng]));
  map.fitBounds(bounds.pad(0.25), { maxZoom: 16 });
}

function refreshMapLayout() {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(() => {
    if (!map) {
      return;
    }

    fitMapToAnimals();
  }, 100);
}

function getStatus(animal) {
  if (animal.moving) {
    return { kind: "alert", label: "Movimiento" };
  }

  if (animal.battery <= 30) {
    return { kind: "warn", label: "Bateria baja" };
  }

  return { kind: "ok", label: "Normal" };
}

function randomOffset() {
  return (Math.random() - 0.5) * 0.002;
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}
