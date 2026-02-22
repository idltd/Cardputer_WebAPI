let panel, canvas, ctx;
const trail = []; // {lat, lon}
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

    panel.querySelector('#gps-clear-trail').onclick = () => {
        trail.length = 0;
        ctx.clearRect(0, 0, canvas.width, canvas.height);
    };
}

export function activateGps(api) {
    api.subscribe('/ws/gps', onGpsData);
}

export function deactivateGps(api) {
    api.unsubscribe('/ws/gps');
}

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

    // HDOP bar (1=ideal, >10=poor)
    if (d.hdop != null) {
        const pct = Math.max(0, Math.min(100, (1 - (d.hdop - 1) / 9) * 100));
        const bar = $('gps-hdop-bar');
        bar.style.width = pct + '%';
        bar.className = 'bar-fill' + (pct < 30 ? ' danger' : pct < 60 ? ' warn' : '');
    }

    // Trail
    if (d.valid && d.lat != null && d.lon != null) {
        trail.push({ lat: d.lat, lon: d.lon });
        if (trail.length > MAX_TRAIL) trail.shift();
        drawTrail();
    }
}

function drawTrail() {
    if (trail.length < 2) return;
    const w = canvas.clientWidth, h = canvas.clientHeight;
    canvas.width = w * devicePixelRatio;
    canvas.height = h * devicePixelRatio;
    ctx.setTransform(devicePixelRatio, 0, 0, devicePixelRatio, 0, 0);

    let minLat = Infinity, maxLat = -Infinity, minLon = Infinity, maxLon = -Infinity;
    for (const p of trail) {
        if (p.lat < minLat) minLat = p.lat;
        if (p.lat > maxLat) maxLat = p.lat;
        if (p.lon < minLon) minLon = p.lon;
        if (p.lon > maxLon) maxLon = p.lon;
    }
    // Add padding
    const pad = 20;
    const dLat = maxLat - minLat || 0.0001;
    const dLon = maxLon - minLon || 0.0001;

    ctx.clearRect(0, 0, w, h);
    ctx.strokeStyle = '#00ff88';
    ctx.lineWidth = 2;
    ctx.lineJoin = 'round';
    ctx.beginPath();
    for (let i = 0; i < trail.length; i++) {
        const x = pad + ((trail[i].lon - minLon) / dLon) * (w - pad * 2);
        const y = pad + (1 - (trail[i].lat - minLat) / dLat) * (h - pad * 2);
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    }
    ctx.stroke();

    // Current position dot
    const last = trail[trail.length - 1];
    const cx = pad + ((last.lon - minLon) / dLon) * (w - pad * 2);
    const cy = pad + (1 - (last.lat - minLat) / dLat) * (h - pad * 2);
    ctx.fillStyle = '#ff4444';
    ctx.beginPath();
    ctx.arc(cx, cy, 4, 0, Math.PI * 2);
    ctx.fill();
}
