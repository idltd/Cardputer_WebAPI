export class CardputerAPI {
    constructor() {
        this._ip = '';
        this._connected = false;
        this._ws = new Map(); // path -> WebSocket
        this._wsCallbacks = new Map(); // path -> callback (for reconnect)
        this._reconnectTimers = new Map();
        this.onStatusChange = null;
    }

    get ip() { return this._ip; }
    get connected() { return this._connected; }

    connect(ip) {
        this._ip = ip;
        this._setStatus(true);
        // Test connection with a system info fetch
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
        // Close all WebSockets
        for (const [path, ws] of this._ws) {
            ws.close();
        }
        this._ws.clear();
        this._wsCallbacks.clear();
        for (const timer of this._reconnectTimers.values()) {
            clearTimeout(timer);
        }
        this._reconnectTimers.clear();
        this._setStatus(false);
    }

    async get(path) {
        try {
            const resp = await fetch(`http://${this._ip}${path}`, {
                signal: AbortSignal.timeout(5000)
            });
            if (!resp.ok) {
                const body = await resp.json().catch(() => null);
                throw new Error(body?.error || `${resp.status} ${resp.statusText}`);
            }
            return resp.json();
        } catch (e) {
            if (e.name === 'TimeoutError' || e.name === 'AbortError') {
                throw new Error('Request timed out — is the Cardputer reachable?');
            }
            if (e.name === 'TypeError') {
                throw new Error('Network error — check WiFi connection');
            }
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
            if (e.name === 'TimeoutError' || e.name === 'AbortError') {
                throw new Error('Request timed out — is the Cardputer reachable?');
            }
            if (e.name === 'TypeError') {
                throw new Error('Network error — check WiFi connection');
            }
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
        if (ws) {
            ws.close();
            this._ws.delete(wsPath);
        }
        const timer = this._reconnectTimers.get(wsPath);
        if (timer) {
            clearTimeout(timer);
            this._reconnectTimers.delete(wsPath);
        }
    }

    _openWs(wsPath, callback) {
        const url = `ws://${this._ip}${wsPath}`;
        const ws = new WebSocket(url);

        ws.onmessage = (e) => {
            try {
                callback(JSON.parse(e.data));
            } catch {
                callback(e.data);
            }
        };

        ws.onclose = () => {
            this._ws.delete(wsPath);
            // Only reconnect if we're still connected AND still subscribed
            if (this._connected && this._wsCallbacks.has(wsPath)) {
                const timer = setTimeout(() => {
                    this._reconnectTimers.delete(wsPath);
                    if (this._connected && this._wsCallbacks.has(wsPath)) {
                        this._openWs(wsPath, callback);
                    }
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
