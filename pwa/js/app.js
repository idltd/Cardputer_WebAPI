import { CardputerAPI } from './api.js';
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

// Connection management
const ipInput = document.getElementById('ip-input');
const connectBtn = document.getElementById('connect-btn');
const statusDot = document.getElementById('status-dot');
const statusText = document.getElementById('status-text');
const sysSummary = document.getElementById('sys-summary');

// When served from the Cardputer itself, always use that host's IP and auto-connect.
// When opened standalone (file:// or localhost), restore the last-used IP.
const savedIp = localStorage.getItem('cardputer-ip');
const servedFromDevice = location.hostname !== '' &&
    location.hostname !== 'localhost' &&
    location.hostname !== '127.0.0.1';

if (servedFromDevice) {
    ipInput.value = location.hostname;
} else if (savedIp) {
    ipInput.value = savedIp;
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

    if (connected) {
        startSysPolling();
        tabs[activeTab].activate(api);
    } else {
        stopSysPolling();
        sysSummary.classList.add('hidden');
        tabs[activeTab].deactivate(api);
    }
};

connectBtn.addEventListener('click', () => {
    if (api.connected) {
        api.disconnect();
    } else {
        const ip = ipInput.value.trim();
        localStorage.setItem('cardputer-ip', ip);
        statusDot.className = 'dot connecting';
        statusText.textContent = 'Connecting...';
        api.connect(ip).catch(e => {
            statusText.textContent = e.message || 'Connection failed';
        });
    }
});

// Allow Enter key to connect
ipInput.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' && !api.connected) connectBtn.click();
});

function startSysPolling() {
    pollSysInfo();
    sysInfoInterval = setInterval(pollSysInfo, 5000);
}

function stopSysPolling() {
    if (sysInfoInterval) { clearInterval(sysInfoInterval); sysInfoInterval = null; }
}

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

// Tab switching
document.getElementById('tab-bar').addEventListener('click', (e) => {
    const btn = e.target.closest('.tab');
    if (!btn) return;
    const name = btn.dataset.tab;
    if (name === activeTab) return;

    // Deactivate old
    if (api.connected) tabs[activeTab].deactivate(api);
    const oldTab = document.querySelector('.tab.active');
    oldTab.classList.remove('active');
    oldTab.setAttribute('aria-selected', 'false');
    document.querySelector('.tab-panel.active').classList.remove('active');

    // Activate new
    activeTab = name;
    btn.classList.add('active');
    btn.setAttribute('aria-selected', 'true');
    document.getElementById('tab-' + name).classList.add('active');
    if (api.connected) tabs[activeTab].activate(api);
});

// Initialize all tabs (render static HTML)
for (const [name, tab] of Object.entries(tabs)) {
    tab.init(document.getElementById('tab-' + name), api);
}

// Cleanup on page unload
window.addEventListener('beforeunload', () => {
    if (api.connected) api.disconnect();
});
