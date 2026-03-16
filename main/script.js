const now = new Date();
const timeStr = now.toLocaleString();

const note = document.querySelector('.note');
const p = document.createElement('p');
p.innerHTML = 'Page loaded: <b>' + timeStr + '</b>';
note.appendChild(p);

const history = [];
const maxSamples = 20;
const colors = ['#3498db', '#e74c3c', '#27ae60', '#f39c12'];

function getColor(index) {
  return colors[index % colors.length];
}

function drawTooltip(ctx, x, lines, W) {
  const padX = 8, padY = 6, lineH = 16;
  const boxW = 130, boxH = padY * 2 + lineH * lines.length;
  let tx = x + 12;
  if (tx + boxW > W) tx = x - boxW - 12;

  ctx.fillStyle = 'rgba(44,62,80,0.85)';
  ctx.beginPath();
  ctx.roundRect(tx, 10, boxW, boxH, 4);
  ctx.fill();

  ctx.fillStyle = '#fff';
  ctx.font = '11px Arial';
  ctx.textAlign = 'left';
  lines.forEach((line, i) => {
    ctx.fillText(line, tx + padX, 10 + padY + lineH * (i + 1) - 4);
  });
}

function drawChart(hoverIndex = -1, resize = true) {
  const canvas = document.getElementById('rssiChart');
  if (resize) {
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;
  }
  const ctx = canvas.getContext('2d');
  const W = canvas.width;
  const H = canvas.height;
  const padL = 55, padR = 20, padT = 20, padB = 50;
  const chartW = W - padL - padR;
  const chartH = H - padT - padB;
  const minRssi = -100, maxRssi = 0;

  ctx.clearRect(0, 0, W, H);
  ctx.fillStyle = '#f8f9fa';
  ctx.fillRect(0, 0, W, H);

  // Y grid & labels
  ctx.strokeStyle = '#dee2e6';
  ctx.fillStyle = '#7f8c8d';
  ctx.font = '11px Arial';
  ctx.textAlign = 'right';
  for (const v of [-100, -75, -50, -25, 0]) {
    const gy = padT + chartH - ((v - minRssi) / (maxRssi - minRssi)) * chartH;
    ctx.beginPath();
    ctx.moveTo(padL, gy);
    ctx.lineTo(padL + chartW, gy);
    ctx.stroke();
    ctx.fillText(v + 'dBm', padL - 4, gy + 4);
  }

  if (history.length === 0) {
    ctx.fillStyle = '#adb5bd';
    ctx.textAlign = 'center';
    ctx.font = '13px Arial';
    ctx.fillText('Waiting for data...', padL + chartW / 2, padT + chartH / 2);
    return;
  }

  const macs = [];
  for (const entry of history) {
    for (const sta of entry.stations) {
      if (!macs.includes(sta.mac)) macs.push(sta.mac);
    }
  }

  macs.forEach((mac, m) => {
    ctx.strokeStyle = getColor(m);
    ctx.lineWidth = 2;
    ctx.beginPath();
    let started = false;
    history.forEach((entry, t) => {
      const x = padL + (t / (maxSamples - 1)) * chartW;
      const sta = entry.stations.find(s => s.mac === mac);
      if (!sta) { started = false; return; }
      const y = padT + chartH - ((sta.rssi - minRssi) / (maxRssi - minRssi)) * chartH;
      if (!started) { ctx.moveTo(x, y); started = true; }
      else ctx.lineTo(x, y);
    });
    ctx.stroke();

    const last = history[history.length - 1];
    const lastSta = last.stations.find(s => s.mac === mac);
    if (lastSta) {
      const lx = padL + ((history.length - 1) / (maxSamples - 1)) * chartW;
      const ly = padT + chartH - ((lastSta.rssi - minRssi) / (maxRssi - minRssi)) * chartH;
      ctx.beginPath();
      ctx.arc(lx, ly, 4, 0, 2 * Math.PI);
      ctx.fillStyle = getColor(m);
      ctx.fill();
      ctx.fillStyle = getColor(m);
      ctx.font = 'bold 11px Arial';
      ctx.textAlign = 'left';
      ctx.fillText(lastSta.rssi + 'dBm', lx + 6, ly + 4);
    }
  });

  // X axis time labels
  ctx.fillStyle = '#7f8c8d';
  ctx.font = '10px Arial';
  ctx.textAlign = 'center';
  const step = Math.floor(maxSamples / 4);
  history.forEach((entry, t) => {
    if (t % step === 0) {
      const lx = padL + (t / (maxSamples - 1)) * chartW;
      ctx.fillText(entry.time, lx, padT + chartH + 16);
    }
  });

  // legend
  ctx.font = '11px Arial';
  ctx.textAlign = 'left';
  macs.forEach((mac, m) => {
    ctx.fillStyle = getColor(m);
    ctx.fillRect(padL + m * 160, padT + chartH + 30, 12, 12);
    ctx.fillStyle = '#2c3e50';
    ctx.fillText(mac, padL + m * 160 + 16, padT + chartH + 41);
  });

  // hover tooltip
  if (hoverIndex >= 0 && hoverIndex < history.length) {
    const hx = padL + (hoverIndex / (maxSamples - 1)) * chartW;

    ctx.strokeStyle = 'rgba(44,62,80,0.4)';
    ctx.lineWidth = 1;
    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.moveTo(hx, padT);
    ctx.lineTo(hx, padT + chartH);
    ctx.stroke();
    ctx.setLineDash([]);

    const entry = history[hoverIndex];
    const lines = [entry.time];
    macs.forEach((mac, m) => {
      const sta = entry.stations.find(s => s.mac === mac);
      if (sta) lines.push(mac.slice(-8) + ': ' + sta.rssi + 'dBm');
    });
    drawTooltip(ctx, hx, lines, W);

    macs.forEach((mac, m) => {
      const sta = entry.stations.find(s => s.mac === mac);
      if (!sta) return;
      const hy = padT + chartH - ((sta.rssi - minRssi) / (maxRssi - minRssi)) * chartH;
      ctx.beginPath();
      ctx.arc(hx, hy, 5, 0, 2 * Math.PI);
      ctx.fillStyle = getColor(m);
      ctx.fill();
      ctx.strokeStyle = '#fff';
      ctx.lineWidth = 2;
      ctx.stroke();
    });
  }
}

function fetchStatus() {
  fetch('/api/status')
    .then(r => r.text().then(text => {
      document.getElementById('rssiStatus').textContent = 'Raw: ' + text;
      return JSON.parse(text);
    }))
    .then(data => {
      const t = new Date().toLocaleTimeString();
      history.push({ time: t, stations: data.stations });
      if (history.length > maxSamples) history.shift();
      drawChart(-1, true);
      document.getElementById('rssiStatus').textContent =
        'Connected: ' + data.stations.length + ' station(s)  |  Updated: ' + t;
    })
    .catch(e => {
      document.getElementById('rssiStatus').textContent = 'Error: ' + e.message;
    });
}

// RSSI hover events
const rssiCanvas = document.getElementById('rssiChart');
rssiCanvas.addEventListener('mousemove', e => {
  const rect = rssiCanvas.getBoundingClientRect();
  const scaleX = rssiCanvas.width / rect.width;
  const mouseX = (e.clientX - rect.left) * scaleX;
  const padL = 55, padR = 20;
  const chartW = rssiCanvas.width - padL - padR;
  const idx = Math.round((mouseX - padL) / chartW * (maxSamples - 1));
  const clamped = Math.max(0, Math.min(history.length - 1, idx));
  drawChart(clamped, false);
});
rssiCanvas.addEventListener('mouseleave', () => drawChart(-1, false));

// ADC chart
const adcHistory = [];
const maxAdcSamples = 20;

function drawAdcChart(hoverIndex = -1, resize = true) {
  const canvas = document.getElementById('noiseChart');
  if (resize) {
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;
  }
  const ctx = canvas.getContext('2d');
  const W = canvas.width;
  const H = canvas.height;
  const padL = 50, padR = 20, padT = 16, padB = 16;
  const chartW = W - padL - padR;
  const chartH = H - padT - padB;
  const maxAdc = 4095;

  ctx.clearRect(0, 0, W, H);
  ctx.fillStyle = '#f8f9fa';
  ctx.fillRect(0, 0, W, H);

  ctx.strokeStyle = '#dee2e6';
  ctx.fillStyle = '#7f8c8d';
  ctx.font = '10px Arial';
  ctx.textAlign = 'right';
  for (const v of [0, 1024, 2048, 3072, 4095]) {
    const gy = padT + chartH - (v / maxAdc) * chartH;
    ctx.beginPath();
    ctx.setLineDash(v === 0 || v === 4095 ? [] : [4, 4]);
    ctx.moveTo(padL, gy);
    ctx.lineTo(padL + chartW, gy);
    ctx.stroke();
    ctx.setLineDash([]);
    ctx.fillText(v, padL - 4, gy + 4);
  }

  if (adcHistory.length < 2) {
    ctx.fillStyle = '#adb5bd';
    ctx.textAlign = 'center';
    ctx.font = '13px Arial';
    ctx.fillText('Waiting for data...', padL + chartW / 2, padT + chartH / 2);
    return;
  }

  ctx.strokeStyle = '#e67e22';
  ctx.lineWidth = 2;
  ctx.beginPath();
  adcHistory.forEach((val, i) => {
    const x = padL + (i / (maxAdcSamples - 1)) * chartW;
    const y = padT + chartH - (val / maxAdc) * chartH;
    i === 0 ? ctx.moveTo(x, y) : ctx.lineTo(x, y);
  });
  ctx.stroke();

  const lastVal = adcHistory[adcHistory.length - 1];
  const lx = padL + ((adcHistory.length - 1) / (maxAdcSamples - 1)) * chartW;
  const ly = padT + chartH - (lastVal / maxAdc) * chartH;
  ctx.beginPath();
  ctx.arc(lx, ly, 4, 0, 2 * Math.PI);
  ctx.fillStyle = '#e67e22';
  ctx.fill();
  ctx.fillStyle = '#e67e22';
  ctx.font = 'bold 11px Arial';
  ctx.textAlign = 'left';
  ctx.fillText(lastVal, lx + 6, ly + 4);

  // hover tooltip
  if (hoverIndex >= 0 && hoverIndex < adcHistory.length) {
    const hx = padL + (hoverIndex / (maxAdcSamples - 1)) * chartW;
    const hy = padT + chartH - (adcHistory[hoverIndex] / maxAdc) * chartH;

    ctx.strokeStyle = 'rgba(44,62,80,0.4)';
    ctx.lineWidth = 1;
    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.moveTo(hx, padT);
    ctx.lineTo(hx, padT + chartH);
    ctx.stroke();
    ctx.setLineDash([]);

    drawTooltip(ctx, hx, ['ADC: ' + adcHistory[hoverIndex]], W);

    ctx.beginPath();
    ctx.arc(hx, hy, 5, 0, 2 * Math.PI);
    ctx.fillStyle = '#e67e22';
    ctx.fill();
    ctx.strokeStyle = '#fff';
    ctx.lineWidth = 2;
    ctx.stroke();
  }
}

function fetchAdc() {
  fetch('/api/adc')
    .then(r => r.json())
    .then(data => {
      adcHistory.push(data.adc);
      if (adcHistory.length > maxAdcSamples) adcHistory.shift();
      drawAdcChart(-1, true);
    })
    .catch(() => {});
}

// ADC hover events
const adcCanvas = document.getElementById('noiseChart');
adcCanvas.addEventListener('mousemove', e => {
  const rect = adcCanvas.getBoundingClientRect();
  const scaleX = adcCanvas.width / rect.width;
  const mouseX = (e.clientX - rect.left) * scaleX;
  const padL = 50, padR = 20;
  const chartW = adcCanvas.width - padL - padR;
  const idx = Math.round((mouseX - padL) / chartW * (maxAdcSamples - 1));
  const clamped = Math.max(0, Math.min(adcHistory.length - 1, idx));
  drawAdcChart(clamped, false);
});
adcCanvas.addEventListener('mouseleave', () => drawAdcChart(-1, false));

fetchStatus();
setInterval(fetchStatus, 3000);

fetchAdc();
setInterval(fetchAdc, 200);
