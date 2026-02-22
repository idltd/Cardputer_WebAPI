// tab-record.js — Microphone recording with waveform visualisation

let panel;
let mediaRecorder = null;
let audioCtx = null;
let analyser = null;
let animFrame = null;
let chunks = [];
let recordings = [];  // { blob, url, name, duration }
let startTime = null;

export function initRecord(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>Microphone Recording</h3>
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
    if (mediaRecorder && mediaRecorder.state !== 'inactive') {
        mediaRecorder.stop();
    }
    stopViz();
}

// ── Recording ──────────────────────────────────────────────────────────────

async function startRecording() {
    const status = panel.querySelector('#rec-status');
    let stream;
    try {
        stream = await navigator.mediaDevices.getUserMedia({ audio: true, video: false });
    } catch (e) {
        status.textContent = 'Microphone access denied';
        return;
    }

    chunks = [];
    startTime = Date.now();

    // Waveform viz
    audioCtx = new AudioContext();
    analyser = audioCtx.createAnalyser();
    analyser.fftSize = 256;
    const src = audioCtx.createMediaStreamSource(stream);
    src.connect(analyser);
    drawLiveWaveform();
    startTimer();

    const mimeType = getSupportedMime();
    mediaRecorder = new MediaRecorder(stream, mimeType ? { mimeType } : undefined);
    mediaRecorder.ondataavailable = e => { if (e.data.size > 0) chunks.push(e.data); };
    mediaRecorder.onstop = () => {
        stream.getTracks().forEach(t => t.stop());
        stopViz();
        saveRecording(mimeType);
    };
    mediaRecorder.start(250);

    panel.querySelector('#rec-btn').disabled = true;
    panel.querySelector('#rec-stop-btn').disabled = false;
    status.textContent = 'Recording…';
}

function stopRecording() {
    if (mediaRecorder && mediaRecorder.state !== 'inactive') {
        mediaRecorder.stop();
    }
    panel.querySelector('#rec-btn').disabled = false;
    panel.querySelector('#rec-stop-btn').disabled = true;
    panel.querySelector('#rec-status').textContent = '';
    panel.querySelector('#rec-timer').textContent = '0:00';
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

// ── List ───────────────────────────────────────────────────────────────────

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
            <button class="danger" data-idx="${i}" style="padding:4px 10px;font-size:12px">✕</button>
        `;
        row.querySelector('[data-idx]').onclick = () => {
            URL.revokeObjectURL(recordings[i].url);
            recordings.splice(i, 1);
            renderRecordingList();
        };
        list.appendChild(row);
    });
}

// ── Waveform Visualisation ─────────────────────────────────────────────────

function drawIdleWaveform() {
    const canvas = panel.querySelector('#rec-waveform');
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    canvas.width = canvas.offsetWidth || 300;
    const w = canvas.width, h = canvas.height;
    ctx.fillStyle = getComputedStyle(document.documentElement).getPropertyValue('--surface2').trim() || '#1e1e1e';
    ctx.fillRect(0, 0, w, h);
    ctx.strokeStyle = '#333';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(0, h / 2);
    ctx.lineTo(w, h / 2);
    ctx.stroke();
}

function drawLiveWaveform() {
    const canvas = panel.querySelector('#rec-waveform');
    if (!canvas || !analyser) return;
    canvas.width = canvas.offsetWidth || 300;
    const ctx = canvas.getContext('2d');
    const buf = new Uint8Array(analyser.frequencyBinCount);
    const accent = getComputedStyle(document.documentElement).getPropertyValue('--accent').trim() || '#00ff88';
    const bg = getComputedStyle(document.documentElement).getPropertyValue('--surface2').trim() || '#1e1e1e';

    function frame() {
        animFrame = requestAnimationFrame(frame);
        analyser.getByteTimeDomainData(buf);
        const w = canvas.width, h = canvas.height;
        ctx.fillStyle = bg;
        ctx.fillRect(0, 0, w, h);
        ctx.strokeStyle = accent;
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        const step = w / buf.length;
        for (let i = 0; i < buf.length; i++) {
            const y = (buf[i] / 128.0) * (h / 2);
            i === 0 ? ctx.moveTo(i * step, y) : ctx.lineTo(i * step, y);
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

// ── Timer ──────────────────────────────────────────────────────────────────

let timerInterval = null;

function startTimer() {
    clearInterval(timerInterval);
    timerInterval = setInterval(() => {
        if (!startTime) { clearInterval(timerInterval); return; }
        const el = panel.querySelector('#rec-timer');
        if (!el) { clearInterval(timerInterval); return; }
        const s = Math.floor((Date.now() - startTime) / 1000);
        el.textContent = `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`;
        // Auto-stop at 5 minutes
        if (s >= 300) stopRecording();
    }, 1000);
}

// ── Helpers ────────────────────────────────────────────────────────────────

function getSupportedMime() {
    const candidates = ['audio/webm;codecs=opus', 'audio/webm', 'audio/ogg;codecs=opus', 'audio/ogg'];
    return candidates.find(m => MediaRecorder.isTypeSupported(m)) || '';
}
