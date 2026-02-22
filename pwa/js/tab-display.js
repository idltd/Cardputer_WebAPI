let panel, currentApi;

export function initDisplay(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>Draw Text</h3>
            <div class="form-row">
                <input type="text" id="disp-text" placeholder="Hello!" style="flex:1">
            </div>
            <div class="form-row">
                <label>X</label>
                <input type="number" id="disp-x" value="0" min="0" style="width:60px">
                <label>Y</label>
                <input type="number" id="disp-y" value="0" min="0" style="width:60px">
                <label>Size</label>
                <input type="number" id="disp-size" value="2" min="1" max="8" style="width:50px">
            </div>
            <div class="form-row">
                <label>Color</label>
                <input type="color" id="disp-text-color" value="#ffffff">
                <button id="disp-text-btn">Send Text</button>
                <span id="disp-text-status" style="font-size:12px;color:var(--text-dim)"></span>
            </div>
        </div>
        <div class="card">
            <h3>Clear Screen</h3>
            <div class="form-row">
                <label>Color</label>
                <input type="color" id="disp-clear-color" value="#000000">
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
                <label>Color</label>
                <input type="color" id="disp-fill-color" value="#00ff88">
                <button id="disp-fill-btn">Fill Rect</button>
            </div>
        </div>
    `;

    panel.querySelector('#disp-text-btn').onclick = () => {
        if (!currentApi) return;
        const body = {
            text: panel.querySelector('#disp-text').value,
            x: parseInt(panel.querySelector('#disp-x').value),
            y: parseInt(panel.querySelector('#disp-y').value),
            size: parseInt(panel.querySelector('#disp-size').value),
            color: panel.querySelector('#disp-text-color').value,
        };
        const status = panel.querySelector('#disp-text-status');
        currentApi.post('/api/display/text', body)
            .then(() => { status.textContent = 'Done'; setTimeout(() => status.textContent = '', 2000); })
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
