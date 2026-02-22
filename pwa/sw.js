// Bump version to invalidate cache after updates
const CACHE_NAME = 'cardputer-control-v2';

const ASSETS = [
    './',
    './index.html',
    './css/style.css',
    './js/app.js',
    './js/api.js',
    './js/tab-system.js',
    './js/tab-gps.js',
    './js/tab-lora.js',
    './js/tab-imu.js',
    './js/tab-keyboard.js',
    './js/tab-ir.js',
    './js/tab-display.js',
    './js/tab-gpio.js',
    './js/tab-audio.js',
    './manifest.json',
    './assets/icons/icon-192.png',
    './assets/icons/icon-512.png'
];

self.addEventListener('install', (event) => {
    event.waitUntil(
        caches.open(CACHE_NAME)
            .then((cache) => cache.addAll(ASSETS))
            .then(() => self.skipWaiting())
    );
});

self.addEventListener('activate', (event) => {
    event.waitUntil(
        caches.keys().then((keys) =>
            Promise.all(keys.filter((k) => k !== CACHE_NAME).map((k) => caches.delete(k)))
        ).then(() => self.clients.claim())
    );
});

self.addEventListener('fetch', (event) => {
    const url = new URL(event.request.url);
    // Don't cache API/WS requests to the Cardputer
    if (url.pathname.startsWith('/api/') || url.pathname.startsWith('/ws/')) return;

    event.respondWith(
        fetch(event.request).then((response) => {
            if (response.ok) {
                const clone = response.clone();
                caches.open(CACHE_NAME).then((cache) => cache.put(event.request, clone));
            }
            return response;
        }).catch(() => caches.match(event.request))
    );
});
