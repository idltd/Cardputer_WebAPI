let panel, currentApi;
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
                <select id="lora-sf">
                    ${[7,8,9,10,11,12].map(v => `<option value="${v}" ${v===7?'selected':''}>${v}</option>`).join('')}
                </select>
                <label>BW (kHz)</label>
                <select id="lora-bw">
                    ${[7.8,10.4,15.6,20.8,31.25,41.7,62.5,125,250,500].map(v => `<option value="${v}" ${v===125?'selected':''}>${v}</option>`).join('')}
                </select>
            </div>
            <div class="form-row">
                <label>Power (dBm)</label>
                <input type="number" id="lora-power" value="10" min="-9" max="22" style="width:60px">
                <label>CR</label>
                <select id="lora-cr">
                    ${[5,6,7,8].map(v => `<option value="${v}" ${v===5?'selected':''}>4/${v}</option>`).join('')}
                </select>
            </div>
            <div class="form-row">
                <label>Sync Word</label>
                <input type="text" id="lora-sync" value="0x12" style="width:60px" placeholder="0x12">
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
    panel.querySelector('#lora-mode-text').onclick = () => { hexMode = false; setToggle(false); };
    panel.querySelector('#lora-mode-hex').onclick = () => { hexMode = true; setToggle(true); };

    function setToggle(hex) {
        panel.querySelector('#lora-mode-text').classList.toggle('active', !hex);
        panel.querySelector('#lora-mode-hex').classList.toggle('active', hex);
    }

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
    panel.querySelector('#lora-clear-btn').onclick = () => {
        rxLog.length = 0;
        renderLog();
    };
}

export function activateLora(api) {
    currentApi = api;
    refreshConfig();
    api.subscribe('/ws/lora', onLoraRx);
}

export function deactivateLora(api) {
    api.unsubscribe('/ws/lora');
    currentApi = null;
}

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
    rxLog.unshift({
        time: new Date().toLocaleTimeString(),
        data: d.data,
        hex: d.hex,
        rssi: d.rssi,
        snr: d.snr,
        len: d.length,
    });
    if (rxLog.length > 100) rxLog.pop();
    renderLog();
}

function renderLog() {
    const el = panel.querySelector('#lora-rx-log');
    if (rxLog.length === 0) {
        el.innerHTML = '<div style="color:var(--text-dim);padding:8px">No packets yet</div>';
        return;
    }
    el.innerHTML = rxLog.map(p => {
        const rssiColor = p.rssi > -70 ? 'var(--accent)' : p.rssi > -100 ? 'var(--yellow)' : 'var(--red)';
        const rssiW = Math.max(5, Math.min(100, (p.rssi + 140) / 1.4));
        const snrColor = p.snr > 5 ? 'var(--accent)' : p.snr > 0 ? 'var(--yellow)' : 'var(--red)';
        const snrW = Math.max(5, Math.min(100, (p.snr + 20) / 0.4));
        return `<div class="log-entry">
            <span class="log-time">${p.time}</span>
            <span>${p.len}B</span><br>
            <span style="word-break:break-all">${escHtml(p.data)}</span>
            <span style="color:var(--text-dim);font-size:11px"> [${p.hex}]</span><br>
            <span class="rssi-bar" style="background:${rssiColor};width:${rssiW}px" title="RSSI: ${p.rssi}"></span>
            <span style="font-size:11px">${p.rssi} dBm</span>
            <span class="rssi-bar" style="background:${snrColor};width:${snrW}px;margin-left:8px" title="SNR: ${p.snr}"></span>
            <span style="font-size:11px">${p.snr} dB</span>
        </div>`;
    }).join('');
}

function escHtml(s) {
    return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}
