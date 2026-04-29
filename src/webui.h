#pragma once
#include "settings.h"

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
      padding: 12px;
      backdrop-filter: blur(4px);
      box-shadow: 0 10px 36px rgba(0, 0, 0, 0.35);
      animation: panelIn 500ms ease-out;
    }

    .head {
      display: flex;
      align-items: baseline;
      justify-content: space-between;
      gap: 10px;
      margin: 2px 2px 8px;
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

    .spec-canvas {
      width: 100%;
      height: clamp(170px, 29vh, 260px);
      display: block;
      border-radius: 10px;
      border: 1px solid var(--line);
      background: rgba(4, 12, 17, 0.9);
      image-rendering: pixelated;
    }

    .vu-block {
      margin-top: 10px;
      border: 1px solid var(--line);
      border-radius: 10px;
      background: rgba(3, 9, 13, 0.55);
      padding: 8px;
    }

    .vu-head {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
      color: var(--ink-dim);
      font-size: 0.74rem;
      margin-bottom: 6px;
    }

    .vu-head .title {
      color: var(--ink);
      opacity: 0.92;
      font-weight: 600;
      letter-spacing: 0.02em;
    }

    .vu-canvas {
      width: 100%;
      height: clamp(84px, 14.5vh, 130px);
      display: block;
      border-radius: 8px;
      border: 1px solid var(--line);
      background: rgba(4, 12, 17, 0.9);
      image-rendering: pixelated;
    }

    .vu-axis {
      margin-top: 6px;
      display: flex;
      justify-content: space-between;
      color: var(--ink-dim);
      font-size: 0.74rem;
    }

    .wave-block {
      margin-top: 10px;
      border: 1px solid var(--line);
      border-radius: 10px;
      background: rgba(3, 9, 13, 0.55);
      padding: 8px;
    }

    .wave-head {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
      color: var(--ink-dim);
      font-size: 0.74rem;
      margin-bottom: 6px;
    }

    .wave-head .title {
      color: var(--ink);
      opacity: 0.92;
      font-weight: 600;
      letter-spacing: 0.02em;
    }

    .wave-canvas {
      width: 100%;
      height: clamp(84px, 14.5vh, 130px);
      display: block;
      border-radius: 8px;
      border: 1px solid var(--line);
      background: rgba(4, 12, 17, 0.9);
      image-rendering: pixelated;
    }

    .wave-axis {
      margin-top: 6px;
      display: flex;
      justify-content: space-between;
      color: var(--ink-dim);
      font-size: 0.74rem;
    }

    .chart {
      display: grid;
      grid-template-columns: 1fr;
      gap: 8px;
      align-items: stretch;
      margin-top: 10px;
    }

    .plot {
      display: grid;
      grid-template-columns: 26px 1fr;
      grid-template-rows: 1fr auto;
      gap: 6px 8px;
      min-height: 0;
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
      padding: 8px;
      display: grid;
      grid-template-columns: 1fr;
      row-gap: 6px;
      align-items: stretch;
      min-height: 0;
    }

    .legend-bar {
      border-radius: 6px;
      background: linear-gradient(to right, rgb(0, 0, 204) 0%, rgb(60, 80, 180) 32%, rgb(190, 120, 30) 72%, rgb(255, 255, 0) 100%);
      border: 1px solid rgba(180, 230, 255, 0.2);
      min-height: 14px;
      min-width: 0;
    }

    .legend-labels {
      display: flex;
      flex-direction: row;
      justify-content: space-between;
      font-size: 0.72rem;
      color: var(--ink-dim);
      text-align: left;
    }

    .legend-title {
      font-size: 0.72rem;
      color: var(--ink);
      opacity: 0.92;
      margin-bottom: 0;
      text-align: left;
    }

    @media (max-width: 760px) {
      .spec-canvas {
        height: clamp(160px, 34vh, 240px);
      }

      .wave-canvas,
      .vu-canvas {
        height: clamp(80px, 16vh, 118px);
      }
    }

    .foot {
      margin-top: 8px;
      color: var(--ink-dim);
      font-size: 0.8rem;
      display: flex;
      justify-content: space-between;
      gap: 8px;
      flex-wrap: wrap;
    }

    .controls {
      width: 100%;
      display: flex;
      align-items: center;
      gap: 10px;
      color: var(--ink-dim);
      font-size: 0.8rem;
    }

    .controls input[type="range"] {
      width: min(360px, 100%);
      accent-color: #f6d64a;
    }

    .controls .value {
      color: var(--ink);
      min-width: 52px;
      text-align: right;
      font-variant-numeric: tabular-nums;
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
      <h1>Realtime Spectrogram</h1>
      <div id="status" class="meta">Connecting...</div>
    </div>
    <div class="wave-block">
      <div class="wave-head">
        <span class="title">Waveform</span>
        <span>Downsampled PCM</span>
      </div>
      <canvas id="waveform" class="wave-canvas" width="640" height="128"></canvas>
      <div class="wave-axis">
        <span id="waveT0">-12.8 s</span>
        <span id="waveT1">-9.6 s</span>
        <span>Time (s, newest at right)</span>
        <span id="waveT2">-3.2 s</span>
        <span>Now</span>
      </div>
    </div>
    <div class="chart">
      <div class="plot">
        <div class="y-label">Frequency (Hz)</div>
        <div class="canvas-wrap">
          <canvas id="spectrogram" class="spec-canvas" width="640" height="64"></canvas>
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
          <span id="legendTop">-20</span>
          <span id="legendMid">-55</span>
          <span id="legendBottom">-90</span>
        </div>
      </div>
    </div>
    <div class="vu-block">
      <div class="vu-head">
        <span class="title">Spectrum VU Meter</span>
        <span>Peak hold</span>
      </div>
      <canvas id="vuMeter" class="vu-canvas" width="640" height="120"></canvas>
      <div class="vu-axis">
        <span id="vuMin">80 Hz</span>
        <span id="vuMid">1.2 kHz</span>
        <span id="vuMax">8.0 kHz</span>
      </div>
    </div>
    <div class="foot">
      <span id="fps">FPS: --</span>
      <span id="bandLabel">Band: --</span>
      <span>Each vertical column is one FFT frame</span>
      <div class="controls">
        <label for="rangeDb">Dynamic range</label>
        <input id="rangeDb" type="range" min="35" max="70" step="1" value="70" />
        <span id="rangeDbValue" class="value">70 dB</span>
      </div>
      <div class="controls">
        <label for="waveGain">Wave gain</label>
        <input id="waveGain" type="range" min="1.0" max="4.0" step="0.1" value="2.6" />
        <span id="waveGainValue" class="value">2.6x</span>
      </div>
    </div>
  </section>

  <script>
    const statusEl = document.getElementById('status');
    const fpsEl = document.getElementById('fps');
    const bandLabelEl = document.getElementById('bandLabel');
    const yTicksEl = document.getElementById('yTicks');
    const rangeDbEl = document.getElementById('rangeDb');
    const rangeDbValueEl = document.getElementById('rangeDbValue');
    const waveGainEl = document.getElementById('waveGain');
    const waveGainValueEl = document.getElementById('waveGainValue');
    const vuMinEl = document.getElementById('vuMin');
    const vuMidEl = document.getElementById('vuMid');
    const vuMaxEl = document.getElementById('vuMax');
    const waveT0El = document.getElementById('waveT0');
    const waveT1El = document.getElementById('waveT1');
    const waveT2El = document.getElementById('waveT2');
    const legendTopEl = document.getElementById('legendTop');
    const legendMidEl = document.getElementById('legendMid');
    const legendBottomEl = document.getElementById('legendBottom');
    const canvas = document.getElementById('spectrogram');
    const ctx = canvas.getContext('2d', { alpha: false });
    const vuCanvas = document.getElementById('vuMeter');
    const vuCtx = vuCanvas.getContext('2d', { alpha: false });
    const waveCanvas = document.getElementById('waveform');
    const waveCtx = waveCanvas.getContext('2d', { alpha: false });
    const h = canvas.height;
    const w = canvas.width;
    const vuH = vuCanvas.height;
    const vuW = vuCanvas.width;
    const waveH = waveCanvas.height;
    const waveW = waveCanvas.width;
    const RANGE_DB_STORAGE_KEY = 'specDisplayRangeDb';
    const WAVE_GAIN_STORAGE_KEY = 'specWaveGain';
    const MIN_DBFS = -90;
    const MAX_DBFS = -20;
    const PEAK_ATTACK = 0.35;
    const PEAK_RELEASE = 0.06;
    const WAVE_STRIP_WIDTH = 1;
    let minFreqHz = 80;
    let maxFreqHz = 8000;
    let waveformBins = 128;
    let wsFrameBytes = h + waveformBins * 2;
    let wsPushIntervalMs = 80;
    let tickFreqs = [80, 125, 250, 500, 1000, 2000, 4000, 8000];
    let frames = 0;
    let lastFpsMs = performance.now();
    let latestBins = null;
    let latestWavePcm = null;
    let displayTopDb = MAX_DBFS;
    let displayRangeDb = 70;
    let waveVerticalGain = 2.6;
    let vuLevels = new Float32Array(h);
    let vuPeaks = new Float32Array(h);
    let vuPeakHold = new Uint16Array(h);

    function formatHz(v) {
      if (v >= 1000) return `${(v / 1000).toFixed(v % 1000 === 0 ? 0 : 1)} k`;
      return `${v}`;
    }

    function formatSeconds(v) {
      return `-${v.toFixed(v >= 10 ? 0 : 1)} s`;
    }

    function chooseTicks(minHz, maxHz) {
      const allTicks = [50, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500, 16000];
      const inRange = allTicks.filter((f) => f >= minHz && f <= maxHz);
      if (inRange.length <= 8) {
        return inRange;
      }

      const out = [];
      const last = inRange.length - 1;
      for (let i = 0; i < 8; i++) {
        const idx = Math.round((i / 7) * last);
        out.push(inRange[idx]);
      }
      return [...new Set(out)];
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

      bandLabelEl.textContent = `Band: ${formatHz(minFreqHz)}Hz to ${formatHz(maxFreqHz)}Hz (log scale)`;
      const midHz = Math.sqrt(minFreqHz * maxFreqHz);
      vuMinEl.textContent = `${formatHz(minFreqHz)}Hz`;
      vuMidEl.textContent = `${formatHz(midHz)}Hz`;
      vuMaxEl.textContent = `${formatHz(maxFreqHz)}Hz`;
    }

    function updateWaveTimeAxis() {
      const totalSeconds = (waveW * WAVE_STRIP_WIDTH * wsPushIntervalMs) / 1000;
      waveT0El.textContent = formatSeconds(totalSeconds);
      waveT1El.textContent = formatSeconds(totalSeconds * 0.75);
      waveT2El.textContent = formatSeconds(totalSeconds * 0.25);
    }

    async function loadSpectrumConfig() {
      try {
        const r = await fetch('/spectrum', { cache: 'no-store' });
        if (!r.ok) {
          return;
        }
        const cfg = await r.json();
        const minHz = Number(cfg.min_hz);
        const maxHz = Number(cfg.max_hz);
        if (Number.isFinite(minHz) && Number.isFinite(maxHz) && minHz > 0 && maxHz > minHz) {
          minFreqHz = minHz;
          maxFreqHz = maxHz;
          tickFreqs = chooseTicks(minFreqHz, maxFreqHz);
        }

        const wfBins = Number(cfg.waveform_bins);
        if (Number.isInteger(wfBins) && wfBins >= 16 && wfBins <= 1024) {
          waveformBins = wfBins;
        }

        const frameBytes = Number(cfg.ws_frame_bytes);
        if (Number.isInteger(frameBytes) && frameBytes >= h && frameBytes <= 4096) {
          wsFrameBytes = frameBytes;
        } else {
          wsFrameBytes = h + waveformBins * 2;
        }

        const pushMs = Number(cfg.ws_push_interval_ms);
        if (Number.isFinite(pushMs) && pushMs >= 10 && pushMs <= 1000) {
          wsPushIntervalMs = pushMs;
        }
      } catch (_e) {
      }

      updateWaveTimeAxis();
    }

    function palette(v) {
      const t = v / 255;
      const r = Math.min(255, Math.max(0, 255 * Math.pow(t, 1.2)));
      const g = Math.min(255, Math.max(0, 255 * Math.pow(t, 2.0)));
      const b = Math.min(255, Math.max(0, 255 * (1.0 - t) * 0.8));
      return [r | 0, g | 0, b | 0];
    }

    function clamp(v, lo, hi) {
      return Math.min(hi, Math.max(lo, v));
    }

    function setDisplayRange(v, persist) {
      const range = clamp(Number(v), Number(rangeDbEl.min), Number(rangeDbEl.max));
      displayRangeDb = range;
      rangeDbEl.value = `${range}`;
      rangeDbValueEl.textContent = `${range} dB`;
      if (persist) {
        try {
          localStorage.setItem(RANGE_DB_STORAGE_KEY, `${range}`);
        } catch (_e) {
        }
      }
      updateLegend(displayTopDb);
    }

    function setWaveGain(v, persist) {
      const gain = clamp(Number(v), Number(waveGainEl.min), Number(waveGainEl.max));
      waveVerticalGain = gain;
      waveGainEl.value = `${gain}`;
      waveGainValueEl.textContent = `${gain.toFixed(1)}x`;
      if (persist) {
        try {
          localStorage.setItem(WAVE_GAIN_STORAGE_KEY, `${gain}`);
        } catch (_e) {
        }
      }
    }

    function binToDbfs(v) {
      return MIN_DBFS + (v / 255) * (MAX_DBFS - MIN_DBFS);
    }

    function percentileBin(values, pct) {
      const sorted = Array.from(values).sort((a, b) => a - b);
      const idx = Math.round(clamp((pct / 100) * (sorted.length - 1), 0, sorted.length - 1));
      return sorted[idx];
    }

    function ensureVuState(count) {
      if (vuLevels.length === count) {
        return;
      }
      vuLevels = new Float32Array(count);
      vuPeaks = new Float32Array(count);
      vuPeakHold = new Uint16Array(count);
    }

    function fmtDb(v) {
      return `${Math.round(v)}`;
    }

    function updateLegend(topDb) {
      const bottomDb = topDb - displayRangeDb;
      const midDb = topDb - displayRangeDb * 0.5;
      legendTopEl.textContent = fmtDb(topDb);
      legendMidEl.textContent = fmtDb(midDb);
      legendBottomEl.textContent = fmtDb(bottomDb);
    }

    function drawVu(values, displayBottomDb) {
      const count = values.length;
      ensureVuState(count);

      vuCtx.fillStyle = 'rgb(0,0,0)';
      vuCtx.fillRect(0, 0, vuW, vuH);

      const pitch = vuW / count;
      const barW = Math.max(2, Math.floor(pitch) - 1);
      const segCount = 20;
      const segGap = 1;
      const segH = Math.max(2, Math.floor((vuH - (segCount - 1) * segGap) / segCount));

      for (let i = 0; i < count; i++) {
        const db = binToDbfs(values[i] || 0);
        const norm = clamp((db - displayBottomDb) / displayRangeDb, 0, 1);

        const rise = norm > vuLevels[i] ? 0.45 : 0.12;
        vuLevels[i] += (norm - vuLevels[i]) * rise;

        if (vuLevels[i] >= vuPeaks[i]) {
          vuPeaks[i] = vuLevels[i];
          vuPeakHold[i] = 18;
        } else if (vuPeakHold[i] > 0) {
          vuPeakHold[i]--;
        } else {
          vuPeaks[i] = Math.max(0, vuPeaks[i] - 0.025);
        }

        const x = Math.floor(i * pitch);
        for (let s = 0; s < segCount; s++) {
          const y = vuH - ((s + 1) * segH + s * segGap);
          if (y < 0) {
            continue;
          }
          const threshold = (s + 1) / segCount;
          if (vuLevels[i] >= threshold) {
            const [r, g, b] = palette((threshold * 255) | 0);
            vuCtx.fillStyle = `rgb(${r},${g},${b})`;
          } else {
            vuCtx.fillStyle = 'rgb(10,20,28)';
          }
          vuCtx.fillRect(x, y, barW, segH);
        }

        const peakY = vuH - Math.round(vuPeaks[i] * (vuH - 3));
        vuCtx.fillStyle = 'rgb(255,96,70)';
        vuCtx.fillRect(x, clamp(peakY, 0, vuH - 2), barW, 2);
      }
    }

    function drawWave(pcmSamples) {
      if (!pcmSamples || pcmSamples.length === 0) {
        return;
      }

      waveCtx.drawImage(waveCanvas, -WAVE_STRIP_WIDTH, 0);
      waveCtx.fillStyle = 'rgb(8,18,26)';
      waveCtx.fillRect(waveW - WAVE_STRIP_WIDTH, 0, WAVE_STRIP_WIDTH, waveH);

      const midY = Math.floor(waveH * 0.5);
      const toY = (sample) => {
        const n = clamp((sample / 32768) * waveVerticalGain, -1, 1);
        return clamp(Math.round(midY - n * (waveH * 0.45)), 0, waveH - 1);
      };

      for (let col = 0; col < WAVE_STRIP_WIDTH; col++) {
        const s0 = Math.floor((col * pcmSamples.length) / WAVE_STRIP_WIDTH);
        const s1 = Math.max(s0 + 1, Math.floor(((col + 1) * pcmSamples.length) / WAVE_STRIP_WIDTH));

        let minS = 32767;
        let maxS = -32768;
        let absPeak = 0;
        for (let i = s0; i < s1; i++) {
          const s = pcmSamples[i];
          if (s < minS) minS = s;
          if (s > maxS) maxS = s;
          const absVal = Math.abs(s);
          if (absVal > absPeak) absPeak = absVal;
        }

        const yMin = toY(maxS);
        const yMax = toY(minS);
        const level = clamp((absPeak / 32768) * 255, 0, 255);
        const [r, g, b] = palette(level);
        const x = waveW - WAVE_STRIP_WIDTH + col;

        waveCtx.fillStyle = `rgba(${r},${g},${b},0.35)`;
        waveCtx.fillRect(x, yMin, 1, Math.max(1, yMax - yMin + 1));
        waveCtx.fillStyle = `rgb(${r},${g},${b})`;
        waveCtx.fillRect(x, yMin, 1, 1);
        waveCtx.fillRect(x, yMax, 1, 1);
      }

      waveCtx.fillStyle = 'rgba(180,230,255,0.18)';
      waveCtx.fillRect(waveW - WAVE_STRIP_WIDTH, midY, WAVE_STRIP_WIDTH, 1);
    }

    function drawColumn(values, pcmSamples) {
      const peakDb = binToDbfs(percentileBin(values, 99.5));
      const targetTopDb = clamp(peakDb, MIN_DBFS + displayRangeDb, MAX_DBFS);
      const alpha = targetTopDb > displayTopDb ? PEAK_ATTACK : PEAK_RELEASE;
      displayTopDb += (targetTopDb - displayTopDb) * alpha;
      const displayBottomDb = displayTopDb - displayRangeDb;

      ctx.drawImage(canvas, -1, 0);
      for (let y = 0; y < h; y++) {
        const idx = h - 1 - y;
        const db = binToDbfs(values[idx] || 0);
        const norm = clamp((db - displayBottomDb) / displayRangeDb, 0, 1);
        const [r, g, b] = palette(norm * 255);
        ctx.fillStyle = `rgb(${r},${g},${b})`;
        ctx.fillRect(w - 1, y, 1, 1);
      }

      drawVu(values, displayBottomDb);
      drawWave(pcmSamples);

      updateLegend(displayTopDb);

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
        drawColumn(latestBins, latestWavePcm);
        latestBins = null;
        latestWavePcm = null;
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
        const bytes = new Uint8Array(evt.data);
        if (bytes.length < h) return;

        const bins = bytes.slice(0, h);
        let wavePcm = null;

        if (bytes.length >= wsFrameBytes && wsFrameBytes >= h + waveformBins * 2) {
          const dv = new DataView(evt.data);
          wavePcm = new Int16Array(waveformBins);
          for (let i = 0; i < waveformBins; i++) {
            wavePcm[i] = dv.getInt16(h + i * 2, true);
          }
        }

        latestBins = bins;
        latestWavePcm = wavePcm;
      };
    }

    ctx.fillStyle = 'rgb(0,0,0)';
    ctx.fillRect(0, 0, w, h);
    vuCtx.fillStyle = 'rgb(0,0,0)';
    vuCtx.fillRect(0, 0, vuW, vuH);
    waveCtx.fillStyle = 'rgb(0,0,0)';
    waveCtx.fillRect(0, 0, waveW, waveH);
    rangeDbEl.addEventListener('input', () => setDisplayRange(rangeDbEl.value, true));
    waveGainEl.addEventListener('input', () => setWaveGain(waveGainEl.value, true));
    updateWaveTimeAxis();
    try {
      const savedRange = localStorage.getItem(RANGE_DB_STORAGE_KEY);
      if (savedRange != null) {
        setDisplayRange(savedRange, false);
      } else {
        setDisplayRange(displayRangeDb, false);
      }
    } catch (_e) {
      setDisplayRange(displayRangeDb, false);
    }
    try {
      const savedGain = localStorage.getItem(WAVE_GAIN_STORAGE_KEY);
      if (savedGain != null) {
        setWaveGain(savedGain, false);
      } else {
        setWaveGain(waveVerticalGain, false);
      }
    } catch (_e) {
      setWaveGain(waveVerticalGain, false);
    }
    updateLegend(displayTopDb);
    loadSpectrumConfig().finally(() => {
      renderFrequencyTicks();
      requestAnimationFrame(renderLoop);
      connect();
    });
  </script>
</body>
</html>
)HTML";

} // namespace WebUi
