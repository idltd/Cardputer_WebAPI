// api_webapp.cpp — Serves the web control app directly from firmware flash.
// All web files are embedded as string literals — no LittleFS or data upload needed.

#include "config.h"
#include "api_webapp.h"
#include "api_server.h"

// ── index.html ─────────────────────────────────────────────────────────────

static const char FILE_INDEX[] = R"WEBEND(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <meta name="theme-color" content="#00ff88">
    <title>Cardputer Control</title>
    <link rel="stylesheet" href="css/style.css">
</head>
<body>
    <div id="connection-bar">
        <div class="conn-row">
            <input type="text" id="ip-input" value="192.168.4.1" placeholder="IP address" aria-label="Cardputer IP address">
            <button id="connect-btn">Connect</button>
            <span id="status-dot" class="dot disconnected" aria-hidden="true"></span>
            <span id="status-text">Disconnected</span>
        </div>
        <div id="sys-summary" class="hidden"></div>
    </div>
    <nav id="tab-bar" role="tablist" aria-label="Hardware modules">
        <button class="tab active" data-tab="system" role="tab" aria-selected="true" aria-controls="tab-system">System</button>
        <button class="tab" data-tab="gps" role="tab" aria-selected="false" aria-controls="tab-gps">GPS</button>
        <button class="tab" data-tab="lora" role="tab" aria-selected="false" aria-controls="tab-lora">LoRa</button>
        <button class="tab" data-tab="imu" role="tab" aria-selected="false" aria-controls="tab-imu">IMU</button>
        <button class="tab" data-tab="keyboard" role="tab" aria-selected="false" aria-controls="tab-keyboard">Keys</button>
        <button class="tab" data-tab="ir" role="tab" aria-selected="false" aria-controls="tab-ir">IR</button>
        <button class="tab" data-tab="display" role="tab" aria-selected="false" aria-controls="tab-display">Display</button>
        <button class="tab" data-tab="gpio" role="tab" aria-selected="false" aria-controls="tab-gpio">GPIO</button>
        <button class="tab" data-tab="audio" role="tab" aria-selected="false" aria-controls="tab-audio">Audio</button>
        <button class="tab" data-tab="grove" role="tab" aria-selected="false" aria-controls="tab-grove">Grove</button>
        <button class="tab" data-tab="record" role="tab" aria-selected="false" aria-controls="tab-record">Record</button>
    </nav>
    <main id="tab-content">
        <section id="tab-system" class="tab-panel active" role="tabpanel"></section>
        <section id="tab-gps"    class="tab-panel" role="tabpanel"></section>
        <section id="tab-lora"   class="tab-panel" role="tabpanel"></section>
        <section id="tab-imu"    class="tab-panel" role="tabpanel"></section>
        <section id="tab-keyboard" class="tab-panel" role="tabpanel"></section>
        <section id="tab-ir"     class="tab-panel" role="tabpanel"></section>
        <section id="tab-display" class="tab-panel" role="tabpanel"></section>
        <section id="tab-gpio"   class="tab-panel" role="tabpanel"></section>
        <section id="tab-audio"  class="tab-panel" role="tabpanel"></section>
        <section id="tab-grove"  class="tab-panel" role="tabpanel"></section>
        <section id="tab-record" class="tab-panel" role="tabpanel"></section>
    </main>
    <script type="module" src="js/app.js"></script>
</body>
</html>
)WEBEND";

// ── css/style.css ──────────────────────────────────────────────────────────

static const char FILE_STYLE_CSS[] = R"WEBEND(*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

:root {
    --bg: #0a0a0a;
    --surface: #141414;
    --surface2: #1e1e1e;
    --border: #2a2a2a;
    --text: #e0e0e0;
    --text-dim: #888;
    --accent: #00ff88;
    --accent-dim: #00cc6a;
    --red: #ff4444;
    --yellow: #ffcc00;
    --blue: #4488ff;
    --cyan: #00ccff;
    --orange: #ff8800;
}

html, body {
    height: 100%; width: 100%;
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', system-ui, sans-serif;
    font-size: 14px;
    background: var(--bg);
    color: var(--text);
    overflow: hidden;
    -webkit-user-select: none; user-select: none;
}

#connection-bar {
    background: var(--surface);
    border-bottom: 1px solid var(--border);
    padding: 8px 12px;
}

.conn-row {
    display: flex;
    align-items: center;
    gap: 8px;
}

#ip-input {
    flex: 1;
    max-width: 160px;
    padding: 6px 10px;
    border-radius: 6px;
    border: 1px solid var(--border);
    background: var(--surface2);
    color: var(--text);
    font-size: 14px;
    font-family: monospace;
}

#connect-btn {
    padding: 6px 16px;
    border-radius: 6px;
    border: none;
    background: var(--accent);
    color: #000;
    font-weight: 600;
    font-size: 13px;
    cursor: pointer;
}
#connect-btn:active { opacity: 0.8; }
#connect-btn.connected {
    background: var(--red);
    color: #fff;
}

.dot {
    width: 10px; height: 10px;
    border-radius: 50%;
    flex-shrink: 0;
}
.dot.disconnected { background: var(--red); }
.dot.connected { background: var(--accent); box-shadow: 0 0 6px var(--accent); }
.dot.connecting { background: var(--yellow); animation: pulse 1s infinite; }

@keyframes pulse { 50% { opacity: 0.4; } }

#status-text {
    font-size: 12px;
    color: var(--text-dim);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}

#sys-summary {
    font-size: 11px;
    color: var(--text-dim);
    margin-top: 4px;
    font-family: monospace;
}

.hidden { display: none !important; }

#tab-bar {
    display: flex;
    overflow-x: auto;
    background: var(--surface);
    border-bottom: 1px solid var(--border);
    scrollbar-width: none;
    -webkit-overflow-scrolling: touch;
}
#tab-bar::-webkit-scrollbar { display: none; }

.tab {
    flex-shrink: 0;
    padding: 8px 14px;
    border: none;
    background: none;
    color: var(--text-dim);
    font-size: 13px;
    font-weight: 500;
    cursor: pointer;
    border-bottom: 2px solid transparent;
    white-space: nowrap;
}
.tab.active {
    color: var(--accent);
    border-bottom-color: var(--accent);
}

#tab-content {
    flex: 1;
    overflow-y: auto;
    -webkit-overflow-scrolling: touch;
}

body {
    display: flex;
    flex-direction: column;
}

.tab-panel {
    display: none;
    padding: 12px;
}
.tab-panel.active { display: block; }

.card {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 12px;
    margin-bottom: 10px;
}
.card h3 {
    font-size: 12px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
    color: var(--accent);
    margin-bottom: 8px;
}

.kv { display: flex; justify-content: space-between; padding: 3px 0; font-size: 13px; }
.kv .label { color: var(--text-dim); }
.kv .value { font-family: monospace; color: var(--text); }

.bar-wrap {
    height: 6px;
    background: var(--surface2);
    border-radius: 3px;
    overflow: hidden;
    margin: 4px 0 2px;
}
.bar-fill {
    height: 100%;
    border-radius: 3px;
    background: var(--accent);
    transition: width 0.3s;
}
.bar-fill.warn { background: var(--yellow); }
.bar-fill.danger { background: var(--red); }

.form-row {
    display: flex;
    gap: 8px;
    align-items: center;
    margin-bottom: 8px;
    flex-wrap: wrap;
}

.form-row label {
    font-size: 12px;
    color: var(--text-dim);
    min-width: 50px;
}

input[type="text"], input[type="number"], select {
    padding: 6px 8px;
    border-radius: 6px;
    border: 1px solid var(--border);
    background: var(--surface2);
    color: var(--text);
    font-size: 13px;
    font-family: monospace;
    min-width: 0;
}

input[type="range"] {
    flex: 1;
    accent-color: var(--accent);
}

input[type="color"] {
    width: 36px; height: 30px;
    border: 1px solid var(--border);
    border-radius: 6px;
    background: var(--surface2);
    cursor: pointer;
    padding: 2px;
}

button {
    padding: 6px 14px;
    border-radius: 6px;
    border: none;
    background: var(--accent);
    color: #000;
    font-weight: 600;
    font-size: 13px;
    cursor: pointer;
    white-space: nowrap;
}
button:active { opacity: 0.8; }
button.secondary {
    background: var(--surface2);
    color: var(--text);
    border: 1px solid var(--border);
}
button.danger { background: var(--red); color: #fff; }

.log {
    max-height: 300px;
    overflow-y: auto;
    font-size: 12px;
    font-family: monospace;
}
.log-entry {
    padding: 6px 8px;
    border-bottom: 1px solid var(--border);
    line-height: 1.4;
}
.log-entry:last-child { border-bottom: none; }
.log-time { color: var(--text-dim); margin-right: 6px; }

.badge {
    display: inline-block;
    padding: 1px 6px;
    border-radius: 4px;
    font-size: 11px;
    font-weight: 600;
    margin-right: 3px;
}
.badge.shift { background: var(--blue); color: #fff; }
.badge.ctrl { background: var(--orange); color: #fff; }
.badge.alt { background: var(--cyan); color: #000; }
.badge.fn { background: var(--yellow); color: #000; }

.imu-bar-group { margin-bottom: 6px; }
.imu-bar-label {
    display: flex;
    justify-content: space-between;
    font-size: 11px;
    color: var(--text-dim);
}
.imu-bar-track {
    height: 8px;
    background: var(--surface2);
    border-radius: 4px;
    position: relative;
    overflow: hidden;
}
.imu-bar-thumb {
    position: absolute;
    top: 0; height: 100%;
    border-radius: 4px;
    transition: left 0.08s, width 0.08s;
}

canvas {
    display: block;
    width: 100%;
    border-radius: 6px;
    border: 1px solid var(--border);
    background: var(--surface2);
}

.rssi-bar {
    display: inline-block;
    height: 10px;
    border-radius: 3px;
    margin-right: 6px;
    vertical-align: middle;
}

.preset-row {
    display: flex;
    gap: 6px;
    flex-wrap: wrap;
    margin-bottom: 8px;
}
.preset-row button {
    font-size: 12px;
    padding: 4px 10px;
}

.toggle-group {
    display: flex;
    border-radius: 6px;
    overflow: hidden;
    border: 1px solid var(--border);
}
.toggle-group button {
    border-radius: 0;
    border: none;
    background: var(--surface2);
    color: var(--text-dim);
    padding: 6px 12px;
    font-size: 12px;
}
.toggle-group button.active {
    background: var(--accent);
    color: #000;
}
)WEBEND";

// ── js/api.js ──────────────────────────────────────────────────────────────

static const char FILE_API_JS[] = R"WEBEND(export class CardputerAPI {
    constructor() {
        this._ip = '';
        this._connected = false;
        this._ws = new Map();
        this._wsCallbacks = new Map();
        this._reconnectTimers = new Map();
        this.onStatusChange = null;
    }

    get ip() { return this._ip; }
    get connected() { return this._connected; }

    connect(ip) {
        this._ip = ip;
        this._setStatus(true);
        return this.get('/api/system/info').then(data => {
            this._setStatus(true);
            return data;
        }).catch(err => {
            this._setStatus(false);
            throw err;
        });
    }

    disconnect() {
        this._connected = false;
        for (const [path, ws] of this._ws) { ws.close(); }
        this._ws.clear();
        this._wsCallbacks.clear();
        for (const timer of this._reconnectTimers.values()) { clearTimeout(timer); }
        this._reconnectTimers.clear();
        this._setStatus(false);
    }

    async get(path) {
        try {
            const resp = await fetch(`http://${this._ip}${path}`, { signal: AbortSignal.timeout(5000) });
            if (!resp.ok) {
                const body = await resp.json().catch(() => null);
                throw new Error(body?.error || `${resp.status} ${resp.statusText}`);
            }
            return resp.json();
        } catch (e) {
            if (e.name === 'TimeoutError' || e.name === 'AbortError') throw new Error('Request timed out');
            if (e.name === 'TypeError') throw new Error('Network error — check WiFi connection');
            throw e;
        }
    }

    async post(path, body) {
        try {
            const resp = await fetch(`http://${this._ip}${path}`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(body),
                signal: AbortSignal.timeout(5000)
            });
            if (!resp.ok) {
                const data = await resp.json().catch(() => null);
                throw new Error(data?.error || `${resp.status} ${resp.statusText}`);
            }
            return resp.json();
        } catch (e) {
            if (e.name === 'TimeoutError' || e.name === 'AbortError') throw new Error('Request timed out');
            if (e.name === 'TypeError') throw new Error('Network error — check WiFi connection');
            throw e;
        }
    }

    subscribe(wsPath, callback) {
        if (this._ws.has(wsPath)) return;
        this._wsCallbacks.set(wsPath, callback);
        this._openWs(wsPath, callback);
    }

    unsubscribe(wsPath) {
        this._wsCallbacks.delete(wsPath);
        const ws = this._ws.get(wsPath);
        if (ws) { ws.close(); this._ws.delete(wsPath); }
        const timer = this._reconnectTimers.get(wsPath);
        if (timer) { clearTimeout(timer); this._reconnectTimers.delete(wsPath); }
    }

    _openWs(wsPath, callback) {
        const url = `ws://${this._ip}${wsPath}`;
        const ws = new WebSocket(url);
        ws.onmessage = (e) => { try { callback(JSON.parse(e.data)); } catch { callback(e.data); } };
        ws.onclose = () => {
            this._ws.delete(wsPath);
            if (this._connected && this._wsCallbacks.has(wsPath)) {
                const timer = setTimeout(() => {
                    this._reconnectTimers.delete(wsPath);
                    if (this._connected && this._wsCallbacks.has(wsPath)) this._openWs(wsPath, callback);
                }, 2000);
                this._reconnectTimers.set(wsPath, timer);
            }
        };
        ws.onerror = () => ws.close();
        this._ws.set(wsPath, ws);
    }

    _setStatus(connected) {
        this._connected = connected;
        if (this.onStatusChange) this.onStatusChange(connected);
    }
}
)WEBEND";

// ── js/app.js ──────────────────────────────────────────────────────────────

static const char FILE_APP_JS[] = R"WEBEND(import { CardputerAPI } from './api.js';
import { initSystem, activateSystem, deactivateSystem } from './tab-system.js';
import { initGps, activateGps, deactivateGps } from './tab-gps.js';
import { initLora, activateLora, deactivateLora } from './tab-lora.js';
import { initImu, activateImu, deactivateImu } from './tab-imu.js';
import { initKeyboard, activateKeyboard, deactivateKeyboard } from './tab-keyboard.js';
import { initIr, activateIr, deactivateIr } from './tab-ir.js';
import { initDisplay, activateDisplay, deactivateDisplay } from './tab-display.js';
import { initGpio, activateGpio, deactivateGpio } from './tab-gpio.js';
import { initAudio, activateAudio, deactivateAudio } from './tab-audio.js';
import { initRecord, activateRecord, deactivateRecord } from './tab-record.js';
import { initGrove, activateGrove, deactivateGrove } from './tab-grove.js';

const api = new CardputerAPI();

const tabs = {
    system:   { init: initSystem,   activate: activateSystem,   deactivate: deactivateSystem },
    gps:      { init: initGps,      activate: activateGps,      deactivate: deactivateGps },
    lora:     { init: initLora,     activate: activateLora,     deactivate: deactivateLora },
    imu:      { init: initImu,      activate: activateImu,      deactivate: deactivateImu },
    keyboard: { init: initKeyboard, activate: activateKeyboard, deactivate: deactivateKeyboard },
    ir:       { init: initIr,       activate: activateIr,       deactivate: deactivateIr },
    display:  { init: initDisplay,  activate: activateDisplay,  deactivate: deactivateDisplay },
    gpio:     { init: initGpio,     activate: activateGpio,     deactivate: deactivateGpio },
    audio:    { init: initAudio,    activate: activateAudio,    deactivate: deactivateAudio },
    grove:    { init: initGrove,    activate: activateGrove,    deactivate: deactivateGrove },
    record:   { init: initRecord,   activate: activateRecord,   deactivate: deactivateRecord },
};

let activeTab = 'system';
let sysInfoInterval = null;

const ipInput = document.getElementById('ip-input');
const connectBtn = document.getElementById('connect-btn');
const statusDot = document.getElementById('status-dot');
const statusText = document.getElementById('status-text');
const sysSummary = document.getElementById('sys-summary');

const servedFromDevice = location.hostname !== '' &&
    location.hostname !== 'localhost' &&
    location.hostname !== '127.0.0.1';

if (servedFromDevice) {
    ipInput.value = location.hostname;
} else {
    const savedIp = localStorage.getItem('cardputer-ip');
    if (savedIp) ipInput.value = savedIp;
}

if (servedFromDevice && !api.connected) {
    statusDot.className = 'dot connecting';
    statusText.textContent = 'Connecting...';
    api.connect(location.hostname).catch(e => {
        statusText.textContent = e.message || 'Connection failed';
    });
}

api.onStatusChange = (connected) => {
    statusDot.className = 'dot ' + (connected ? 'connected' : 'disconnected');
    statusText.textContent = connected ? 'Connected' : 'Disconnected';
    connectBtn.textContent = connected ? 'Disconnect' : 'Connect';
    connectBtn.classList.toggle('connected', connected);
    if (connected) { startSysPolling(); tabs[activeTab].activate(api); }
    else { stopSysPolling(); sysSummary.classList.add('hidden'); tabs[activeTab].deactivate(api); }
};

connectBtn.addEventListener('click', () => {
    if (api.connected) {
        api.disconnect();
    } else {
        const ip = ipInput.value.trim();
        if (!servedFromDevice) localStorage.setItem('cardputer-ip', ip);
        statusDot.className = 'dot connecting';
        statusText.textContent = 'Connecting...';
        api.connect(ip).catch(e => { statusText.textContent = e.message || 'Connection failed'; });
    }
});

ipInput.addEventListener('keydown', (e) => { if (e.key === 'Enter' && !api.connected) connectBtn.click(); });

function startSysPolling() { pollSysInfo(); sysInfoInterval = setInterval(pollSysInfo, 5000); }
function stopSysPolling() { if (sysInfoInterval) { clearInterval(sysInfoInterval); sysInfoInterval = null; } }

function pollSysInfo() {
    if (!api.connected) return;
    api.get('/api/system/info').then(data => {
        const heap = data.heap;
        const upSec = Math.floor(data.uptime_ms / 1000);
        const m = Math.floor(upSec / 60), s = upSec % 60;
        sysSummary.textContent = `${data.chip?.model || '?'}  |  Heap: ${fmt(heap?.free)}/${fmt(heap?.total)}  |  Up: ${m}m${s}s`;
        sysSummary.classList.remove('hidden');
    }).catch(() => {});
}

function fmt(n) {
    if (n == null) return '?';
    if (n >= 1048576) return (n / 1048576).toFixed(1) + 'M';
    if (n >= 1024) return (n / 1024).toFixed(0) + 'K';
    return String(n);
}

document.getElementById('tab-bar').addEventListener('click', (e) => {
    const btn = e.target.closest('.tab');
    if (!btn) return;
    const name = btn.dataset.tab;
    if (name === activeTab) return;
    if (api.connected) tabs[activeTab].deactivate(api);
    const oldTab = document.querySelector('.tab.active');
    oldTab.classList.remove('active');
    oldTab.setAttribute('aria-selected', 'false');
    document.querySelector('.tab-panel.active').classList.remove('active');
    activeTab = name;
    btn.classList.add('active');
    btn.setAttribute('aria-selected', 'true');
    document.getElementById('tab-' + name).classList.add('active');
    if (api.connected) tabs[activeTab].activate(api);
});

for (const [name, tab] of Object.entries(tabs)) {
    tab.init(document.getElementById('tab-' + name), api);
}

window.addEventListener('beforeunload', () => { if (api.connected) api.disconnect(); });
)WEBEND";

// ── js/tab-system.js ───────────────────────────────────────────────────────

static const char FILE_TAB_SYSTEM_JS[] = R"WEBEND(let panel, interval;

export function initSystem(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card" id="sys-chip"><h3>Chip</h3><div class="kv-list"></div></div>
        <div class="card" id="sys-memory">
            <h3>Memory</h3>
            <div class="kv"><span class="label">Heap</span><span class="value" id="sys-heap-text">--</span></div>
            <div class="bar-wrap"><div class="bar-fill" id="sys-heap-bar"></div></div>
            <div class="kv"><span class="label">PSRAM</span><span class="value" id="sys-psram-text">--</span></div>
            <div class="bar-wrap"><div class="bar-fill" id="sys-psram-bar"></div></div>
        </div>
        <div class="card" id="sys-net"><h3>Network</h3><div class="kv-list"></div></div>
        <div class="card" id="sys-misc"><h3>System</h3><div class="kv-list"></div></div>
    `;
}

export function activateSystem(api) { refresh(api); interval = setInterval(() => refresh(api), 5000); }
export function deactivateSystem() { if (interval) { clearInterval(interval); interval = null; } }

function refresh(api) { api.get('/api/system/info').then(render).catch(() => {}); }

function render(d) {
    setKvList('#sys-chip .kv-list', [
        ['Model', d.chip?.model], ['Revision', d.chip?.revision],
        ['Cores', d.chip?.cores], ['Freq', d.chip?.freq_mhz + ' MHz'],
    ]);
    const heapUsed = (d.heap?.total || 0) - (d.heap?.free || 0);
    const heapPct = d.heap?.total ? (heapUsed / d.heap.total * 100) : 0;
    document.getElementById('sys-heap-text').textContent = `${fmt(heapUsed)} / ${fmt(d.heap?.total)} (${heapPct.toFixed(0)}%)`;
    const heapBar = document.getElementById('sys-heap-bar');
    heapBar.style.width = heapPct + '%';
    heapBar.className = 'bar-fill' + (heapPct > 80 ? ' danger' : heapPct > 60 ? ' warn' : '');
    const psramUsed = (d.psram?.total || 0) - (d.psram?.free || 0);
    const psramPct = d.psram?.total ? (psramUsed / d.psram.total * 100) : 0;
    document.getElementById('sys-psram-text').textContent = d.psram?.total
        ? `${fmt(psramUsed)} / ${fmt(d.psram.total)} (${psramPct.toFixed(0)}%)` : 'Not available';
    document.getElementById('sys-psram-bar').style.width = psramPct + '%';
    setKvList('#sys-net .kv-list', [
        ['SSID', d.wifi?.ssid], ['IP', d.wifi?.ip],
        ['WiFi Clients', d.wifi?.clients], ['WS Clients', d.ws_clients],
    ]);
    const upSec = Math.floor((d.uptime_ms || 0) / 1000);
    const h = Math.floor(upSec / 3600), m = Math.floor((upSec % 3600) / 60), s = upSec % 60;
    setKvList('#sys-misc .kv-list', [
        ['Uptime', `${h}h ${m}m ${s}s`], ['SDK', d.sdk_version],
        ['Flash', fmt(d.flash?.size)], ['Flash Speed', fmt(d.flash?.speed) + 'Hz'],
    ]);
}

function setKvList(selector, pairs) {
    const el = panel.querySelector(selector);
    el.innerHTML = pairs.map(([k, v]) =>
        `<div class="kv"><span class="label">${k}</span><span class="value">${v ?? '--'}</span></div>`
    ).join('');
}

function fmt(n) {
    if (n == null) return '?';
    if (n >= 1048576) return (n / 1048576).toFixed(1) + 'M';
    if (n >= 1024) return (n / 1024).toFixed(0) + 'K';
    return String(n);
}
)WEBEND";

// ── js/tab-gps.js ──────────────────────────────────────────────────────────

static const char FILE_TAB_GPS_JS[] = R"WEBEND(let panel, canvas, ctx;
const trail = [];
const MAX_TRAIL = 500;

export function initGps(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>GPS Position</h3>
            <div class="kv"><span class="label">Status</span><span class="value" id="gps-status">--</span></div>
            <div class="kv"><span class="label">Latitude</span><span class="value" id="gps-lat">--</span></div>
            <div class="kv"><span class="label">Longitude</span><span class="value" id="gps-lon">--</span></div>
            <div class="kv"><span class="label">Altitude</span><span class="value" id="gps-alt">--</span></div>
            <div class="kv"><span class="label">Speed</span><span class="value" id="gps-speed">--</span></div>
            <div class="kv"><span class="label">Course</span><span class="value" id="gps-course">--</span></div>
            <div class="kv"><span class="label">Satellites</span><span class="value" id="gps-sats">--</span></div>
            <div class="kv"><span class="label">HDOP</span><span class="value" id="gps-hdop">--</span></div>
            <div class="kv"><span class="label">UTC</span><span class="value" id="gps-utc">--</span></div>
        </div>
        <div class="card">
            <h3>HDOP Quality</h3>
            <div class="bar-wrap"><div class="bar-fill" id="gps-hdop-bar" style="width:0"></div></div>
        </div>
        <div class="card">
            <h3>Trail Map</h3>
            <button id="gps-clear-trail" class="secondary" style="margin-bottom:8px;font-size:11px">Clear Trail</button>
            <canvas id="gps-canvas" height="200"></canvas>
        </div>
    `;
    canvas = document.getElementById('gps-canvas');
    ctx = canvas.getContext('2d');
    panel.querySelector('#gps-clear-trail').onclick = () => { trail.length = 0; ctx.clearRect(0, 0, canvas.width, canvas.height); };
}

export function activateGps(api) { api.subscribe('/ws/gps', onGpsData); }
export function deactivateGps(api) { api.unsubscribe('/ws/gps'); }

function onGpsData(d) {
    const $ = id => document.getElementById(id);
    $('gps-status').textContent = d.valid ? 'Fix' : 'No Fix';
    $('gps-status').style.color = d.valid ? 'var(--accent)' : 'var(--red)';
    $('gps-lat').textContent = d.lat != null ? d.lat.toFixed(6) + '\u00B0' : '--';
    $('gps-lon').textContent = d.lon != null ? d.lon.toFixed(6) + '\u00B0' : '--';
    $('gps-alt').textContent = d.alt_m != null ? d.alt_m.toFixed(1) + ' m' : '--';
    $('gps-speed').textContent = d.speed_kmh != null ? d.speed_kmh.toFixed(1) + ' km/h' : '--';
    $('gps-course').textContent = d.course_deg != null ? d.course_deg.toFixed(1) + '\u00B0' : '--';
    $('gps-sats').textContent = d.satellites ?? '--';
    $('gps-hdop').textContent = d.hdop != null ? d.hdop.toFixed(1) : '--';
    $('gps-utc').textContent = d.utc ?? '--';
    if (d.hdop != null) {
        const pct = Math.max(0, Math.min(100, (1 - (d.hdop - 1) / 9) * 100));
        const bar = $('gps-hdop-bar');
        bar.style.width = pct + '%';
        bar.className = 'bar-fill' + (pct < 30 ? ' danger' : pct < 60 ? ' warn' : '');
    }
    if (d.valid && d.lat != null && d.lon != null) {
        trail.push({ lat: d.lat, lon: d.lon });
        if (trail.length > MAX_TRAIL) trail.shift();
        drawTrail();
    }
}

function drawTrail() {
    if (trail.length < 2) return;
    const w = canvas.clientWidth, h = canvas.clientHeight;
    canvas.width = w * devicePixelRatio; canvas.height = h * devicePixelRatio;
    ctx.setTransform(devicePixelRatio, 0, 0, devicePixelRatio, 0, 0);
    let minLat = Infinity, maxLat = -Infinity, minLon = Infinity, maxLon = -Infinity;
    for (const p of trail) {
        if (p.lat < minLat) minLat = p.lat; if (p.lat > maxLat) maxLat = p.lat;
        if (p.lon < minLon) minLon = p.lon; if (p.lon > maxLon) maxLon = p.lon;
    }
    const pad = 20, dLat = maxLat - minLat || 0.0001, dLon = maxLon - minLon || 0.0001;
    ctx.clearRect(0, 0, w, h);
    ctx.strokeStyle = '#00ff88'; ctx.lineWidth = 2; ctx.lineJoin = 'round';
    ctx.beginPath();
    for (let i = 0; i < trail.length; i++) {
        const x = pad + ((trail[i].lon - minLon) / dLon) * (w - pad * 2);
        const y = pad + (1 - (trail[i].lat - minLat) / dLat) * (h - pad * 2);
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    }
    ctx.stroke();
    const last = trail[trail.length - 1];
    const cx = pad + ((last.lon - minLon) / dLon) * (w - pad * 2);
    const cy = pad + (1 - (last.lat - minLat) / dLat) * (h - pad * 2);
    ctx.fillStyle = '#ff4444'; ctx.beginPath(); ctx.arc(cx, cy, 4, 0, Math.PI * 2); ctx.fill();
}
)WEBEND";

// ── js/tab-lora.js ─────────────────────────────────────────────────────────

static const char FILE_TAB_LORA_JS[] = R"WEBEND(let panel, currentApi;
const rxLog = [];

export function initLora(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>LoRa Configuration</h3>
            <div class="form-row">
                <label>Freq (MHz)</label>
                <input type="number" id="lora-freq" value="915.0" step="0.1" style="width:90px">
            </div>
            <div class="form-row">
                <label>SF</label>
                <select id="lora-sf">${[7,8,9,10,11,12].map(v=>`<option value="${v}"${v===7?' selected':''}>${v}</option>`).join('')}</select>
                <label>BW (kHz)</label>
                <select id="lora-bw">${[7.8,10.4,15.6,20.8,31.25,41.7,62.5,125,250,500].map(v=>`<option value="${v}"${v===125?' selected':''}>${v}</option>`).join('')}</select>
            </div>
            <div class="form-row">
                <label>Power (dBm)</label>
                <input type="number" id="lora-power" value="10" min="-9" max="22" style="width:60px">
                <label>CR</label>
                <select id="lora-cr">${[5,6,7,8].map(v=>`<option value="${v}"${v===5?' selected':''}>4/${v}</option>`).join('')}</select>
            </div>
            <div class="form-row">
                <label>Sync Word</label>
                <input type="text" id="lora-sync" value="0x12" style="width:60px">
            </div>
            <div class="form-row">
                <button id="lora-apply-btn">Apply Config</button>
                <button id="lora-refresh-btn" class="secondary">Refresh</button>
                <span id="lora-config-status" style="font-size:12px;color:var(--text-dim)"></span>
            </div>
        </div>
        <div class="card">
            <h3>Send Packet</h3>
            <div class="form-row">
                <input type="text" id="lora-send-data" placeholder="Payload" style="flex:1">
                <div class="toggle-group">
                    <button id="lora-mode-text" class="active">Text</button>
                    <button id="lora-mode-hex">Hex</button>
                </div>
            </div>
            <div class="form-row">
                <button id="lora-send-btn">Send</button>
                <span id="lora-send-status" style="font-size:12px;color:var(--text-dim)"></span>
            </div>
        </div>
        <div class="card">
            <h3>Received Packets</h3>
            <button id="lora-clear-btn" class="secondary" style="margin-bottom:8px;font-size:11px">Clear</button>
            <div class="log" id="lora-rx-log"><div style="color:var(--text-dim);padding:8px">No packets yet</div></div>
        </div>
    `;

    let hexMode = false;
    panel.querySelector('#lora-mode-text').onclick = () => { hexMode = false; panel.querySelector('#lora-mode-text').classList.add('active'); panel.querySelector('#lora-mode-hex').classList.remove('active'); };
    panel.querySelector('#lora-mode-hex').onclick  = () => { hexMode = true;  panel.querySelector('#lora-mode-hex').classList.add('active');  panel.querySelector('#lora-mode-text').classList.remove('active'); };

    panel.querySelector('#lora-send-btn').onclick = () => {
        if (!currentApi) return;
        const data = panel.querySelector('#lora-send-data').value;
        if (!data) return;
        const status = panel.querySelector('#lora-send-status');
        status.textContent = 'Sending...';
        currentApi.post('/api/lora/send', { data, hex: hexMode }).then(r => {
            status.textContent = `Sent ${r.bytes} bytes`;
            panel.querySelector('#lora-send-data').value = '';
            setTimeout(() => status.textContent = '', 2000);
        }).catch(e => { status.textContent = 'Error: ' + e.message; });
    };

    panel.querySelector('#lora-apply-btn').onclick = () => {
        if (!currentApi) return;
        const syncVal = panel.querySelector('#lora-sync').value.trim();
        const cfg = {
            frequency_mhz: parseFloat(panel.querySelector('#lora-freq').value),
            spreading_factor: parseInt(panel.querySelector('#lora-sf').value),
            bandwidth_khz: parseFloat(panel.querySelector('#lora-bw').value),
            tx_power_dbm: parseInt(panel.querySelector('#lora-power').value),
            coding_rate: parseInt(panel.querySelector('#lora-cr').value),
            sync_word: parseInt(syncVal, syncVal.startsWith('0x') ? 16 : 10),
        };
        const status = panel.querySelector('#lora-config-status');
        status.textContent = 'Applying...';
        currentApi.post('/api/lora/config', cfg).then(r => {
            status.textContent = r.ready ? 'Applied' : 'LoRa not ready';
            setTimeout(() => status.textContent = '', 2000);
        }).catch(e => { status.textContent = 'Error: ' + e.message; });
    };

    panel.querySelector('#lora-refresh-btn').onclick = () => refreshConfig();
    panel.querySelector('#lora-clear-btn').onclick = () => { rxLog.length = 0; renderLog(); };
}

export function activateLora(api) { currentApi = api; refreshConfig(); api.subscribe('/ws/lora', onLoraRx); }
export function deactivateLora(api) { api.unsubscribe('/ws/lora'); currentApi = null; }

function refreshConfig() {
    if (!currentApi) return;
    currentApi.get('/api/lora/config').then(d => {
        panel.querySelector('#lora-freq').value = d.frequency_mhz;
        panel.querySelector('#lora-sf').value = d.spreading_factor;
        panel.querySelector('#lora-bw').value = d.bandwidth_khz;
        panel.querySelector('#lora-power').value = d.tx_power_dbm;
        panel.querySelector('#lora-cr').value = d.coding_rate;
        panel.querySelector('#lora-sync').value = '0x' + d.sync_word.toString(16).toUpperCase();
        panel.querySelector('#lora-config-status').textContent = d.ready ? 'Ready' : 'Not initialized';
    }).catch(() => {});
}

function onLoraRx(d) {
    rxLog.unshift({ time: new Date().toLocaleTimeString(), data: d.data, hex: d.hex, rssi: d.rssi, snr: d.snr, len: d.length });
    if (rxLog.length > 100) rxLog.pop();
    renderLog();
}

function renderLog() {
    const el = panel.querySelector('#lora-rx-log');
    if (rxLog.length === 0) { el.innerHTML = '<div style="color:var(--text-dim);padding:8px">No packets yet</div>'; return; }
    el.innerHTML = rxLog.map(p => {
        const rssiColor = p.rssi > -70 ? 'var(--accent)' : p.rssi > -100 ? 'var(--yellow)' : 'var(--red)';
        const rssiW = Math.max(5, Math.min(100, (p.rssi + 140) / 1.4));
        const snrColor = p.snr > 5 ? 'var(--accent)' : p.snr > 0 ? 'var(--yellow)' : 'var(--red)';
        const snrW = Math.max(5, Math.min(100, (p.snr + 20) / 0.4));
        return `<div class="log-entry"><span class="log-time">${p.time}</span><span>${p.len}B</span><br>
            <span style="word-break:break-all">${escHtml(p.data)}</span>
            <span style="color:var(--text-dim);font-size:11px"> [${p.hex}]</span><br>
            <span class="rssi-bar" style="background:${rssiColor};width:${rssiW}px"></span><span style="font-size:11px">${p.rssi} dBm</span>
            <span class="rssi-bar" style="background:${snrColor};width:${snrW}px;margin-left:8px"></span><span style="font-size:11px">${p.snr} dB</span>
        </div>`;
    }).join('');
}

function escHtml(s) { return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }
)WEBEND";

// ── js/tab-imu.js ──────────────────────────────────────────────────────────

static const char FILE_TAB_IMU_JS[] = R"WEBEND(let panel, canvas, ctx, currentApi;
let rafPending = false;
let latestTilt = { ax: 0, ay: 0 };

const axes = [
    { id: 'ax', label: 'Accel X', color: '#ff4444', min: -2, max: 2 },
    { id: 'ay', label: 'Accel Y', color: '#44ff44', min: -2, max: 2 },
    { id: 'az', label: 'Accel Z', color: '#4488ff', min: -2, max: 2 },
    { id: 'gx', label: 'Gyro X',  color: '#ff8800', min: -500, max: 500 },
    { id: 'gy', label: 'Gyro Y',  color: '#ff00ff', min: -500, max: 500 },
    { id: 'gz', label: 'Gyro Z',  color: '#00ccff', min: -500, max: 500 },
];

export function initImu(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>IMU Data</h3>
            <div id="imu-bars">
                ${axes.map(a => `
                    <div class="imu-bar-group">
                        <div class="imu-bar-label"><span>${a.label}</span><span id="imu-val-${a.id}">0</span></div>
                        <div class="imu-bar-track"><div class="imu-bar-thumb" id="imu-bar-${a.id}" style="background:${a.color}"></div></div>
                    </div>`).join('')}
            </div>
        </div>
        <div class="card">
            <h3>Rate</h3>
            <div class="form-row">
                <input type="range" id="imu-rate" min="10" max="500" value="100" step="10">
                <span id="imu-rate-val">100ms</span>
                <button id="imu-rate-btn" class="secondary">Set</button>
                <span id="imu-rate-status" style="font-size:12px;color:var(--text-dim)"></span>
            </div>
        </div>
        <div class="card">
            <h3>Tilt Visualization</h3>
            <canvas id="imu-canvas" height="180"></canvas>
        </div>
    `;
    const rateSlider = panel.querySelector('#imu-rate');
    const rateVal = panel.querySelector('#imu-rate-val');
    rateSlider.oninput = () => { rateVal.textContent = rateSlider.value + 'ms'; };
    panel.querySelector('#imu-rate-btn').onclick = () => {
        if (!currentApi) return;
        const status = panel.querySelector('#imu-rate-status');
        currentApi.post('/api/imu/rate', { rate_ms: parseInt(rateSlider.value) }).then(r => {
            status.textContent = `Set to ${r.rate_ms}ms`;
            setTimeout(() => status.textContent = '', 2000);
        }).catch(e => { status.textContent = e.message; });
    };
    canvas = panel.querySelector('#imu-canvas');
    ctx = canvas.getContext('2d');
}

export function activateImu(api) { currentApi = api; api.subscribe('/ws/imu', onImuData); }
export function deactivateImu(api) { api.unsubscribe('/ws/imu'); currentApi = null; }

function onImuData(d) {
    const vals = { ax: d.accel?.x??0, ay: d.accel?.y??0, az: d.accel?.z??0, gx: d.gyro?.x??0, gy: d.gyro?.y??0, gz: d.gyro?.z??0 };
    for (const a of axes) {
        const v = vals[a.id];
        document.getElementById('imu-val-' + a.id).textContent = v.toFixed(2);
        const pct = ((v - a.min) / (a.max - a.min)) * 100;
        const clamped = Math.max(0, Math.min(100, pct));
        const mid = 50;
        const bar = document.getElementById('imu-bar-' + a.id);
        if (clamped >= mid) { bar.style.left = mid + '%'; bar.style.width = (clamped - mid) + '%'; }
        else { bar.style.left = clamped + '%'; bar.style.width = (mid - clamped) + '%'; }
    }
    latestTilt.ax = vals.ax; latestTilt.ay = vals.ay;
    if (!rafPending) { rafPending = true; requestAnimationFrame(() => { rafPending = false; drawTilt(latestTilt.ax, latestTilt.ay); }); }
}

function drawTilt(ax, ay) {
    const w = canvas.clientWidth, h = canvas.clientHeight;
    canvas.width = w * devicePixelRatio; canvas.height = h * devicePixelRatio;
    ctx.setTransform(devicePixelRatio, 0, 0, devicePixelRatio, 0, 0);
    ctx.clearRect(0, 0, w, h);
    const cx = w / 2, cy = h / 2, r = Math.min(cx, cy) - 10;
    ctx.strokeStyle = '#2a2a2a'; ctx.lineWidth = 1; ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI * 2); ctx.stroke();
    ctx.strokeStyle = '#1e1e1e'; ctx.beginPath();
    ctx.moveTo(cx - r, cy); ctx.lineTo(cx + r, cy); ctx.moveTo(cx, cy - r); ctx.lineTo(cx, cy + r); ctx.stroke();
    const dx = Math.max(-1, Math.min(1, ax / 1.5)) * r;
    const dy = Math.max(-1, Math.min(1, ay / 1.5)) * r;
    ctx.fillStyle = '#00ff88'; ctx.shadowColor = '#00ff88'; ctx.shadowBlur = 12;
    ctx.beginPath(); ctx.arc(cx + dx, cy + dy, 8, 0, Math.PI * 2); ctx.fill();
    ctx.shadowBlur = 0;
}
)WEBEND";

// ── js/tab-keyboard.js ─────────────────────────────────────────────────────

static const char FILE_TAB_KEYBOARD_JS[] = R"WEBEND(let panel;
const keyLog = [];
let renderPending = false;

export function initKeyboard(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>Keyboard Events</h3>
            <button id="keys-clear-btn" class="secondary" style="margin-bottom:8px;font-size:11px">Clear</button>
            <div class="log" id="keys-log"><div style="color:var(--text-dim);padding:8px">Press keys on the Cardputer...</div></div>
        </div>
    `;
    panel.querySelector('#keys-clear-btn').onclick = () => { keyLog.length = 0; renderLog(); };
}

export function activateKeyboard(api) { api.subscribe('/ws/keyboard', onKeyEvent); }
export function deactivateKeyboard(api) { api.unsubscribe('/ws/keyboard'); }

function onKeyEvent(d) {
    if (!d || typeof d !== 'object') return;
    const mods = [];
    if (d.shift) mods.push('<span class="badge shift">SHIFT</span>');
    if (d.ctrl)  mods.push('<span class="badge ctrl">CTRL</span>');
    if (d.alt || d.opt) mods.push('<span class="badge alt">ALT</span>');
    if (d.fn)    mods.push('<span class="badge fn">FN</span>');
    let charText = '';
    if (d.char) charText = `"${escHtml(d.char)}"`;
    else if (d.enter) charText = '[ENTER]';
    else if (d.del)   charText = '[DEL]';
    else if (d.tab)   charText = '[TAB]';
    else if (d.space) charText = '[SPACE]';
    keyLog.unshift({ time: new Date().toLocaleTimeString(), char: charText, mods: mods.join('') });
    if (keyLog.length > 50) keyLog.length = 50;
    if (!renderPending) { renderPending = true; requestAnimationFrame(() => { renderPending = false; renderLog(); }); }
}

function renderLog() {
    const el = panel?.querySelector('#keys-log');
    if (!el) return;
    if (keyLog.length === 0) { el.innerHTML = '<div style="color:var(--text-dim);padding:8px">Press keys on the Cardputer...</div>'; return; }
    el.innerHTML = keyLog.map(e => `<div class="log-entry"><span class="log-time">${e.time}</span>${e.mods} <strong>${e.char}</strong></div>`).join('');
}

function escHtml(s) { return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }
)WEBEND";

// ── js/tab-ir.js ───────────────────────────────────────────────────────────

static const char FILE_TAB_IR_JS[] = R"WEBEND(let panel, currentApi;
const irHistory = [];

export function initIr(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>IR Transmitter</h3>
            <div class="form-row">
                <label>Protocol</label>
                <select id="ir-protocol">
                    <option value="nec">NEC</option>
                    <option value="nec_ext">NEC Extended</option>
                    <option value="onkyo">Onkyo (NEC16)</option>
                </select>
            </div>
            <div class="form-row">
                <label>Address</label><input type="text" id="ir-address" placeholder="0x00" style="width:80px">
                <label>Command</label><input type="text" id="ir-command" placeholder="0x00" style="width:80px">
            </div>
            <div class="form-row">
                <label>Repeats</label>
                <input type="number" id="ir-repeats" value="0" min="0" max="10" style="width:60px">
            </div>
            <div class="form-row">
                <button id="ir-send-btn">Send IR</button>
                <span id="ir-status" style="font-size:12px;color:var(--text-dim)"></span>
            </div>
        </div>
        <div class="card">
            <h3>History</h3>
            <button id="ir-clear-btn" class="secondary" style="margin-bottom:8px;font-size:11px">Clear</button>
            <div class="log" id="ir-log"><div style="color:var(--text-dim);padding:8px">No IR codes sent yet</div></div>
        </div>
    `;
    panel.querySelector('#ir-send-btn').onclick = () => {
        if (!currentApi) return;
        const protocol = panel.querySelector('#ir-protocol').value;
        const address = panel.querySelector('#ir-address').value.trim();
        const command = panel.querySelector('#ir-command').value.trim();
        const repeats = parseInt(panel.querySelector('#ir-repeats').value) || 0;
        const status = panel.querySelector('#ir-status');
        if (!address || !command) { status.textContent = 'Enter address and command'; return; }
        status.textContent = 'Sending...';
        currentApi.post('/api/ir/send', { protocol, address, command, repeats }).then(r => {
            status.textContent = 'Sent!';
            irHistory.unshift({ time: new Date().toLocaleTimeString(), protocol: r.protocol,
                address: '0x' + r.address.toString(16).toUpperCase(), command: '0x' + r.command.toString(16).toUpperCase(), repeats: r.repeats });
            if (irHistory.length > 50) irHistory.pop();
            renderLog();
            setTimeout(() => status.textContent = '', 2000);
        }).catch(e => { status.textContent = 'Error: ' + e.message; });
    };
    panel.querySelector('#ir-clear-btn').onclick = () => { irHistory.length = 0; renderLog(); };
}

export function activateIr(api) { currentApi = api; }
export function deactivateIr() { currentApi = null; }

function renderLog() {
    const el = panel.querySelector('#ir-log');
    if (irHistory.length === 0) { el.innerHTML = '<div style="color:var(--text-dim);padding:8px">No IR codes sent yet</div>'; return; }
    el.innerHTML = irHistory.map(e => `<div class="log-entry"><span class="log-time">${e.time}</span>
        <strong>${e.protocol.toUpperCase()}</strong> addr=${e.address} cmd=${e.command} rpt=${e.repeats}</div>`).join('');
}
)WEBEND";

// ── js/tab-display.js ──────────────────────────────────────────────────────

static const char FILE_TAB_DISPLAY_JS[] = R"WEBEND(let panel, currentApi;

export function initDisplay(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>Draw Text</h3>
            <div class="form-row"><input type="text" id="disp-text" placeholder="Hello!" style="flex:1"></div>
            <div class="form-row">
                <label>X</label><input type="number" id="disp-x" value="0" min="0" style="width:60px">
                <label>Y</label><input type="number" id="disp-y" value="0" min="0" style="width:60px">
                <label>Size</label><input type="number" id="disp-size" value="2" min="1" max="8" style="width:50px">
            </div>
            <div class="form-row">
                <label>Color</label><input type="color" id="disp-text-color" value="#ffffff">
                <button id="disp-text-btn">Send Text</button>
                <span id="disp-text-status" style="font-size:12px;color:var(--text-dim)"></span>
            </div>
        </div>
        <div class="card">
            <h3>Clear Screen</h3>
            <div class="form-row">
                <label>Color</label><input type="color" id="disp-clear-color" value="#000000">
                <button id="disp-clear-btn" class="danger">Clear Screen</button>
            </div>
        </div>
        <div class="card">
            <h3>Fill Rectangle</h3>
            <div class="form-row">
                <label>X</label><input type="number" id="disp-fill-x" value="0" style="width:55px">
                <label>Y</label><input type="number" id="disp-fill-y" value="0" style="width:55px">
                <label>W</label><input type="number" id="disp-fill-w" value="50" style="width:55px">
                <label>H</label><input type="number" id="disp-fill-h" value="30" style="width:55px">
            </div>
            <div class="form-row">
                <label>Color</label><input type="color" id="disp-fill-color" value="#00ff88">
                <button id="disp-fill-btn">Fill Rect</button>
            </div>
        </div>
    `;
    panel.querySelector('#disp-text-btn').onclick = () => {
        if (!currentApi) return;
        const status = panel.querySelector('#disp-text-status');
        currentApi.post('/api/display/text', {
            text: panel.querySelector('#disp-text').value,
            x: parseInt(panel.querySelector('#disp-x').value),
            y: parseInt(panel.querySelector('#disp-y').value),
            size: parseInt(panel.querySelector('#disp-size').value),
            color: panel.querySelector('#disp-text-color').value,
        }).then(() => { status.textContent = 'Done'; setTimeout(() => status.textContent = '', 2000); })
          .catch(e => { status.textContent = e.message; });
    };
    panel.querySelector('#disp-clear-btn').onclick = () => {
        if (!currentApi) return;
        currentApi.post('/api/display/clear', { color: panel.querySelector('#disp-clear-color').value });
    };
    panel.querySelector('#disp-fill-btn').onclick = () => {
        if (!currentApi) return;
        currentApi.post('/api/display/fill', {
            x: parseInt(panel.querySelector('#disp-fill-x').value),
            y: parseInt(panel.querySelector('#disp-fill-y').value),
            w: parseInt(panel.querySelector('#disp-fill-w').value),
            h: parseInt(panel.querySelector('#disp-fill-h').value),
            color: panel.querySelector('#disp-fill-color').value,
        });
    };
}

export function activateDisplay(api) { currentApi = api; }
export function deactivateDisplay() { currentApi = null; }
)WEBEND";

// ── js/tab-gpio.js ─────────────────────────────────────────────────────────

static const char FILE_TAB_GPIO_JS[] = R"WEBEND(let panel, currentApi;
const gpioLog = [];

export function initGpio(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>GPIO Control</h3>
            <div class="form-row">
                <label>Pin</label>
                <input type="number" id="gpio-pin" value="0" min="0" max="48" style="width:60px">
            </div>
            <div class="form-row">
                <label>Mode</label>
                <select id="gpio-mode">
                    <option value="input">INPUT</option>
                    <option value="output">OUTPUT</option>
                    <option value="input_pullup">INPUT_PULLUP</option>
                    <option value="input_pulldown">INPUT_PULLDOWN</option>
                </select>
                <button id="gpio-mode-btn" class="secondary">Set Mode</button>
            </div>
            <div class="form-row">
                <button id="gpio-read-btn" class="secondary">Read</button>
                <span id="gpio-read-val" style="font-family:monospace;font-size:14px">--</span>
            </div>
            <div class="form-row">
                <label>Write</label>
                <div class="toggle-group">
                    <button id="gpio-low" class="active">LOW</button>
                    <button id="gpio-high">HIGH</button>
                </div>
                <button id="gpio-write-btn">Write</button>
            </div>
        </div>
        <div class="card">
            <h3>Activity Log</h3>
            <button id="gpio-clear-btn" class="secondary" style="margin-bottom:8px;font-size:11px">Clear</button>
            <div class="log" id="gpio-log"><div style="color:var(--text-dim);padding:8px">No activity yet</div></div>
        </div>
    `;
    let writeVal = 0;
    panel.querySelector('#gpio-low').onclick  = () => { writeVal = 0; panel.querySelector('#gpio-low').classList.add('active');  panel.querySelector('#gpio-high').classList.remove('active'); };
    panel.querySelector('#gpio-high').onclick = () => { writeVal = 1; panel.querySelector('#gpio-high').classList.add('active'); panel.querySelector('#gpio-low').classList.remove('active'); };
    panel.querySelector('#gpio-mode-btn').onclick = () => {
        if (!currentApi) return;
        currentApi.post(`/api/gpio/${panel.querySelector('#gpio-pin').value}/mode`, { mode: panel.querySelector('#gpio-mode').value })
            .then(r => addLog(`Pin ${r.pin} mode set to ${r.mode}`)).catch(e => addLog(`Error: ${e.message}`));
    };
    panel.querySelector('#gpio-read-btn').onclick = () => {
        if (!currentApi) return;
        currentApi.get(`/api/gpio/${panel.querySelector('#gpio-pin').value}`)
            .then(r => { panel.querySelector('#gpio-read-val').textContent = r.value ? 'HIGH (1)' : 'LOW (0)'; addLog(`Pin ${r.pin} = ${r.value}`); })
            .catch(e => addLog(`Error: ${e.message}`));
    };
    panel.querySelector('#gpio-write-btn').onclick = () => {
        if (!currentApi) return;
        currentApi.post(`/api/gpio/${panel.querySelector('#gpio-pin').value}`, { value: writeVal })
            .then(r => addLog(`Pin ${r.pin} written ${r.value ? 'HIGH' : 'LOW'}`)).catch(e => addLog(`Error: ${e.message}`));
    };
    panel.querySelector('#gpio-clear-btn').onclick = () => { gpioLog.length = 0; renderLog(); };
}

export function activateGpio(api) { currentApi = api; }
export function deactivateGpio() { currentApi = null; }

function addLog(msg) { gpioLog.unshift({ time: new Date().toLocaleTimeString(), msg }); if (gpioLog.length > 100) gpioLog.pop(); renderLog(); }

function renderLog() {
    const el = panel.querySelector('#gpio-log');
    if (gpioLog.length === 0) { el.innerHTML = '<div style="color:var(--text-dim);padding:8px">No activity yet</div>'; return; }
    el.innerHTML = gpioLog.map(e => `<div class="log-entry"><span class="log-time">${e.time}</span>${e.msg}</div>`).join('');
}
)WEBEND";

// ── js/tab-audio.js ────────────────────────────────────────────────────────

static const char FILE_TAB_AUDIO_JS[] = R"WEBEND(let panel, currentApi;

export function initAudio(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>Tone Generator</h3>
            <div class="form-row">
                <label>Frequency</label>
                <input type="range" id="audio-freq" min="20" max="8000" value="440" step="1">
                <span id="audio-freq-val" style="font-family:monospace;min-width:55px">440 Hz</span>
            </div>
            <div class="preset-row">
                <button class="secondary" data-freq="262">C4</button>
                <button class="secondary" data-freq="330">E4</button>
                <button class="secondary" data-freq="440">A4</button>
                <button class="secondary" data-freq="523">C5</button>
                <button class="secondary" data-freq="659">E5</button>
                <button class="secondary" data-freq="880">A5</button>
                <button class="secondary" data-freq="1000">1kHz</button>
            </div>
            <div class="form-row">
                <label>Duration</label>
                <input type="range" id="audio-dur" min="50" max="5000" value="500" step="50">
                <span id="audio-dur-val" style="font-family:monospace;min-width:55px">500ms</span>
            </div>
            <div class="form-row">
                <button id="audio-play-btn">Play</button>
                <button id="audio-stop-btn" class="danger">Stop</button>
                <span id="audio-status" style="font-size:12px;color:var(--text-dim)"></span>
            </div>
        </div>
        <div class="card">
            <h3>Volume</h3>
            <div class="form-row">
                <input type="range" id="audio-vol" min="0" max="255" value="128">
                <span id="audio-vol-val" style="font-family:monospace;min-width:30px">128</span>
                <button id="audio-vol-btn" class="secondary">Set</button>
            </div>
        </div>
    `;
    const freqSlider = panel.querySelector('#audio-freq');
    const freqVal    = panel.querySelector('#audio-freq-val');
    const durSlider  = panel.querySelector('#audio-dur');
    const durVal     = panel.querySelector('#audio-dur-val');
    const volSlider  = panel.querySelector('#audio-vol');
    const volVal     = panel.querySelector('#audio-vol-val');

    freqSlider.oninput = () => { freqVal.textContent = freqSlider.value + ' Hz'; };
    durSlider.oninput  = () => { durVal.textContent  = durSlider.value  + 'ms'; };
    volSlider.oninput  = () => { volVal.textContent  = volSlider.value; };

    panel.querySelector('.preset-row').addEventListener('click', (e) => {
        const btn = e.target.closest('[data-freq]');
        if (!btn) return;
        freqSlider.value = btn.dataset.freq;
        freqVal.textContent = btn.dataset.freq + ' Hz';
    });

    panel.querySelector('#audio-play-btn').onclick = () => {
        if (!currentApi) return;
        const status = panel.querySelector('#audio-status');
        currentApi.post('/api/audio/tone', { freq: parseInt(freqSlider.value), duration: parseInt(durSlider.value) })
            .then(() => { status.textContent = 'Playing'; setTimeout(() => status.textContent = '', parseInt(durSlider.value)); })
            .catch(e => { status.textContent = e.message; });
    };
    panel.querySelector('#audio-stop-btn').onclick = () => {
        if (!currentApi) return;
        currentApi.post('/api/audio/stop', {}).then(() => {
            panel.querySelector('#audio-status').textContent = 'Stopped';
            setTimeout(() => panel.querySelector('#audio-status').textContent = '', 2000);
        });
    };
    panel.querySelector('#audio-vol-btn').onclick = () => {
        if (!currentApi) return;
        currentApi.post('/api/audio/volume', { volume: parseInt(volSlider.value) });
    };
}

export function activateAudio(api) { currentApi = api; }
export function deactivateAudio() { currentApi = null; }
)WEBEND";

// ── js/tab-record.js ───────────────────────────────────────────────────────

static const char FILE_TAB_RECORD_JS[] = R"WEBEND(let panel;
let mediaRecorder = null;
let audioCtx = null;
let analyser = null;
let animFrame = null;
let chunks = [];
let recordings = [];
let startTime = null;
let timerInterval = null;

export function initRecord(el) {
    panel = el;
    const hostname = location.hostname || '192.168.4.1';
    const httpsNotice = !window.isSecureContext ? `
        <div style="background:#2a1500;border:1px solid #ff8000;border-radius:6px;padding:10px;margin-bottom:4px;font-size:12px;line-height:1.6">
            <b style="color:#ff8000">&#9888; Microphone requires HTTPS</b><br>
            Browsers block mic access over plain HTTP. To enable in <b>Chrome</b>:<br>
            1. Open <code>chrome://flags/#unsafely-treat-insecure-origin-as-secure</code><br>
            2. Add <code>http://${hostname}</code> and relaunch Chrome<br>
            <span style="color:var(--text-dim)">Firefox: about:config \u2192 set <code>media.devices.insecure.enabled</code> true</span>
        </div>` : '';
    panel.innerHTML = `
        <div class="card">
            <h3>Microphone Recording</h3>
            ${httpsNotice}
            <canvas id="rec-waveform" height="80"></canvas>
            <div class="form-row" style="margin-top:8px">
                <button id="rec-btn">Record</button>
                <button id="rec-stop-btn" class="danger" disabled>Stop</button>
                <span id="rec-timer" style="font-family:monospace;font-size:13px;min-width:50px;color:var(--text-dim)">0:00</span>
                <span id="rec-status" style="font-size:12px;color:var(--text-dim);flex:1;text-align:right"></span>
            </div>
        </div>
        <div class="card" id="rec-list-card" style="display:none">
            <h3>Recordings</h3>
            <div id="rec-list"></div>
        </div>
    `;
    panel.querySelector('#rec-btn').onclick = startRecording;
    panel.querySelector('#rec-stop-btn').onclick = stopRecording;
    drawIdleWaveform();
}

export function activateRecord() {}
export function deactivateRecord() {
    if (mediaRecorder && mediaRecorder.state !== 'inactive') mediaRecorder.stop();
    stopViz();
}

async function startRecording() {
    const status = panel.querySelector('#rec-status');
    let stream;
    try { stream = await navigator.mediaDevices.getUserMedia({ audio: true, video: false }); }
    catch (e) {
        if (e.name === 'NotAllowedError' || e.name === 'PermissionDeniedError')
            status.textContent = !window.isSecureContext ? 'Blocked: HTTP origin \u2014 see instructions above' : 'Permission denied \u2014 allow mic in browser settings';
        else if (e.name === 'NotFoundError') status.textContent = 'No microphone found';
        else status.textContent = 'Mic error: ' + e.message;
        return;
    }
    chunks = []; startTime = Date.now();
    audioCtx = new AudioContext(); analyser = audioCtx.createAnalyser(); analyser.fftSize = 256;
    audioCtx.createMediaStreamSource(stream).connect(analyser);
    drawLiveWaveform(); startTimer();
    const mimeType = getSupportedMime();
    mediaRecorder = new MediaRecorder(stream, mimeType ? { mimeType } : undefined);
    mediaRecorder.ondataavailable = e => { if (e.data.size > 0) chunks.push(e.data); };
    mediaRecorder.onstop = () => { stream.getTracks().forEach(t => t.stop()); stopViz(); saveRecording(mimeType); };
    mediaRecorder.start(250);
    panel.querySelector('#rec-btn').disabled = true;
    panel.querySelector('#rec-stop-btn').disabled = false;
    status.textContent = 'Recording\u2026';
}

function stopRecording() {
    if (mediaRecorder && mediaRecorder.state !== 'inactive') mediaRecorder.stop();
    panel.querySelector('#rec-btn').disabled = false;
    panel.querySelector('#rec-stop-btn').disabled = true;
    panel.querySelector('#rec-status').textContent = '';
    panel.querySelector('#rec-timer').textContent = '0:00';
    clearInterval(timerInterval);
}

function saveRecording(mimeType) {
    const blob = new Blob(chunks, { type: mimeType || 'audio/webm' });
    const url = URL.createObjectURL(blob);
    const ext = mimeType && mimeType.includes('ogg') ? 'ogg' : 'webm';
    const duration = ((Date.now() - startTime) / 1000).toFixed(1);
    const name = `rec-${new Date().toISOString().replace(/[:.]/g, '-').slice(0, -5)}.${ext}`;
    recordings.unshift({ blob, url, name, duration });
    renderRecordingList();
}

function renderRecordingList() {
    const card = panel.querySelector('#rec-list-card');
    const list = panel.querySelector('#rec-list');
    if (recordings.length === 0) { card.style.display = 'none'; return; }
    card.style.display = '';
    list.innerHTML = '';
    recordings.forEach((rec, i) => {
        const row = document.createElement('div');
        row.style.cssText = 'display:flex;align-items:center;gap:8px;padding:6px 0;border-bottom:1px solid var(--border)';
        row.innerHTML = `
            <span style="font-family:monospace;font-size:12px;flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap">${rec.name}</span>
            <span style="font-size:11px;color:var(--text-dim);white-space:nowrap">${rec.duration}s</span>
            <audio controls src="${rec.url}" style="height:28px;flex-shrink:0"></audio>
            <a href="${rec.url}" download="${rec.name}" style="flex-shrink:0">
                <button class="secondary" style="padding:4px 10px;font-size:12px">&#8595;</button>
            </a>
            <button class="danger" data-idx="${i}" style="padding:4px 10px;font-size:12px">\u2715</button>
        `;
        row.querySelector('[data-idx]').onclick = () => { URL.revokeObjectURL(recordings[i].url); recordings.splice(i, 1); renderRecordingList(); };
        list.appendChild(row);
    });
}

function drawIdleWaveform() {
    const canvas = panel.querySelector('#rec-waveform');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    canvas.width = canvas.offsetWidth || 300;
    ctx.fillStyle = '#1e1e1e'; ctx.fillRect(0, 0, canvas.width, canvas.height);
    ctx.strokeStyle = '#333'; ctx.lineWidth = 1.5;
    ctx.beginPath(); ctx.moveTo(0, canvas.height / 2); ctx.lineTo(canvas.width, canvas.height / 2); ctx.stroke();
}

function drawLiveWaveform() {
    const canvas = panel.querySelector('#rec-waveform');
    if (!canvas || !analyser) return;
    canvas.width = canvas.offsetWidth || 300;
    const ctx = canvas.getContext('2d');
    const buf = new Uint8Array(analyser.frequencyBinCount);
    function frame() {
        animFrame = requestAnimationFrame(frame);
        analyser.getByteTimeDomainData(buf);
        const w = canvas.width, h = canvas.height;
        ctx.fillStyle = '#1e1e1e'; ctx.fillRect(0, 0, w, h);
        ctx.strokeStyle = '#00ff88'; ctx.lineWidth = 1.5;
        ctx.beginPath();
        const step = w / buf.length;
        for (let i = 0; i < buf.length; i++) {
            const y = (buf[i] / 128.0) * (h / 2);
            i === 0 ? ctx.moveTo(0, y) : ctx.lineTo(i * step, y);
        }
        ctx.stroke();
    }
    frame();
}

function stopViz() {
    if (animFrame) { cancelAnimationFrame(animFrame); animFrame = null; }
    if (audioCtx) { audioCtx.close(); audioCtx = null; analyser = null; }
    drawIdleWaveform();
}

function startTimer() {
    clearInterval(timerInterval);
    timerInterval = setInterval(() => {
        if (!startTime) { clearInterval(timerInterval); return; }
        const el = panel.querySelector('#rec-timer');
        if (!el) { clearInterval(timerInterval); return; }
        const s = Math.floor((Date.now() - startTime) / 1000);
        el.textContent = `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`;
        if (s >= 300) stopRecording();
    }, 1000);
}

function getSupportedMime() {
    const candidates = ['audio/webm;codecs=opus', 'audio/webm', 'audio/ogg;codecs=opus', 'audio/ogg'];
    return candidates.find(m => MediaRecorder.isTypeSupported(m)) || '';
}
)WEBEND";

// ── js/tab-grove.js ────────────────────────────────────────────────────────

static const char FILE_TAB_GROVE_JS[] = R"WEBEND(let panel, currentApi;
let sensors = [];
let activeSensorId = '';
let sse = null;
const groveLog = [];

export function initGrove(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>Grove Sensor</h3>
            <div class="form-row">
                <label>Type</label>
                <select id="grove-type" style="flex:1">
                    <option value="">Connect to load sensors</option>
                </select>
            </div>
            <div id="grove-safety" style="display:none;border-radius:6px;padding:10px 12px;margin-top:8px;font-size:12px;line-height:1.5"></div>
            <div id="grove-pins" style="margin-top:8px;font-size:12px;color:var(--text-dim)"></div>
            <div class="form-row" style="margin-top:10px">
                <button id="grove-cfg-btn">Configure</button>
            </div>
        </div>
        <div class="card">
            <h3>Reading</h3>
            <div class="form-row">
                <button id="grove-read-btn" class="secondary">Read Once</button>
                <button id="grove-stream-btn" class="secondary">&#9654; Stream</button>
            </div>
            <div id="grove-value" style="font-size:16px;font-family:monospace;padding:10px 0;min-height:46px;color:var(--accent)">&#8212;</div>
            <div id="grove-digital-row" class="form-row" style="display:none">
                <label>Output</label>
                <div class="toggle-group">
                    <button id="grove-low-btn" class="active">LOW</button>
                    <button id="grove-high-btn">HIGH</button>
                </div>
                <button id="grove-write-btn">Write</button>
            </div>
            <div id="grove-pwm-row" class="form-row" style="display:none">
                <label>Duty</label>
                <input type="range" id="grove-duty" min="0" max="255" value="128" style="flex:1">
                <span id="grove-duty-val" style="font-family:monospace;min-width:28px">128</span>
                <button id="grove-pwm-btn">Set</button>
            </div>
            <div id="grove-rotary-ctrl" style="display:none;margin-top:8px">
                <button id="grove-rotary-reset" class="secondary">Reset Steps</button>
            </div>
        </div>
        <div class="card">
            <h3>Log</h3>
            <button id="grove-log-clear" class="secondary" style="margin-bottom:8px;font-size:11px">Clear</button>
            <div class="log" id="grove-log-el">
                <div style="color:var(--text-dim);padding:8px">No readings yet</div>
            </div>
        </div>
    `;

    const sel = panel.querySelector('#grove-type');
    sel.addEventListener('change', () => updateSafetyInfo(sel.value));

    let digitalWriteVal = 0;
    panel.querySelector('#grove-low-btn').onclick = () => {
        digitalWriteVal = 0;
        panel.querySelector('#grove-low-btn').classList.add('active');
        panel.querySelector('#grove-high-btn').classList.remove('active');
    };
    panel.querySelector('#grove-high-btn').onclick = () => {
        digitalWriteVal = 1;
        panel.querySelector('#grove-high-btn').classList.add('active');
        panel.querySelector('#grove-low-btn').classList.remove('active');
    };

    const dutySlider = panel.querySelector('#grove-duty');
    const dutyValEl  = panel.querySelector('#grove-duty-val');
    dutySlider.addEventListener('input', () => { dutyValEl.textContent = dutySlider.value; });

    panel.querySelector('#grove-cfg-btn').onclick = () => {
        if (!currentApi) return;
        const id = sel.value;
        if (!id) return;
        stopStream();
        currentApi.post('/api/grove/configure', { sensor: id }).then(r => {
            activeSensorId = r.sensor;
            showOutputControls(r.sensor);
            addLog('Configured: ' + r.name + ' (D=GPIO' + r.pin_d + (r.uses_d2 ? ', D2=GPIO' + r.pin_d2 : '') + ')');
        }).catch(e => addLog('Error: ' + e.message));
    };

    panel.querySelector('#grove-read-btn').onclick = () => {
        if (!currentApi) return;
        currentApi.get('/api/grove/read').then(r => {
            renderReading(r);
            addLog(readingToString(r));
        }).catch(e => addLog('Error: ' + e.message));
    };

    panel.querySelector('#grove-stream-btn').onclick = () => {
        if (sse) stopStream();
        else startStream();
    };

    panel.querySelector('#grove-write-btn').onclick = () => {
        if (!currentApi) return;
        currentApi.post('/api/grove/write', { value: digitalWriteVal })
            .then(() => addLog('Wrote: ' + (digitalWriteVal ? 'HIGH' : 'LOW')))
            .catch(e => addLog('Error: ' + e.message));
    };

    panel.querySelector('#grove-pwm-btn').onclick = () => {
        if (!currentApi) return;
        const duty = parseInt(dutySlider.value);
        currentApi.post('/api/grove/write', { duty })
            .then(() => addLog('PWM duty: ' + duty))
            .catch(e => addLog('Error: ' + e.message));
    };

    panel.querySelector('#grove-rotary-reset').onclick = () => {
        if (!currentApi) return;
        currentApi.post('/api/grove/rotary/reset', {})
            .then(() => {
                addLog('Rotary steps reset');
                renderReading({ sensor: 'rotary', steps: 0, ts: Date.now() });
            })
            .catch(e => addLog('Error: ' + e.message));
    };

    panel.querySelector('#grove-log-clear').onclick = () => {
        groveLog.length = 0;
        renderLog();
    };
}

export function activateGrove(api) {
    currentApi = api;
    loadSensors();
    api.get('/api/grove/config').then(r => {
        if (r.sensor) {
            activeSensorId = r.sensor;
            const sel = panel.querySelector('#grove-type');
            sel.value = r.sensor;
            updateSafetyInfo(r.sensor);
            showOutputControls(r.sensor);
        }
    }).catch(() => {});
}

export function deactivateGrove() {
    stopStream();
    currentApi = null;
}

function loadSensors() {
    if (!currentApi) return;
    currentApi.get('/api/grove/sensors').then(data => {
        sensors = data.sensors || [];
        const sel = panel.querySelector('#grove-type');
        sel.innerHTML = '<option value="">Select sensor type…</option>' +
            sensors.map(s =>
                '<option value="' + s.id + '">' + s.name + ' (' + s.vcc + (s.gpio_safe ? '' : ' ⚠') + ')</option>'
            ).join('');
        if (activeSensorId) {
            sel.value = activeSensorId;
            updateSafetyInfo(activeSensorId);
        }
    }).catch(() => {});
}

function updateSafetyInfo(id) {
    const el     = panel.querySelector('#grove-safety');
    const pinsEl = panel.querySelector('#grove-pins');
    if (!id) { el.style.display = 'none'; pinsEl.textContent = ''; return; }
    const s = sensors.find(x => x.id === id);
    if (!s) return;

    if (currentApi) {
        currentApi.get('/api/grove/config').then(r => {
            pinsEl.textContent = 'D = GPIO' + r.pin_d + ' (yellow)' +
                (s.uses_d2 ? '  |  D2 = GPIO' + r.pin_d2 + ' (white)' : '');
        }).catch(() => {
            pinsEl.textContent = 'D: ' + s.d_role + (s.uses_d2 ? '  |  D2: ' + s.d2_role : '');
        });
    }

    if (s.gpio_safe) {
        el.style.cssText = 'display:block;background:#0d2818;border:1px solid var(--accent);border-radius:6px;padding:10px 12px;margin-top:8px;font-size:12px;line-height:1.5';
        el.innerHTML = '<strong style="color:var(--accent)">✓ GPIO Safe — ' + s.vcc + '</strong><br>' + s.voltage_note;
    } else {
        el.style.cssText = 'display:block;background:#2a0d0d;border:1px solid var(--red);border-radius:6px;padding:10px 12px;margin-top:8px;font-size:12px;line-height:1.5';
        el.innerHTML = '<strong style="color:var(--red)">⚠ Caution — ' + s.vcc + '</strong><br>' + s.voltage_note;
    }
}

function showOutputControls(id) {
    panel.querySelector('#grove-digital-row').style.display = id === 'digital_out' ? 'flex' : 'none';
    panel.querySelector('#grove-pwm-row').style.display     = id === 'pwm_out'     ? 'flex' : 'none';
    panel.querySelector('#grove-rotary-ctrl').style.display = id === 'rotary'      ? 'block' : 'none';
}

function startStream() {
    if (!currentApi || sse) return;
    const btn = panel.querySelector('#grove-stream-btn');
    btn.textContent = '■ Stop';
    btn.classList.remove('secondary');
    sse = new EventSource('http://' + currentApi.ip + '/api/grove/stream');
    sse.addEventListener('reading', e => {
        const r = JSON.parse(e.data);
        renderReading(r);
        addLog(readingToString(r));
    });
    sse.onerror = () => { stopStream(); addLog('Stream closed'); };
    addLog('Stream started');
}

function stopStream() {
    if (sse) { sse.close(); sse = null; }
    const btn = panel.querySelector('#grove-stream-btn');
    if (btn) { btn.textContent = '▶ Stream'; btn.classList.add('secondary'); }
}

function renderReading(r) {
    const el = panel.querySelector('#grove-value');
    if (r.error) { el.style.color = 'var(--red)'; el.textContent = r.error; return; }
    el.style.color = 'var(--accent)';
    const parts = [];
    if (r.label        !== undefined) parts.push(r.label);
    else if (r.value   !== undefined) parts.push((r.value ? 'HIGH' : 'LOW') + ' (' + r.value + ')');
    if (r.raw          !== undefined) parts.push('raw: ' + r.raw + '  ' + r.voltage + 'V');
    if (r.temperature  !== undefined) parts.push(r.temperature + '°C');
    if (r.humidity     !== undefined) parts.push(r.humidity + '%RH');
    if (r.distance_cm  !== undefined) parts.push(r.distance_cm + ' cm');
    if (r.steps        !== undefined) parts.push('Steps: ' + r.steps);
    if (r.note         !== undefined) parts.push(r.note);
    el.textContent = parts.join('  |  ') || '—';
}

function readingToString(r) {
    if (r.error) return 'Error: ' + r.error;
    const parts = [];
    if (r.label        !== undefined) parts.push(r.label);
    else if (r.value   !== undefined) parts.push('val=' + r.value);
    if (r.raw          !== undefined) parts.push('raw=' + r.raw + ' ' + r.voltage + 'V');
    if (r.temperature  !== undefined) parts.push('temp=' + r.temperature + '°C');
    if (r.humidity     !== undefined) parts.push('hum=' + r.humidity + '%');
    if (r.distance_cm  !== undefined) parts.push('dist=' + r.distance_cm + 'cm');
    if (r.steps        !== undefined) parts.push('steps=' + r.steps);
    return parts.join(' ') || JSON.stringify(r);
}

function addLog(msg) {
    groveLog.unshift({ time: new Date().toLocaleTimeString(), msg });
    if (groveLog.length > 100) groveLog.pop();
    renderLog();
}

function renderLog() {
    const el = panel.querySelector('#grove-log-el');
    if (groveLog.length === 0) {
        el.innerHTML = '<div style="color:var(--text-dim);padding:8px">No readings yet</div>';
        return;
    }
    el.innerHTML = groveLog.map(e =>
        '<div class="log-entry"><span class="log-time">' + e.time + '</span>' + e.msg + '</div>'
    ).join('');
}
)WEBEND";

// ── Route registration ─────────────────────────────────────────────────────

void setupWebApp() {
    // Root — serve the web app index
    apiServer.http().on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/html", FILE_INDEX);
    });

    apiServer.http().on("/css/style.css", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "text/css", FILE_STYLE_CSS);
    });

    apiServer.http().on("/js/api.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_API_JS);
    });
    apiServer.http().on("/js/app.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_APP_JS);
    });
    apiServer.http().on("/js/tab-system.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_SYSTEM_JS);
    });
    apiServer.http().on("/js/tab-gps.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_GPS_JS);
    });
    apiServer.http().on("/js/tab-lora.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_LORA_JS);
    });
    apiServer.http().on("/js/tab-imu.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_IMU_JS);
    });
    apiServer.http().on("/js/tab-keyboard.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_KEYBOARD_JS);
    });
    apiServer.http().on("/js/tab-ir.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_IR_JS);
    });
    apiServer.http().on("/js/tab-display.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_DISPLAY_JS);
    });
    apiServer.http().on("/js/tab-gpio.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_GPIO_JS);
    });
    apiServer.http().on("/js/tab-audio.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_AUDIO_JS);
    });
    apiServer.http().on("/js/tab-record.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_RECORD_JS);
    });
    apiServer.http().on("/js/tab-grove.js", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->send(200, "application/javascript", FILE_TAB_GROVE_JS);
    });

    Serial.println("[App] Web app routes registered — browse to http://192.168.4.1/");
}
