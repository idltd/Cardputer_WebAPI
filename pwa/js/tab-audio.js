let panel, currentApi;

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
    const freqVal = panel.querySelector('#audio-freq-val');
    const durSlider = panel.querySelector('#audio-dur');
    const durVal = panel.querySelector('#audio-dur-val');
    const volSlider = panel.querySelector('#audio-vol');
    const volVal = panel.querySelector('#audio-vol-val');

    freqSlider.oninput = () => { freqVal.textContent = freqSlider.value + ' Hz'; };
    durSlider.oninput = () => { durVal.textContent = durSlider.value + 'ms'; };
    volSlider.oninput = () => { volVal.textContent = volSlider.value; };

    // Presets
    panel.querySelector('.preset-row').addEventListener('click', (e) => {
        const btn = e.target.closest('[data-freq]');
        if (!btn) return;
        freqSlider.value = btn.dataset.freq;
        freqVal.textContent = btn.dataset.freq + ' Hz';
    });

    panel.querySelector('#audio-play-btn').onclick = () => {
        if (!currentApi) return;
        const status = panel.querySelector('#audio-status');
        currentApi.post('/api/audio/tone', {
            freq: parseInt(freqSlider.value),
            duration: parseInt(durSlider.value),
        }).then(() => {
            status.textContent = 'Playing';
            setTimeout(() => status.textContent = '', parseInt(durSlider.value));
        }).catch(e => { status.textContent = e.message; });
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
