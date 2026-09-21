import { TruthMapRenderer } from "./truth-map-renderer.js";

const vertexShaderSource = `#version 300 es
precision highp float;
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aColor;
uniform mat4 uMvp;
out vec3 vColor;
void main() {
  gl_Position = uMvp * vec4(aPosition, 1.0);
  vColor = aColor;
}
`;

const fragmentShaderSource = `#version 300 es
precision highp float;
in vec3 vColor;
out vec4 outColor;
void main() {
  outColor = vec4(vColor, 1.0);
}
`;

function clamp01(value) {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function mixColor(a, b, amount) {
  const t = clamp01(amount);
  return [
    a[0] + (b[0] - a[0]) * t,
    a[1] + (b[1] - a[1]) * t,
    a[2] + (b[2] - a[2]) * t,
  ];
}

function terrainColor(chunk) {
  const water = chunk?.waterKind;
  if (water === "Ocean") return [0.10, 0.27, 0.36];
  if (water === "Coast") return [0.14, 0.34, 0.39];
  if (water === "Lake") return [0.15, 0.40, 0.50];
  if (water === "River" || water === "Stream" || water === "Spring") {
    return [0.18, 0.44, 0.55];
  }
  if (water === "Wetland") return [0.25, 0.42, 0.31];

  const e = clamp01(chunk?.elevation01);
  let base;
  if (e < 0.34) base = [0.16, 0.31, 0.20];
  else if (e < 0.48) base = [0.23, 0.40, 0.24];
  else if (e < 0.62) base = [0.37, 0.48, 0.27];
  else if (e < 0.76) base = [0.48, 0.48, 0.35];
  else base = [0.65, 0.64, 0.56];

  base = mixColor(base, [0.12, 0.34, 0.17], clamp01(chunk?.forestCoverage01) * 0.48);
  base = mixColor(base, [0.34, 0.50, 0.20], clamp01(chunk?.grassCoverage01) * 0.28);
  base = mixColor(base, [0.42, 0.39, 0.34], clamp01(chunk?.rockCoverage01) * 0.34);
  base = mixColor(base, [0.22, 0.40, 0.31], clamp01(chunk?.wetlandCoverage01) * 0.36);
  return base;
}

function presentationHash01(seedText, x, y, index, salt) {
  const input = `${seedText}:${x}:${y}:${index}:${salt}`;
  let hash = 2166136261 >>> 0;
  for (let i = 0; i < input.length; ++i) {
    hash ^= input.charCodeAt(i);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  hash = Math.imul(hash, 2246822519) >>> 0;
  hash ^= hash >>> 13;
  return (hash >>> 0) / 4294967295;
}

function appendCrossedBillboard(vertices, x, y, z, width, height, color) {
  const half = width * 0.5;
  const top = z + height;
  const planes = [
    [[x - half, y, z], [x + half, y, z], [x + half, y, top], [x - half, y, top]],
    [[x, y - half, z], [x, y + half, z], [x, y + half, top], [x, y - half, top]],
  ];

  for (const quad of planes) {
    appendVertex(vertices, ...quad[0], color);
    appendVertex(vertices, ...quad[1], color);
    appendVertex(vertices, ...quad[2], color);
    appendVertex(vertices, ...quad[0], color);
    appendVertex(vertices, ...quad[2], color);
    appendVertex(vertices, ...quad[3], color);
  }
}

function appendRockMarker(vertices, x, y, z, size, color) {
  const top = [x, y, z + size];
  const p0 = [x - size * 0.45, y - size * 0.35, z];
  const p1 = [x + size * 0.48, y - size * 0.30, z];
  const p2 = [x + size * 0.36, y + size * 0.42, z];
  const p3 = [x - size * 0.40, y + size * 0.34, z];
  for (const pair of [[p0, p1], [p1, p2], [p2, p3], [p3, p0]]) {
    appendVertex(vertices, ...pair[0], color);
    appendVertex(vertices, ...pair[1], color);
    appendVertex(vertices, ...top, color);
  }
}

function appendEcologyPresentation(vertices, chunks, centerX, centerY, heightScale, worldSeed) {
  for (const chunk of chunks) {
    if (chunk.waterKind === "Ocean") continue;

    const baseX = chunk.x - centerX;
    const baseY = chunk.y - centerY;
    const baseZ = (clamp01(chunk.elevation01) - 0.48) * heightScale + 0.02;

    const forest = clamp01(chunk.forestCoverage01);
    const shrubs = clamp01(chunk.shrubCoverage01);
    const rocks = clamp01(chunk.rockCoverage01);
    const wetland = clamp01(chunk.wetlandCoverage01);

    const treeCount = forest < 0.18 ? 0 : Math.min(5, 1 + Math.floor(forest * 5));
    for (let i = 0; i < treeCount; ++i) {
      const ox = (presentationHash01(worldSeed, chunk.x, chunk.y, i, "tree-x") - 0.5) * 0.72;
      const oy = (presentationHash01(worldSeed, chunk.x, chunk.y, i, "tree-y") - 0.5) * 0.72;
      const sizeJitter = 0.80 + presentationHash01(worldSeed, chunk.x, chunk.y, i, "tree-s") * 0.42;
      appendCrossedBillboard(
        vertices,
        baseX + ox,
        baseY + oy,
        baseZ,
        (0.16 + forest * 0.15) * sizeJitter,
        (0.58 + forest * 0.85) * sizeJitter,
        mixColor([0.08, 0.24, 0.10], [0.17, 0.39, 0.14], forest),
      );
    }

    const shrubCount = shrubs < 0.30 ? 0 : Math.min(3, Math.floor(shrubs * 4));
    for (let i = 0; i < shrubCount; ++i) {
      const ox = (presentationHash01(worldSeed, chunk.x, chunk.y, i, "shrub-x") - 0.5) * 0.78;
      const oy = (presentationHash01(worldSeed, chunk.x, chunk.y, i, "shrub-y") - 0.5) * 0.78;
      appendCrossedBillboard(
        vertices,
        baseX + ox,
        baseY + oy,
        baseZ,
        0.16 + shrubs * 0.12,
        0.13 + shrubs * 0.22,
        [0.24, 0.39, 0.16],
      );
    }

    const rockCount = rocks < 0.36 ? 0 : Math.min(2, Math.floor(rocks * 3));
    for (let i = 0; i < rockCount; ++i) {
      const ox = (presentationHash01(worldSeed, chunk.x, chunk.y, i, "rock-x") - 0.5) * 0.74;
      const oy = (presentationHash01(worldSeed, chunk.x, chunk.y, i, "rock-y") - 0.5) * 0.74;
      appendRockMarker(
        vertices,
        baseX + ox,
        baseY + oy,
        baseZ,
        0.15 + rocks * 0.18,
        [0.43, 0.43, 0.39],
      );
    }

    if (wetland >= 0.46 && chunk.waterKind === "None") {
      appendCrossedBillboard(
        vertices,
        baseX,
        baseY,
        baseZ,
        0.46,
        0.05 + wetland * 0.08,
        [0.22, 0.42, 0.31],
      );
    }
  }
}

function compileShader(gl, type, source) {
  const shader = gl.createShader(type);
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    const message = gl.getShaderInfoLog(shader) || "unknown shader error";
    gl.deleteShader(shader);
    throw new Error(message);
  }
  return shader;
}

function createProgram(gl) {
  const vertex = compileShader(gl, gl.VERTEX_SHADER, vertexShaderSource);
  const fragment = compileShader(gl, gl.FRAGMENT_SHADER, fragmentShaderSource);
  const program = gl.createProgram();
  gl.attachShader(program, vertex);
  gl.attachShader(program, fragment);
  gl.linkProgram(program);
  gl.deleteShader(vertex);
  gl.deleteShader(fragment);
  if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    const message = gl.getProgramInfoLog(program) || "unknown link error";
    gl.deleteProgram(program);
    throw new Error(message);
  }
  return program;
}

function multiply4(a, b) {
  const out = new Float32Array(16);
  for (let column = 0; column < 4; column += 1) {
    for (let row = 0; row < 4; row += 1) {
      out[column * 4 + row] =
        a[0 * 4 + row] * b[column * 4 + 0] +
        a[1 * 4 + row] * b[column * 4 + 1] +
        a[2 * 4 + row] * b[column * 4 + 2] +
        a[3 * 4 + row] * b[column * 4 + 3];
    }
  }
  return out;
}

function perspective(fovY, aspect, near, far) {
  const f = 1 / Math.tan(fovY / 2);
  const nf = 1 / (near - far);
  return new Float32Array([
    f / aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (far + near) * nf, -1,
    0, 0, 2 * far * near * nf, 0,
  ]);
}

function normalize3(v) {
  const length = Math.hypot(v[0], v[1], v[2]) || 1;
  return [v[0] / length, v[1] / length, v[2] / length];
}

function cross3(a, b) {
  return [
    a[1] * b[2] - a[2] * b[1],
    a[2] * b[0] - a[0] * b[2],
    a[0] * b[1] - a[1] * b[0],
  ];
}

function dot3(a, b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

function lookAt(eye, target, up) {
  const z = normalize3([
    eye[0] - target[0],
    eye[1] - target[1],
    eye[2] - target[2],
  ]);
  const x = normalize3(cross3(up, z));
  const y = cross3(z, x);

  return new Float32Array([
    x[0], y[0], z[0], 0,
    x[1], y[1], z[1], 0,
    x[2], y[2], z[2], 0,
    -dot3(x, eye), -dot3(y, eye), -dot3(z, eye), 1,
  ]);
}

function appendVertex(array, x, y, z, color) {
  array.push(x, y, z, color[0], color[1], color[2]);
}

export class WorldSurfaceRenderer {
  constructor(canvas) {
    this.canvas = canvas;
    this.fallback = null;
    this.data = null;
    this.yaw = -0.72;
    this.pitch = 0.78;
    this.distance = 25;
    this.minDistance = 5;
    this.maxDistance = 90;
    this.drag = null;
    this.pointers = new Map();
    this.pinchDistance = null;
    this.vertexCount = 0;
    this.worldExtent = 10;

    const gl = canvas.getContext("webgl2", {
      antialias: true,
      alpha: false,
      powerPreference: "high-performance",
    });

    if (!gl) {
      this.fallback = new TruthMapRenderer(canvas);
      this.mode = "2D truth fallback";
      return;
    }

    this.gl = gl;
    this.program = createProgram(gl);
    this.mvpLocation = gl.getUniformLocation(this.program, "uMvp");
    this.buffer = gl.createBuffer();
    this.vao = gl.createVertexArray();

    gl.bindVertexArray(this.vao);
    gl.bindBuffer(gl.ARRAY_BUFFER, this.buffer);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 24, 0);
    gl.enableVertexAttribArray(1);
    gl.vertexAttribPointer(1, 3, gl.FLOAT, false, 24, 12);
    gl.bindVertexArray(null);

    gl.enable(gl.DEPTH_TEST);

    this.mode = "WebGL2 3D terrain + ecology truth";
    this.resizeObserver = new ResizeObserver(() => this.resize());
    this.resizeObserver.observe(canvas);
    this.bindInput();
    this.resize();
  }

  bindInput() {
    const pointerDistance = () => {
      const points = [...this.pointers.values()];
      if (points.length < 2) return null;
      return Math.hypot(
        points[0].x - points[1].x,
        points[0].y - points[1].y,
      );
    };

    this.canvas.addEventListener("pointerdown", (event) => {
      this.canvas.setPointerCapture(event.pointerId);
      this.pointers.set(event.pointerId, {
        x: event.clientX,
        y: event.clientY,
      });
      if (this.pointers.size === 1) {
        this.drag = { x: event.clientX, y: event.clientY };
      } else {
        this.drag = null;
        this.pinchDistance = pointerDistance();
      }
    });

    this.canvas.addEventListener("pointermove", (event) => {
      if (!this.pointers.has(event.pointerId)) return;
      this.pointers.set(event.pointerId, {
        x: event.clientX,
        y: event.clientY,
      });

      if (this.pointers.size >= 2) {
        const nextDistance = pointerDistance();
        if (nextDistance && this.pinchDistance) {
          const scale = this.pinchDistance / nextDistance;
          this.distance = Math.max(
            this.minDistance,
            Math.min(this.maxDistance, this.distance * scale),
          );
        }
        this.pinchDistance = nextDistance;
        this.draw();
        return;
      }

      if (!this.drag) {
        this.drag = { x: event.clientX, y: event.clientY };
        return;
      }

      const dx = event.clientX - this.drag.x;
      const dy = event.clientY - this.drag.y;
      this.drag = { x: event.clientX, y: event.clientY };
      this.yaw -= dx * 0.007;
      this.pitch = Math.max(0.20, Math.min(1.38, this.pitch + dy * 0.006));
      this.draw();
    });

    const stopPointer = (event) => {
      this.pointers.delete(event.pointerId);
      this.pinchDistance = pointerDistance();
      const remaining = [...this.pointers.values()];
      this.drag = remaining.length === 1 ? { ...remaining[0] } : null;
    };
    this.canvas.addEventListener("pointerup", stopPointer);
    this.canvas.addEventListener("pointercancel", stopPointer);

    this.canvas.addEventListener(
      "wheel",
      (event) => {
        event.preventDefault();
        const scale = Math.exp(event.deltaY * 0.001);
        this.distance = Math.max(
          this.minDistance,
          Math.min(this.maxDistance, this.distance * scale),
        );
        this.draw();
      },
      { passive: false },
    );

    this.canvas.addEventListener("dblclick", () => {
      this.yaw = -0.72;
      this.pitch = 0.78;
      this.distance = Math.max(10, this.worldExtent * 1.7);
      this.draw();
    });
  }

  resize() {
    if (this.fallback) {
      this.fallback.resize();
      return;
    }

    const rect = this.canvas.getBoundingClientRect();
    const scale = Math.min(window.devicePixelRatio || 1, 2);
    const width = Math.max(1, Math.round(rect.width * scale));
    const height = Math.max(1, Math.round(rect.height * scale));
    if (this.canvas.width !== width || this.canvas.height !== height) {
      this.canvas.width = width;
      this.canvas.height = height;
    }
    this.draw();
  }

  render(data) {
    if (this.fallback) {
      this.fallback.render(data);
      return;
    }

    this.data = data;
    if (!data?.available || !Array.isArray(data.chunks) || data.chunks.length < 4) {
      this.vertexCount = 0;
      this.draw();
      return;
    }

    const byCoord = new Map();
    let minX = Infinity;
    let maxX = -Infinity;
    let minY = Infinity;
    let maxY = -Infinity;

    for (const chunk of data.chunks) {
      byCoord.set(`${chunk.x},${chunk.y}`, chunk);
      minX = Math.min(minX, chunk.x);
      maxX = Math.max(maxX, chunk.x);
      minY = Math.min(minY, chunk.y);
      maxY = Math.max(maxY, chunk.y);
    }

    const centerX = (minX + maxX) * 0.5;
    const centerY = (minY + maxY) * 0.5;
    const vertices = [];
    const heightScale = Math.max(4, Math.max(maxX - minX, maxY - minY) * 0.48);

    const point = (chunk) => [
      chunk.x - centerX,
      chunk.y - centerY,
      (clamp01(chunk.elevation01) - 0.48) * heightScale,
    ];

    for (let y = minY; y < maxY; y += 1) {
      for (let x = minX; x < maxX; x += 1) {
        const a = byCoord.get(`${x},${y}`);
        const b = byCoord.get(`${x + 1},${y}`);
        const c = byCoord.get(`${x + 1},${y + 1}`);
        const d = byCoord.get(`${x},${y + 1}`);
        if (!a || !b || !c || !d) continue;

        const pa = point(a);
        const pb = point(b);
        const pc = point(c);
        const pd = point(d);

        appendVertex(vertices, ...pa, terrainColor(a));
        appendVertex(vertices, ...pb, terrainColor(b));
        appendVertex(vertices, ...pc, terrainColor(c));

        appendVertex(vertices, ...pa, terrainColor(a));
        appendVertex(vertices, ...pc, terrainColor(c));
        appendVertex(vertices, ...pd, terrainColor(d));
      }
    }

    appendEcologyPresentation(
      vertices,
      data.chunks,
      centerX,
      centerY,
      heightScale,
      String(data.worldSeed ?? "0"),
    );

    this.worldExtent = Math.max(6, Math.max(maxX - minX, maxY - minY));
    this.minDistance = Math.max(4, this.worldExtent * 0.55);
    this.maxDistance = Math.max(30, this.worldExtent * 5.5);
    if (!Number.isFinite(this.distance) || this.distance < this.minDistance) {
      this.distance = this.worldExtent * 1.7;
    }

    const gl = this.gl;
    gl.bindBuffer(gl.ARRAY_BUFFER, this.buffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.DYNAMIC_DRAW);
    this.vertexCount = vertices.length / 6;
    this.draw();
  }

  draw() {
    if (this.fallback) return;

    const gl = this.gl;
    const width = Math.max(1, this.canvas.width);
    const height = Math.max(1, this.canvas.height);
    gl.viewport(0, 0, width, height);
    gl.clearColor(0.035, 0.055, 0.040, 1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    if (!this.vertexCount) return;

    const horizontal = Math.cos(this.pitch) * this.distance;
    const eye = [
      Math.cos(this.yaw) * horizontal,
      Math.sin(this.yaw) * horizontal,
      Math.sin(this.pitch) * this.distance,
    ];
    const projection = perspective(
      Math.PI / 3,
      width / height,
      0.1,
      this.maxDistance * 4,
    );
    const view = lookAt(eye, [0, 0, 0], [0, 0, 1]);
    const mvp = multiply4(projection, view);

    gl.useProgram(this.program);
    gl.uniformMatrix4fv(this.mvpLocation, false, mvp);
    gl.bindVertexArray(this.vao);
    gl.drawArrays(gl.TRIANGLES, 0, this.vertexCount);
    gl.bindVertexArray(null);
  }
}
