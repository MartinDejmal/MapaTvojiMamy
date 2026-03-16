# MapaTvojiMamy
Firmware pro ESP desku s PCB mapou tvojí mámy.

## Web konfigurace a diagnostika
Frontend soubory jsou v adresáři `data/`:
- `data/index.html`
- `data/app.js`
- `data/app.css`

Tyto soubory je potřeba nahrát do LittleFS.

### Upload LittleFS dat (PlatformIO)
1. Připoj ESP32.
2. V kořeni projektu spusť:
   - `pio run -t uploadfs`
3. Poté nahraj firmware:
   - `pio run -t upload`

V PlatformIO IDE lze použít task **Upload Filesystem Image**.
