const statusBox = document.getElementById('statusBox');
const configForm = document.getElementById('configForm');
const formMessage = document.getElementById('formMessage');
const testFetchBox = document.getElementById('testFetchBox');
const apBanner = document.getElementById('apBanner');
const apSsidLabel = document.getElementById('apSsidLabel');
const apIpLabel = document.getElementById('apIpLabel');
const apLink = document.getElementById('apLink');
const tabButtons = Array.from(document.querySelectorAll('.tab-button'));
const tabPanels = Array.from(document.querySelectorAll('.tab-panel'));

const mapContainer = document.getElementById('mapContainer');
const mapImage = document.getElementById('mapImage');
const mapOverlay = document.getElementById('mapOverlay');
const mapTooltip = document.getElementById('mapTooltip');
const mapError = document.getElementById('mapError');

let activeTab = 'status';
let mapPollTimer = null;
let mapData = null;

async function fetchJson(url, options = {}) {
  const response = await fetch(url, {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  });
  const data = await response.json();
  if (!response.ok) {
    throw new Error(data.message || data.error || `HTTP ${response.status}`);
  }
  return data;
}

function fillConfigForm(config) {
  configForm.wifiSsid.value = config.wifi?.ssid || '';
  configForm.wifiPassword.value = config.wifi?.password || '';
  configForm.hostname.value = config.wifi?.hostname || '';
  configForm.dataUrl.value = config.mapProfile?.url || '';
  configForm.parserType.value = config.mapProfile?.parserType || 'INDEXED_H1';
  configForm.locationField.value = config.mapProfile?.locationField || '';
  configForm.valueField.value = config.mapProfile?.valueField || '';
  configForm.colorField.value = config.mapProfile?.colorField || '';
  configForm.minValue.value = config.mapProfile?.minValue ?? '';
  configForm.maxValue.value = config.mapProfile?.maxValue ?? '';
  configForm.refreshIntervalMs.value = config.mapProfile?.refreshIntervalMs ?? '';
  configForm.brightness.value = config.render?.brightness ?? '';
  configForm.wheelMin.value = config.render?.wheelMin ?? '';
  configForm.wheelMax.value = config.render?.wheelMax ?? '';
  configForm.ledOrder.value = config.render?.ledOrder || 'TVOJEMAMA';
}

function collectConfig() {
  return {
    schemaVersion: 1,
    wifi: {
      ssid: configForm.wifiSsid.value,
      password: configForm.wifiPassword.value,
      hostname: configForm.hostname.value,
    },
    mapProfile: {
      url: configForm.dataUrl.value,
      parserType: configForm.parserType.value,
      locationField: configForm.locationField.value,
      valueField: configForm.valueField.value,
      colorField: configForm.colorField.value,
      minValue: Number(configForm.minValue.value),
      maxValue: Number(configForm.maxValue.value),
      refreshIntervalMs: Number(configForm.refreshIntervalMs.value),
    },
    render: {
      brightness: Number(configForm.brightness.value),
      wheelMin: Number(configForm.wheelMin.value),
      wheelMax: Number(configForm.wheelMax.value),
      ledOrder: configForm.ledOrder.value,
    },
  };
}

function updateApBanner(status) {
  if (status.apMode) {
    apSsidLabel.textContent = status.apSsid || '';
    apIpLabel.textContent = status.apIp || '192.168.4.1';
    apLink.href = `http://${status.apIp || '192.168.4.1'}/`;
    apBanner.hidden = false;
  } else {
    apBanner.hidden = true;
  }
}

function toHexColor(r, g, b) {
  const toHex = (value) => value.toString(16).padStart(2, '0').toUpperCase();
  return `#${toHex(r)}${toHex(g)}${toHex(b)}`;
}

function showTooltip(event, point) {
  const numericText = point.hasNumericValue ? point.numericValue : 'n/a';
  mapTooltip.innerHTML = [
    `<strong>${point.name}</strong>`,
    `index: ${point.index}`,
    `stav: ${point.active ? 'active' : 'inactive'}`,
    `numericValue: ${numericText}`,
    `rgb(${point.r}, ${point.g}, ${point.b})`,
    toHexColor(point.r, point.g, point.b),
  ].join('<br />');

  const rect = mapContainer.getBoundingClientRect();
  const left = event.clientX - rect.left + 12;
  const top = event.clientY - rect.top + 12;
  mapTooltip.style.left = `${left}px`;
  mapTooltip.style.top = `${top}px`;
  mapTooltip.hidden = false;
}

function hideTooltip() {
  mapTooltip.hidden = true;
}

function renderMapPoints() {
  if (!mapData || !mapData.image || !Array.isArray(mapData.points)) {
    return;
  }

  const imageWidth = mapImage.clientWidth;
  const imageHeight = mapImage.clientHeight;
  if (!imageWidth || !imageHeight) {
    return;
  }

  const scaleX = imageWidth / mapData.image.width;
  const scaleY = imageHeight / mapData.image.height;
  mapOverlay.replaceChildren();

  mapData.points.forEach((point) => {
    const dot = document.createElement('div');
    dot.className = `map-dot${point.active ? '' : ' inactive'}`;
    const color = point.active ? `rgb(${point.r}, ${point.g}, ${point.b})` : 'rgba(120, 120, 120, 0.55)';
    dot.style.backgroundColor = color;
    dot.style.left = `${point.x * scaleX}px`;
    dot.style.top = `${point.y * scaleY}px`;

    dot.addEventListener('mouseenter', (event) => showTooltip(event, point));
    dot.addEventListener('mousemove', (event) => showTooltip(event, point));
    dot.addEventListener('mouseleave', hideTooltip);
    dot.addEventListener('click', (event) => showTooltip(event, point));

    mapOverlay.appendChild(dot);
  });
}

async function refreshMapState() {
  try {
    const payload = await fetchJson('/api/map-state');
    mapData = payload;
    mapError.textContent = '';

    if (mapImage.dataset.sourceUrl !== payload.image.url) {
      mapImage.dataset.sourceUrl = payload.image.url;
      mapImage.src = payload.image.url;
      mapImage.onload = renderMapPoints;
    }

    renderMapPoints();
  } catch (error) {
    mapError.textContent = `Chyba mapy: ${error.message}`;
  }
}

function startMapPolling() {
  if (mapPollTimer) {
    return;
  }

  refreshMapState();
  mapPollTimer = setInterval(refreshMapState, 2000);
}

function stopMapPolling() {
  if (!mapPollTimer) {
    return;
  }

  clearInterval(mapPollTimer);
  mapPollTimer = null;
  hideTooltip();
}

function activateTab(tabName) {
  activeTab = tabName;

  tabButtons.forEach((button) => {
    button.classList.toggle('active', button.dataset.tab === tabName);
  });

  tabPanels.forEach((panel) => {
    panel.classList.toggle('active', panel.dataset.panel === tabName);
  });

  if (tabName === 'mapa') {
    startMapPolling();
  } else {
    stopMapPolling();
  }

  if (tabName === 'firmware') {
    loadFwInfo();
  }
}

async function refreshStatus() {
  try {
    const status = await fetchJson('/api/status');
    statusBox.textContent = JSON.stringify(status, null, 2);
    updateApBanner(status);
  } catch (e) {
    statusBox.textContent = `Chyba statusu: ${e.message}`;
  }
}

async function loadConfig() {
  try {
    const config = await fetchJson('/api/config');
    fillConfigForm(config);
  } catch (e) {
    formMessage.textContent = `Chyba načtení configu: ${e.message}`;
  }
}

configForm.addEventListener('submit', async (event) => {
  event.preventDefault();
  formMessage.textContent = 'Ukládám...';
  try {
    const payload = collectConfig();
    const result = await fetchJson('/api/config', {
      method: 'POST',
      body: JSON.stringify(payload),
    });
    formMessage.textContent = result.message || 'Uloženo';
  } catch (e) {
    formMessage.textContent = `Chyba uložení: ${e.message}`;
  }
});

document.getElementById('testFetchBtn').addEventListener('click', async () => {
  testFetchBox.textContent = 'Probíhá test fetch...';
  try {
    const result = await fetchJson('/api/test-fetch', { method: 'POST', body: '{}' });
    testFetchBox.textContent = JSON.stringify(result, null, 2);
  } catch (e) {
    testFetchBox.textContent = `Chyba test fetch: ${e.message}`;
  }
});

document.getElementById('restartBtn').addEventListener('click', async () => {
  if (!confirm('Opravdu restartovat zařízení?')) {
    return;
  }
  try {
    await fetchJson('/api/restart', { method: 'POST', body: '{}' });
    formMessage.textContent = 'Zařízení se restartuje...';
  } catch (e) {
    formMessage.textContent = `Chyba restartu: ${e.message}`;
  }
});

tabButtons.forEach((button) => {
  button.addEventListener('click', () => activateTab(button.dataset.tab));
});

// --- Firmware tab ---

const fwForm = document.getElementById('fwForm');
const fwFile = document.getElementById('fwFile');
const fwUploadBtn = document.getElementById('fwUploadBtn');
const fwProgressWrap = document.getElementById('fwProgressWrap');
const fwProgress = document.getElementById('fwProgress');
const fwProgressPct = document.getElementById('fwProgressPct');
const fwStatus = document.getElementById('fwStatus');
const fwInfo = document.getElementById('fwInfo');

function setFwStatus(msg, isError) {
  fwStatus.textContent = msg;
  fwStatus.className = 'fw-status' + (isError ? ' fw-status-error' : ' fw-status-ok');
}

async function loadFwInfo() {
  try {
    const status = await fetchJson('/api/status');
    fwInfo.innerHTML =
      `Verze firmware: <strong>${status.firmwareVersion || '—'}</strong> &nbsp;|&nbsp; ` +
      `Build: <strong>${status.buildDate || '—'} ${status.buildTime || ''}</strong>`;
  } catch (e) {
    fwInfo.textContent = 'Nepodařilo se načíst info o firmware.';
  }
}

fwForm.addEventListener('submit', (event) => {
  event.preventDefault();

  const file = fwFile.files[0];
  if (!file) {
    setFwStatus('Nevybral jsi žádný soubor.', true);
    return;
  }
  if (!file.name.endsWith('.bin')) {
    setFwStatus('Soubor musí mít příponu .bin.', true);
    return;
  }
  if (file.size === 0) {
    setFwStatus('Soubor je prázdný.', true);
    return;
  }

  const formData = new FormData();
  formData.append('firmware', file, file.name);

  const xhr = new XMLHttpRequest();
  xhr.open('POST', '/api/firmware/upload', true);

  fwUploadBtn.disabled = true;
  fwFile.disabled = true;
  fwProgressWrap.hidden = false;
  fwProgress.value = 0;
  fwProgressPct.textContent = '0 %';
  setFwStatus('Připravuji upload…', false);

  xhr.upload.addEventListener('progress', (e) => {
    if (e.lengthComputable) {
      const pct = Math.round((e.loaded / e.total) * 100);
      fwProgress.value = pct;
      fwProgressPct.textContent = pct + ' %';
      if (pct < 100) {
        setFwStatus('Nahrávám firmware… ' + pct + ' %', false);
      } else {
        setFwStatus('Flashuji firmware, čekej…', false);
      }
    }
  });

  xhr.addEventListener('load', () => {
    fwUploadBtn.disabled = false;
    fwFile.disabled = false;
    let data;
    try {
      data = JSON.parse(xhr.responseText);
    } catch (_) {
      setFwStatus('Neočekávaná odpověď serveru.', true);
      return;
    }
    if (data.ok) {
      fwProgress.value = 100;
      fwProgressPct.textContent = '100 %';
      setFwStatus(
        'Firmware nahrán! Zařízení se restartuje – web UI bude chvíli nedostupné.',
        false
      );
    } else {
      setFwStatus('Chyba: ' + (data.error || 'Neznámá chyba'), true);
    }
  });

  xhr.addEventListener('error', () => {
    fwUploadBtn.disabled = false;
    fwFile.disabled = false;
    setFwStatus('Chyba sítě během uploadu.', true);
  });

  xhr.addEventListener('abort', () => {
    fwUploadBtn.disabled = false;
    fwFile.disabled = false;
    setFwStatus('Upload přerušen.', true);
  });

  xhr.send(formData);
});

window.addEventListener('resize', renderMapPoints);

loadConfig();
refreshStatus();
setInterval(refreshStatus, 5000);
activateTab(activeTab);
