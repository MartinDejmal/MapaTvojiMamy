# MapaTvojiMamy

Firmware pro ESP32 desku s PCB mapou České republiky. Deska obsahuje 77 adresovatelných WS2812B LED diod – jednu pro každý okres ČR. Firmware pravidelně stahuje data z nakonfigurovaného JSON API (výchozí: [tmep.cz](http://tmep.cz)), parsuje je a podle hodnot vykresluje barevnou teplotní mapu na LEDky.

---

## Obsah

1. [Cíl projektu](#cíl-projektu)
2. [Architektura](#architektura)
3. [Hardware](#hardware)
4. [Instalace a zprovoznění](#instalace-a-zprovoznění)
5. [Konfigurace](#konfigurace)
6. [Typy parserů](#typy-parserů)
7. [Webová konfigurace a diagnostika](#webová-konfigurace-a-diagnostika)
8. [Struktura projektu](#struktura-projektu)

---

## Cíl projektu

Projekt vznikl jako dárek – PCB s fyzickou mapou ČR osazenou LED diodami zobrazuje v reálném čase teploty (nebo jiné hodnoty) pro všechny okresy. Firmware je navržen tak, aby byl snadno konfigurovatelný přes webové rozhraní bez nutnosti přeflashování.

---

## Architektura

Firmware je rozdělen do samostatných modulů:

| Modul | Soubory | Popis |
|---|---|---|
| **App** | `App.h / App.cpp` | Hlavní orchestrátor. Inicializuje všechny služby v `begin()` a spouští hlavní smyčku v `loop()`. |
| **AppConfig** | `AppConfig.h / AppConfig.cpp` | Datové struktury konfigurace (`WifiConfig`, `MapProfileConfig`, `RenderConfig`) a výchozí hodnoty. |
| **ConfigStore** | `ConfigStore.h / ConfigStore.cpp` | Načítání, ukládání a validace konfigurace ve formátu JSON v souborovém systému LittleFS (`/config.json`). |
| **WifiService** | `WifiService.h / WifiService.cpp` | Připojení k Wi-Fi. Při neúspěchu nebo chybějícím SSID spustí záložní AP režim s captive portálem pro prvotní nastavení. |
| **HttpService** | `HttpService.h / HttpService.cpp` | Stahování dat z URL pomocí HTTP GET. |
| **DataParser** | `DataParser.h / DataParser.cpp` | Parsování JSON odpovědi do pole stavů LED (`LedState`). Podporuje více formátů (viz [Typy parserů](#typy-parserů)). |
| **LocationRegistry** | `LocationRegistry.h / LocationRegistry.cpp` | Mapování názvů okresů (normalizovaných, bez diakritiky) na indexy LED diod (0–76). |
| **LedRenderer** | `LedRenderer.h / LedRenderer.cpp` | Vykreslování stavů LED pomocí knihovny FastLED. Převádí číselné hodnoty na barvu pomocí barevného kola (wheel). |
| **WebConfigServer** | `WebConfigServer.h / WebConfigServer.cpp` | HTTP server na portu 80. Obsluhuje webové rozhraní pro konfiguraci a diagnostiku. |

### Tok dat

```
[JSON API] --HTTP GET--> [HttpService]
                               |
                               v
                        [DataParser]  <-- AppConfig (parserType, pole, rozsahy)
                               |
                               v
                    LedState[77]  <-- [LocationRegistry] (okresy -> LED indexy)
                               |
                               v
                        [LedRenderer]  --> WS2812B LEDky
```

Při každém cyklu (`loop()`) firmware:
1. Zkontroluje, zda uplynul interval `refreshIntervalMs`.
2. Stáhne JSON payload z nakonfigurované URL.
3. Parsuje payload na pole stavů LED.
4. Předá stavy `LedRenderer`, který aktualizuje fyzické LED diody.

---

## Hardware

- **MCU:** ESP32 (deska `esp32dev`)
- **LED:** 77× WS2812B na pinu **GPIO 27**
- **Napájení:** závisí na konkrétní PCB desce

---

## Instalace a zprovoznění

### Požadavky

- [PlatformIO](https://platformio.org/) (CLI nebo rozšíření do VS Code)
- ESP32 připojený přes USB

### Knihovny (automaticky staženy PlatformIO)

- [ArduinoJson](https://arduinojson.org/) (`bblanchon/ArduinoJson`)
- [FastLED](https://fastled.io/) (`fastled/FastLED`)

### Postup

1. **Klonuj repozitář:**
   ```bash
   git clone https://github.com/MartinDejmal/MapaTvojiMamy.git
   cd MapaTvojiMamy
   ```

2. **Vytvoř konfigurační soubor:**
   Zkopíruj vzorový soubor a uprav přihlašovací údaje k Wi-Fi a URL datového zdroje:
   ```bash
   cp config.example.json data/config.json
   # Uprav data/config.json dle potřeby
   ```

3. **Nahraj souborový systém (LittleFS):**
   Obsahuje webový frontend (`index.html`, `app.js`, `app.css`) a konfiguraci:
   ```bash
   pio run -t uploadfs
   ```
   V PlatformIO IDE použij task **Upload Filesystem Image**.

4. **Nahraj firmware:**
   ```bash
   pio run -t upload
   ```

5. **Sleduj výstup sériového monitoru** (rychlost 115 200 Bd):
   ```bash
   pio device monitor
   ```

> **Tip:** Při prvním spuštění bez konfigurace Wi-Fi se zařízení přepne do **AP režimu** – viz sekce [Webová konfigurace](#webová-konfigurace-a-diagnostika).

---

## Konfigurace

Konfigurace se ukládá jako `/config.json` v LittleFS. Po prvním nahrání lze konfiguraci měnit přes webové rozhraní bez nutnosti znovu flashovat firmware.

### Struktura konfiguračního souboru

```json
{
  "schemaVersion": 1,
  "wifi": {
    "ssid": "YOUR_WIFI_SSID",
    "password": "YOUR_WIFI_PASSWORD",
    "hostname": "cr-mapa"
  },
  "mapProfile": {
    "url": "http://tmep.cz/vystup-json.php?okresy_cr=1",
    "parserType": "INDEXED_H1",
    "locationField": "name",
    "valueField": "h1",
    "colorField": "color",
    "minValue": -15.0,
    "maxValue": 40.0,
    "refreshIntervalMs": 1000
  },
  "render": {
    "brightness": 10,
    "wheelMin": 170,
    "wheelMax": 0
  }
}
```

### Popis polí

#### `wifi`
| Pole | Popis |
|---|---|
| `ssid` | Název Wi-Fi sítě |
| `password` | Heslo Wi-Fi sítě |
| `hostname` | Hostname zařízení v síti (výchozí: `cr-mapa`) |

#### `mapProfile`
| Pole | Popis |
|---|---|
| `url` | URL JSON API, ze kterého se stahují data |
| `parserType` | Formát JSON odpovědi (viz [Typy parserů](#typy-parserů)) |
| `locationField` | Název pole s názvem okresu (pro `NAMED_*` parsery); pro `OBJECT_LIST_ID_RGB` se nepoužívá |
| `valueField` | Název pole s číselnou hodnotou (pro `INDEXED_VALUE_FIELD`, `NAMED_VALUE_FIELD`); pro `OBJECT_LIST_ID_RGB` se nepoužívá |
| `colorField` | Název pole s barvou ve formátu `#RRGGBB` (pro `NAMED_COLOR_FIELD`); pro `OBJECT_LIST_ID_RGB` se nepoužívá |
| `minValue` | Minimální hodnota pro mapování na barvu (výchozí: `-15.0`) |
| `maxValue` | Maximální hodnota pro mapování na barvu (výchozí: `40.0`) |
| `refreshIntervalMs` | Interval obnovení dat v milisekundách (výchozí: `1000`) |

#### `render`
| Pole | Popis |
|---|---|
| `brightness` | Jas LED diod (0–255, výchozí: `10`) |
| `wheelMin` | Počáteční pozice na barevném kole pro `minValue` (výchozí: `170` = modrá) |
| `wheelMax` | Koncová pozice na barevném kole pro `maxValue` (výchozí: `0` = červená) |

---

## Typy parserů

Parser se konfiguruje polem `parserType` a určuje, jak firmware interpretuje JSON odpověď z API.

### `INDEXED_H1`

Indexované pole, hodnota se čte přímo z pole `h1`. Výchozí formát pro tmep.cz.

```json
[
  { "h1": 22.5 },
  { "h1": 18.0 },
  ...
]
```
Index prvku v poli odpovídá indexu LED diody (0 = Benešov, 1 = Beroun, …).

### `INDEXED_VALUE_FIELD`

Indexované pole, hodnota se čte z pole nastaveného v `valueField`.

```json
[
  { "temperature": 22.5 },
  { "temperature": 18.0 },
  ...
]
```

### `NAMED_VALUE_FIELD`

Pojmenované pole, hodnota se čte z `valueField`, okres se určuje podle `locationField`. Pořadí prvků v poli nezáleží.

```json
[
  { "name": "Benešov", "temperature": 22.5 },
  { "name": "Beroun", "temperature": 18.0 },
  ...
]
```

### `NAMED_COLOR_FIELD`

Pojmenované pole, barva LED se čte přímo z `colorField` ve formátu hex (`#RRGGBB`). Hodnoty `minValue`/`maxValue` a `wheelMin`/`wheelMax` se v tomto režimu nepoužijí.

```json
[
  { "name": "Benešov", "color": "#ff4400" },
  { "name": "Beroun", "color": "#00aaff" },
  ...
]
```

### `OBJECT_LIST_ID_RGB`

Root JSON je objekt. Parser očekává pole `seznam`, kde každá položka obsahuje `id` (index LED) a přímé RGB složky `r`, `g`, `b`. Pole `utc_datum` (a jakákoliv další metadata) jsou tolerována a ignorována.

```json
{
  "utc_datum": {
    "rok": 2026,
    "mesic": 3,
    "den": 17,
    "hodina": 18,
    "minuta": 20
  },
  "seznam": [
    { "id": 35, "r": 48, "g": 0, "b": 168 },
    { "id": 42, "r": 0, "g": 0, "b": 252 }
  ]
}
```

```json
{
  "mapProfile": {
    "url": "http://example.local/mapa.json",
    "parserType": "OBJECT_LIST_ID_RGB",
    "locationField": "",
    "valueField": "",
    "colorField": "",
    "minValue": -15,
    "maxValue": 40,
    "refreshIntervalMs": 1000
  }
}
```

---

## Webová konfigurace a diagnostika

Webové rozhraní je dostupné v prohlížeči po připojení ESP32 do lokální sítě na adrese:

```
http://cr-mapa/
```
nebo podle nakonfigurovaného hostname / IP adresy zobrazené na sériovém monitoru.

### AP režim (prvotní nastavení)

Pokud zařízení nemůže připojit k Wi-Fi (chybějící nebo špatné přihlašovací údaje), automaticky spustí Wi-Fi přístupový bod:

- **SSID:** `cr-mapa-setup` (nebo `<hostname>-setup`)
- **IP adresa:** zobrazena na sériovém monitoru

Připoj se k tomuto AP, otevři prohlížeč – díky captive portálu se automaticky zobrazí konfigurační stránka.

### REST API

Webový server vystavuje toto API:

| Endpoint | Metoda | Popis |
|---|---|---|
| `/api/status` | GET | Vrátí stav zařízení (Wi-Fi, poslední fetch, heap, počty parsovaných položek…) |
| `/api/config` | GET | Vrátí aktuální konfiguraci jako JSON |
| `/api/config` | POST | Uloží novou konfiguraci a aplikuje ji |
| `/api/test-fetch` | POST | Provede jednorázový fetch dat a vrátí výsledek parsování |
| `/api/restart` | POST | Restartuje zařízení |

---

## Struktura projektu

```
MapaTvojiMamy/
├── src/                    # Zdrojové soubory firmware (C++)
│   ├── main.cpp            # Vstupní bod (setup / loop)
│   ├── App.cpp             # Hlavní orchestrátor
│   ├── AppConfig.cpp       # Výchozí konfigurace
│   ├── ConfigStore.cpp     # Načítání / ukládání konfigurace (LittleFS)
│   ├── WifiService.cpp     # Wi-Fi připojení a AP režim
│   ├── HttpService.cpp     # HTTP GET požadavky
│   ├── DataParser.cpp      # Parsování JSON do stavů LED
│   ├── LocationRegistry.cpp# Mapování názvů okresů na LED indexy
│   ├── LedRenderer.cpp     # Vykreslování LED (FastLED)
│   └── WebConfigServer.cpp # HTTP server pro webové rozhraní
├── include/                # Hlavičkové soubory (.h)
├── data/                   # Webový frontend (LittleFS)
│   ├── index.html          # Admin rozhraní
│   ├── app.js              # Frontend logika
│   └── app.css             # Styly
├── archive/
│   └── MapaTvojiMamy.ino   # Původní jednoduchý sketch (archiv)
├── config.example.json     # Vzorová konfigurace
└── platformio.ini          # PlatformIO konfigurace projektu
```
