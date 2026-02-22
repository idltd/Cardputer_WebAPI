let panel, currentApi;
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
                <label>Address</label>
                <input type="text" id="ir-address" placeholder="0x00" style="width:80px">
                <label>Command</label>
                <input type="text" id="ir-command" placeholder="0x00" style="width:80px">
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
            <div class="log" id="ir-log">
                <div style="color:var(--text-dim);padding:8px">No IR codes sent yet</div>
            </div>
        </div>
    `;

    panel.querySelector('#ir-send-btn').onclick = () => {
        if (!currentApi) return;
        const protocol = panel.querySelector('#ir-protocol').value;
        const address = panel.querySelector('#ir-address').value.trim();
        const command = panel.querySelector('#ir-command').value.trim();
        const repeats = parseInt(panel.querySelector('#ir-repeats').value) || 0;
        const status = panel.querySelector('#ir-status');

        if (!address || !command) {
            status.textContent = 'Enter address and command';
            return;
        }

        status.textContent = 'Sending...';
        currentApi.post('/api/ir/send', { protocol, address, command, repeats }).then(r => {
            status.textContent = 'Sent!';
            irHistory.unshift({
                time: new Date().toLocaleTimeString(),
                protocol: r.protocol,
                address: '0x' + r.address.toString(16).toUpperCase(),
                command: '0x' + r.command.toString(16).toUpperCase(),
                repeats: r.repeats,
            });
            if (irHistory.length > 50) irHistory.pop();
            renderLog();
            setTimeout(() => status.textContent = '', 2000);
        }).catch(e => { status.textContent = 'Error: ' + e.message; });
    };

    panel.querySelector('#ir-clear-btn').onclick = () => {
        irHistory.length = 0;
        renderLog();
    };
}

export function activateIr(api) {
    currentApi = api;
}

export function deactivateIr() {
    currentApi = null;
}

function renderLog() {
    const el = panel.querySelector('#ir-log');
    if (irHistory.length === 0) {
        el.innerHTML = '<div style="color:var(--text-dim);padding:8px">No IR codes sent yet</div>';
        return;
    }
    el.innerHTML = irHistory.map(e =>
        `<div class="log-entry">
            <span class="log-time">${e.time}</span>
            <strong>${e.protocol.toUpperCase()}</strong>
            addr=${e.address} cmd=${e.command} rpt=${e.repeats}
        </div>`
    ).join('');
}
