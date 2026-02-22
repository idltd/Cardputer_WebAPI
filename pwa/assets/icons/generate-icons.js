// Run: node generate-icons.js
// Generates simple PNG icons for the PWA using Canvas

const { createCanvas } = require('canvas');
const fs = require('fs');

function generateIcon(size, filename) {
    const c = createCanvas(size, size);
    const ctx = c.getContext('2d');

    // Background
    ctx.fillStyle = '#0a0a0a';
    ctx.fillRect(0, 0, size, size);

    // Rounded border
    const m = size * 0.05;
    ctx.strokeStyle = '#00ff88';
    ctx.lineWidth = size * 0.02;
    roundRect(ctx, m, m, size - m * 2, size - m * 2, size * 0.12);
    ctx.stroke();

    // "CP" text
    ctx.fillStyle = '#00ff88';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.font = `bold ${size * 0.38}px sans-serif`;
    ctx.fillText('CP', size / 2, size * 0.42);

    // "ctrl" sub-text
    ctx.font = `${size * 0.14}px monospace`;
    ctx.fillStyle = '#00cc6a';
    ctx.fillText('ctrl', size / 2, size * 0.72);

    fs.writeFileSync(filename, c.toBuffer('image/png'));
    console.log(`Generated ${filename}`);
}

function roundRect(ctx, x, y, w, h, r) {
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.lineTo(x + w - r, y);
    ctx.quadraticCurveTo(x + w, y, x + w, y + r);
    ctx.lineTo(x + w, y + h - r);
    ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
    ctx.lineTo(x + r, y + h);
    ctx.quadraticCurveTo(x, y + h, x, y + h - r);
    ctx.lineTo(x, y + r);
    ctx.quadraticCurveTo(x, y, x + r, y);
    ctx.closePath();
}

generateIcon(192, 'icon-192.png');
generateIcon(512, 'icon-512.png');
