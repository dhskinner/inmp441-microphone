#pragma once

namespace WebUi
{

    inline constexpr char PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>ESP32 I2S Spectrogram</title>
  <style>
    :root {
      --bg0: #07131a;
      --bg1: #102936;
      --ink: #dff7ff;
      --ink-dim: #9ec9d8;
      --card: rgba(7, 19, 26, 0.7);
      --line: rgba(180, 230, 255, 0.2);
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      min-height: 100dvh;
      color: var(--ink);
      font-family: "Avenir Next", "Segoe UI", sans-serif;
      background:
        radial-gradient(1200px 700px at 10% -10%, #1f5970 0%, transparent 60%),
        radial-gradient(1000px 700px at 110% 0%, #2a7a6f 0%, transparent 55%),
        linear-gradient(150deg, var(--bg0), var(--bg1));
      display: grid;
      place-items: center;
      padding: 16px;
    }

    .panel {
      width: min(1080px, 100%);
      background: var(--card);
      border: 1px solid var(--line);
      border-radius: 16px;
      padding: 14px;
      backdrop-filter: blur(4px);
      box-shadow: 0 10px 36px rgba(0, 0, 0, 0.35);
      animation: panelIn 500ms ease-out;
    }

    .head {
      display: flex;
      align-items: baseline;
      justify-content: space-between;
      gap: 10px;
      margin: 2px 2px 12px;
    }

    h1 {
      font-size: clamp(1rem, 2vw, 1.35rem);
      margin: 0;
      font-weight: 700;
      letter-spacing: 0.02em;
    }

    .meta {
      color: var(--ink-dim);
      font-size: 0.86rem;
      white-space: nowrap;
    }

    canvas {
      width: 100%;
      height: min(72vh, 480px);
      display: block;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: rgba(4, 12, 17, 0.9);
      image-rendering: pixelated;
    }

    .chart {
      display: grid;
      grid-template-columns: 1fr 52px;
      gap: 10px;
      align-items: stretch;
    }

    .plot {
      display: grid;
      grid-template-columns: 26px 1fr;
      grid-template-rows: 1fr auto;
      gap: 6px 8px;
      min-height: 220px;
    }

    .y-label {
      grid-row: 1;
      grid-column: 1;
      writing-mode: vertical-rl;
      transform: rotate(180deg);
      color: var(--ink-dim);
      font-size: 0.78rem;
      letter-spacing: 0.02em;
      align-self: center;
      text-align: center;
    }

    .canvas-wrap {
      grid-row: 1;
      grid-column: 2;
      position: relative;
    }

    .y-ticks {
      position: absolute;
      inset: 0;
      pointer-events: none;
      color: rgba(220, 246, 255, 0.8);
      font-size: 0.72rem;
    }

    .y-ticks span {
      position: absolute;
      right: 6px;
      transform: translateY(50%);
      text-shadow: 0 1px 2px rgba(0, 0, 0, 0.6);
    }

    .x-axis {
      grid-row: 2;
      grid-column: 2;
      display: flex;
      justify-content: space-between;
      color: var(--ink-dim);
      font-size: 0.74rem;
      line-height: 1.2;
    }

    .x-axis .x-title {
      color: var(--ink);
      opacity: 0.92;
    }

    .legend {
      border: 1px solid var(--line);
      border-radius: 10px;
      background: rgba(3, 9, 13, 0.55);
      padding: 8px 6px;
      display: grid;
      grid-template-columns: 1fr auto;
      column-gap: 6px;
      align-items: stretch;
      min-height: 220px;
    }

    .legend-bar {
      border-radius: 6px;
      background: linear-gradient(to top, rgb(0, 0, 204) 0%, rgb(60, 80, 180) 32%, rgb(190, 120, 30) 72%, rgb(255, 255, 0) 100%);
      border: 1px solid rgba(180, 230, 255, 0.2);
      min-width: 16px;
    }

    .legend-labels {
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      font-size: 0.72rem;
      color: var(--ink-dim);
      text-align: right;
    }

    .legend-title {
      grid-column: 1 / span 2;
      font-size: 0.72rem;
      color: var(--ink);
      opacity: 0.92;
      margin-bottom: 6px;
      text-align: center;
    }

    @media (max-width: 760px) {
      .chart {
        grid-template-columns: 1fr;
      }

      .legend {
        min-height: 120px;
        grid-template-columns: 1fr;
        row-gap: 8px;
      }

      .legend-bar {
        min-height: 18px;
        min-width: 0;
        background: linear-gradient(to right, rgb(0, 0, 204) 0%, rgb(60, 80, 180) 32%, rgb(190, 120, 30) 72%, rgb(255, 255, 0) 100%);
      }

      .legend-labels {
        flex-direction: row;
      }
    }

    .foot {
      margin-top: 10px;
      color: var(--ink-dim);
      font-size: 0.8rem;
      display: flex;
      justify-content: space-between;
      gap: 8px;
      flex-wrap: wrap;
    }

    @keyframes panelIn {
      from { transform: translateY(10px); opacity: 0; }
      to { transform: translateY(0); opacity: 1; }
    }
  </style>
</head>
<body>
  <section class="panel">
    <div class="head">
      <h1>ESP32-S3 Realtime Spectrogram</h1>
      <div id="status" class="meta">Connecting...</div>
    </div>
    <div class="chart">
      <div class="plot">
        <div class="y-label">Frequency (Hz)</div>
        <div class="canvas-wrap">
          <canvas id="spectrogram" width="640" height="64"></canvas>
          <div id="yTicks" class="y-ticks" aria-hidden="true">
          </div>
        </div>
        <div class="x-axis">
          <span>-51 s</span>
          <span>-38 s</span>
          <span class="x-title">Time (s, newest at right)</span>
          <span>-13 s</span>
          <span>Now</span>
        </div>
      </div>
      <div class="legend">
        <div class="legend-title">Intensity (dBFS)</div>
        <div class="legend-bar" aria-hidden="true"></div>
        <div class="legend-labels">
          <span>-20</span>
          <span>-55</span>
          <span>-90</span>
        </div>
      </div>
    </div>
    <div class="foot">
      <span id="fps">FPS: --</span>
      <span>Band: 80 Hz to 8 kHz (log scale)</span>
      <span>Each vertical column is one FFT frame</span>
    </div>
  </section>

  <script>
    const statusEl = document.getElementById('status');
    const fpsEl = document.getElementById('fps');
    const yTicksEl = document.getElementById('yTicks');
    const canvas = document.getElementById('spectrogram');
    const ctx = canvas.getContext('2d', { alpha: false });
    const h = canvas.height;
    const w = canvas.width;
    const minFreqHz = 80;
    const maxFreqHz = 8000;
    const tickFreqs = [80, 125, 250, 500, 1000, 2000, 4000, 8000];
    let frames = 0;
    let lastFpsMs = performance.now();
    let latestBins = null;

    function formatHz(v) {
      if (v >= 1000) return `${(v / 1000).toFixed(v % 1000 === 0 ? 0 : 1)} k`;
      return `${v}`;
    }

    function renderFrequencyTicks() {
      const logMin = Math.log10(minFreqHz);
      const logMax = Math.log10(maxFreqHz);
      const span = logMax - logMin;

      yTicksEl.innerHTML = '';
      tickFreqs.forEach((f) => {
        const p = ((Math.log10(f) - logMin) / span) * 100;
        const tick = document.createElement('span');
        tick.style.bottom = `${Math.max(0, Math.min(100, p))}%`;
        tick.textContent = formatHz(f);
        yTicksEl.appendChild(tick);
      });
    }

    function palette(v) {
      const t = v / 255;
      const r = Math.min(255, Math.max(0, 255 * Math.pow(t, 1.2)));
      const g = Math.min(255, Math.max(0, 255 * Math.pow(t, 2.0)));
      const b = Math.min(255, Math.max(0, 255 * (1.0 - t) * 0.8));
      return [r | 0, g | 0, b | 0];
    }

    function drawColumn(values) {
      ctx.drawImage(canvas, -1, 0);
      for (let y = 0; y < h; y++) {
        const idx = h - 1 - y;
        const [r, g, b] = palette(values[idx] || 0);
        ctx.fillStyle = `rgb(${r},${g},${b})`;
        ctx.fillRect(w - 1, y, 1, 1);
      }

      frames++;
      const now = performance.now();
      if (now - lastFpsMs >= 1000) {
        fpsEl.textContent = `FPS: ${frames}`;
        frames = 0;
        lastFpsMs = now;
      }
    }

    function renderLoop() {
      if (latestBins) {
        drawColumn(latestBins);
        latestBins = null;
      }
      requestAnimationFrame(renderLoop);
    }

    function connect() {
      const ws = new WebSocket(`ws://${location.host}/ws`);
      ws.binaryType = 'arraybuffer';

      ws.onopen = () => statusEl.textContent = 'Connected';
      ws.onclose = () => {
        statusEl.textContent = 'Disconnected, retrying...';
        setTimeout(connect, 1200);
      };
      ws.onerror = () => statusEl.textContent = 'Socket error, reconnecting...';

      ws.onmessage = (evt) => {
        if (!(evt.data instanceof ArrayBuffer)) return;
        const bins = new Uint8Array(evt.data);
        if (bins.length !== h) return;
        latestBins = bins;
      };
    }

    ctx.fillStyle = 'rgb(0,0,0)';
    ctx.fillRect(0, 0, w, h);
    renderFrequencyTicks();
    requestAnimationFrame(renderLoop);
    connect();
  </script>
</body>
</html>
)HTML";

} // namespace WebUi
