let panel, canvas, ctx, currentApi;
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
                        <div class="imu-bar-label">
                            <span>${a.label}</span>
                            <span id="imu-val-${a.id}">0</span>
                        </div>
                        <div class="imu-bar-track">
                            <div class="imu-bar-thumb" id="imu-bar-${a.id}" style="background:${a.color}"></div>
                        </div>
                    </div>
                `).join('')}
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

export function activateImu(api) {
    currentApi = api;
    api.subscribe('/ws/imu', onImuData);
}

export function deactivateImu(api) {
    api.unsubscribe('/ws/imu');
    currentApi = null;
}

function onImuData(d) {
    const vals = {
        ax: d.accel?.x ?? 0, ay: d.accel?.y ?? 0, az: d.accel?.z ?? 0,
        gx: d.gyro?.x ?? 0,  gy: d.gyro?.y ?? 0,  gz: d.gyro?.z ?? 0,
    };

    for (const a of axes) {
        const v = vals[a.id];
        document.getElementById('imu-val-' + a.id).textContent = v.toFixed(2);
        const pct = ((v - a.min) / (a.max - a.min)) * 100;
        const clamped = Math.max(0, Math.min(100, pct));
        const mid = 50;
        const bar = document.getElementById('imu-bar-' + a.id);
        if (clamped >= mid) {
            bar.style.left = mid + '%';
            bar.style.width = (clamped - mid) + '%';
        } else {
            bar.style.left = clamped + '%';
            bar.style.width = (mid - clamped) + '%';
        }
    }

    // Throttle canvas to display refresh rate
    latestTilt.ax = vals.ax;
    latestTilt.ay = vals.ay;
    if (!rafPending) {
        rafPending = true;
        requestAnimationFrame(() => {
            rafPending = false;
            drawTilt(latestTilt.ax, latestTilt.ay);
        });
    }
}

function drawTilt(ax, ay) {
    const w = canvas.clientWidth, h = canvas.clientHeight;
    canvas.width = w * devicePixelRatio;
    canvas.height = h * devicePixelRatio;
    ctx.setTransform(devicePixelRatio, 0, 0, devicePixelRatio, 0, 0);
    ctx.clearRect(0, 0, w, h);

    const cx = w / 2, cy = h / 2;
    const r = Math.min(cx, cy) - 10;

    // Outer ring
    ctx.strokeStyle = '#2a2a2a';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.arc(cx, cy, r, 0, Math.PI * 2);
    ctx.stroke();

    // Crosshairs
    ctx.strokeStyle = '#1e1e1e';
    ctx.beginPath();
    ctx.moveTo(cx - r, cy); ctx.lineTo(cx + r, cy);
    ctx.moveTo(cx, cy - r); ctx.lineTo(cx, cy + r);
    ctx.stroke();

    // Tilt dot — map accel to position (clamp +-2g to circle)
    const dx = Math.max(-1, Math.min(1, ax / 1.5)) * r;
    const dy = Math.max(-1, Math.min(1, ay / 1.5)) * r;

    ctx.fillStyle = '#00ff88';
    ctx.shadowColor = '#00ff88';
    ctx.shadowBlur = 12;
    ctx.beginPath();
    ctx.arc(cx + dx, cy + dy, 8, 0, Math.PI * 2);
    ctx.fill();
    ctx.shadowBlur = 0;
}
