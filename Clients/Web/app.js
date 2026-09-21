import { LifeLensCoreClient } from "./core-adapter.js";
import { WorldSurfaceRenderer } from "./world-surface-renderer.js";

const core = new LifeLensCoreClient();
const canvas = document.querySelector("#worldCanvas");
const renderer = new WorldSurfaceRenderer(canvas);

const ui = {
  status: document.querySelector("#coreStatus"),
  unavailable: document.querySelector("#unavailable"),
  seedInput: document.querySelector("#seedInput"),
  newWorld: document.querySelector("#newWorldButton"),
  step: document.querySelector("#stepButton"),
  hour: document.querySelector("#hourButton"),
  living: document.querySelector("#livingMetric"),
  households: document.querySelector("#householdMetric"),
  couples: document.querySelector("#coupleMetric"),
  events: document.querySelector("#eventMetric"),
  residents: document.querySelector("#residentList"),
  residentCount: document.querySelector("#residentCount"),
  seedLabel: document.querySelector("#worldSeedLabel"),
  minuteLabel: document.querySelector("#minuteLabel"),
  rendererLabel: document.querySelector("#rendererLabel"),
  chunkLabel: document.querySelector("#chunkLabel"),
  left: document.querySelector("#leftButton"),
  right: document.querySelector("#rightButton"),
  up: document.querySelector("#upButton"),
  down: document.querySelector("#downButton"),
};

let centerChunkX = 0;
let centerChunkY = 0;

function setStatus(text, kind) {
  ui.status.textContent = text;
  ui.status.className = `status ${kind}`;
}

function setControls(enabled) {
  for (const button of [
    ui.step,
    ui.hour,
    ui.left,
    ui.right,
    ui.up,
    ui.down,
  ]) {
    button.disabled = !enabled;
  }
}

function dayTimeLabel(minute) {
  const total = Math.max(0, Number(minute) || 0);
  const day = Math.floor(total / 1440) + 1;
  const clock = total % 1440;
  const hour = String(Math.floor(clock / 60)).padStart(2, "0");
  const min = String(clock % 60).padStart(2, "0");
  return `Day ${day} · ${hour}:${min}`;
}

function needPercent(value) {
  return `${Math.round(Math.max(0, Math.min(1, Number(value) || 0)) * 100)}%`;
}

function renderResidents(data) {
  const residents = data?.residents || [];
  ui.residentCount.textContent = String(residents.length);

  if (!residents.length) {
    ui.residents.innerHTML = '<div class="empty">표시할 주민이 없습니다.</div>';
    return;
  }

  ui.residents.innerHTML = residents
    .map(
      (resident) => `
        <article class="resident-card">
          <div class="resident-title">
            <strong>${escapeHtml(resident.name)}</strong>
            <span>${escapeHtml(resident.activityLabel || "Idle")}</span>
          </div>
          <div class="need-grid">
            <span>배고픔 <b>${needPercent(resident.needs?.hunger)}</b></span>
            <span>갈증 <b>${needPercent(resident.needs?.thirst)}</b></span>
            <span>수면 <b>${needPercent(resident.needs?.sleep)}</b></span>
            <span>위생 <b>${needPercent(resident.needs?.hygiene)}</b></span>
          </div>
          <small>${resident.hasPosition ? `Grid ${resident.gridX}, ${resident.gridY}` : "Position unavailable"}</small>
        </article>
      `,
    )
    .join("");
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

function refresh() {
  if (!core.available) return;

  const overview = core.worldOverview();
  const residents = core.residents();
  const terrain = core.terrainWindow(centerChunkX, centerChunkY, 8);

  ui.seedLabel.textContent = overview.available
    ? `WorldSeed ${overview.worldSeed}`
    : "WorldSeed —";
  ui.minuteLabel.textContent = overview.available
    ? dayTimeLabel(overview.minute)
    : "Day —";
  ui.living.textContent = overview.livingResidents ?? "—";
  ui.households.textContent = overview.households ?? "—";
  ui.couples.textContent = overview.activeCouples ?? "—";
  ui.events.textContent = overview.majorLifeEvents ?? "—";
  ui.chunkLabel.textContent = `Chunk ${centerChunkX}, ${centerChunkY}`;

  renderResidents(residents);
  renderer.render(terrain);
}

function newWorld() {
  if (!core.available) return;
  const seed = ui.seedInput.value.trim() || "1";
  core.newGame(seed, "", 2);
  centerChunkX = 0;
  centerChunkY = 0;
  refresh();
}

function move(dx, dy) {
  centerChunkX += dx;
  centerChunkY += dy;
  refresh();
}

ui.newWorld.addEventListener("click", newWorld);
ui.step.addEventListener("click", () => {
  core.runMinutes(10);
  refresh();
});
ui.hour.addEventListener("click", () => {
  core.runMinutes(60);
  refresh();
});
ui.left.addEventListener("click", () => move(-1, 0));
ui.right.addEventListener("click", () => move(1, 0));
ui.up.addEventListener("click", () => move(0, 1));
ui.down.addEventListener("click", () => move(0, -1));

async function boot() {
  if ("serviceWorker" in navigator) {
    navigator.serviceWorker.register("./sw.js").catch(() => {});
  }

  ui.rendererLabel.textContent = renderer.mode;

  try {
    await core.load();
    setStatus("Core WASM ready", "ready");
    ui.unavailable.classList.add("hidden");
    setControls(true);
    newWorld();
  } catch (error) {
    console.warn("LifeLensCore WASM unavailable:", error);
    setStatus("Core WASM not built", "error");
    ui.unavailable.classList.remove("hidden");
    setControls(false);
    renderer.render(null);
  }
}

boot();
