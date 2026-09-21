function elevationColor(value) {
  const e = Math.max(0, Math.min(1, Number(value) || 0));
  if (e < 0.34) return [36, 62, 50];
  if (e < 0.48) return [62, 91, 61];
  if (e < 0.62) return [86, 106, 69];
  if (e < 0.76) return [105, 105, 82];
  return [132, 129, 112];
}

function waterColor(kind) {
  switch (kind) {
    case "Ocean": return "rgb(31 69 89)";
    case "Coast": return "rgb(41 89 101)";
    case "Lake": return "rgb(48 105 126)";
    case "River": return "rgb(55 119 142)";
    case "Stream": return "rgb(66 127 145)";
    case "Wetland": return "rgb(66 104 91)";
    case "Spring": return "rgb(71 133 147)";
    default: return null;
  }
}

export class TruthMapRenderer {
  constructor(canvas) {
    this.canvas = canvas;
    this.context = canvas.getContext("2d");
    this.resizeObserver = new ResizeObserver(() => this.resize());
    this.resizeObserver.observe(canvas);
    this.data = null;
    this.resize();
  }

  resize() {
    const rect = this.canvas.getBoundingClientRect();
    const scale = Math.min(window.devicePixelRatio || 1, 2);
    this.canvas.width = Math.max(1, Math.round(rect.width * scale));
    this.canvas.height = Math.max(1, Math.round(rect.height * scale));
    this.render(this.data);
  }

  render(data) {
    this.data = data;
    const ctx = this.context;
    const width = this.canvas.width;
    const height = this.canvas.height;

    ctx.clearRect(0, 0, width, height);
    ctx.fillStyle = "rgb(12 18 14)";
    ctx.fillRect(0, 0, width, height);

    if (!data?.available || !Array.isArray(data.chunks) || data.chunks.length === 0) {
      ctx.fillStyle = "rgb(135 151 139)";
      ctx.font = `${Math.max(14, width / 55)}px system-ui`;
      ctx.textAlign = "center";
      ctx.fillText("LifeLensCore world truth", width / 2, height / 2);
      return;
    }

    const xs = data.chunks.map((c) => c.x);
    const ys = data.chunks.map((c) => c.y);
    const minX = Math.min(...xs);
    const maxX = Math.max(...xs);
    const minY = Math.min(...ys);
    const maxY = Math.max(...ys);
    const cols = maxX - minX + 1;
    const rows = maxY - minY + 1;

    const cellW = width / cols;
    const cellH = height / rows;

    for (const chunk of data.chunks) {
      const x = (chunk.x - minX) * cellW;
      const y = (maxY - chunk.y) * cellH;
      const [r, g, b] = elevationColor(chunk.elevation01);
      ctx.fillStyle = `rgb(${r} ${g} ${b})`;
      ctx.fillRect(x, y, Math.ceil(cellW) + 1, Math.ceil(cellH) + 1);

      const water = waterColor(chunk.waterKind);
      if (water) {
        ctx.globalAlpha = chunk.waterKind === "Ocean" ? 0.88 : 0.72;
        ctx.fillStyle = water;
        ctx.fillRect(x, y, Math.ceil(cellW) + 1, Math.ceil(cellH) + 1);
        ctx.globalAlpha = 1;
      }
    }

    const centerX = (data.centerChunkX - minX + 0.5) * cellW;
    const centerY = (maxY - data.centerChunkY + 0.5) * cellH;
    ctx.strokeStyle = "rgba(235,245,231,.92)";
    ctx.lineWidth = Math.max(2, width / 500);
    ctx.beginPath();
    ctx.arc(centerX, centerY, Math.max(5, Math.min(cellW, cellH) * 0.18), 0, Math.PI * 2);
    ctx.stroke();
  }
}
