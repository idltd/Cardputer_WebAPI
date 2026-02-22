let panel;
const keyLog = [];
let renderPending = false;

export function initKeyboard(el) {
    panel = el;
    panel.innerHTML = `
        <div class="card">
            <h3>Keyboard Events</h3>
            <button id="keys-clear-btn" class="secondary" style="margin-bottom:8px;font-size:11px">Clear</button>
            <div class="log" id="keys-log">
                <div style="color:var(--text-dim);padding:8px">Press keys on the Cardputer...</div>
            </div>
        </div>
    `;

    panel.querySelector('#keys-clear-btn').onclick = () => {
        keyLog.length = 0;
        renderLog();
    };
}

export function activateKeyboard(api) {
    api.subscribe('/ws/keyboard', onKeyEvent);
}

export function deactivateKeyboard(api) {
    api.unsubscribe('/ws/keyboard');
}

function onKeyEvent(d) {
    if (!d || typeof d !== 'object') return;

    const mods = [];
    if (d.shift) mods.push('<span class="badge shift">SHIFT</span>');
    if (d.ctrl) mods.push('<span class="badge ctrl">CTRL</span>');
    if (d.alt || d.opt) mods.push('<span class="badge alt">ALT</span>');
    if (d.fn) mods.push('<span class="badge fn">FN</span>');

    let charText = '';
    if (d.char) charText = `"${escHtml(d.char)}"`;
    else if (d.enter) charText = '[ENTER]';
    else if (d.del) charText = '[DEL]';
    else if (d.tab) charText = '[TAB]';
    else if (d.space) charText = '[SPACE]';

    keyLog.unshift({
        time: new Date().toLocaleTimeString(),
        char: charText,
        mods: mods.join(''),
    });
    if (keyLog.length > 50) keyLog.length = 50;

    // Throttle DOM updates to one per animation frame
    if (!renderPending) {
        renderPending = true;
        requestAnimationFrame(() => {
            renderPending = false;
            renderLog();
        });
    }
}

function renderLog() {
    const el = panel?.querySelector('#keys-log');
    if (!el) return;
    if (keyLog.length === 0) {
        el.innerHTML = '<div style="color:var(--text-dim);padding:8px">Press keys on the Cardputer...</div>';
        return;
    }
    el.innerHTML = keyLog.map(e =>
        `<div class="log-entry"><span class="log-time">${e.time}</span>${e.mods} <strong>${e.char}</strong></div>`
    ).join('');
}

function escHtml(s) {
    return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
}
