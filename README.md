# wMBus Gateway

An ESP32 + CC1101 receiver for **wireless M-Bus (868 MHz) water and heat meters** with a built-in
web UI and **MQTT / Home Assistant auto-discovery**. No cloud, no extra hub, no YAML.

> English | [Deutsch](README.de.md)

Developed and tested with **Qundis Q water 5.5** and **Qundis Q heat 5.5** (wM-Bus mode C1,
unencrypted). Other OMS meters that use the same telegram layout may work as well.

> **Works with unencrypted meters only.** The gateway has no support for AES-encrypted wM-Bus telegrams and
> no place to enter a meter key. Many meters installed by a landlord or utility send encrypted telegrams;
> for those the key is needed (ask the provider). Such a meter is heard, but shows up as "format unknown".

## Features

- Receives up to **two meters** (each one water or heat), selectable in the UI
- **Web UI** on its own WiFi access point and in your home network: live values, charts
  (level / consumption, 6 h – 3 days), signal strength, CSV export, reception statistics, telegram log
- **Home Assistant** via MQTT auto-discovery: total, reference values (end of last year / month),
  their dates and signal strength per meter
- History and last values are **stored in flash** and survive restarts
- **Over-the-air updates** from the web UI, watchdog, automatic radio chip health check
- Captive-portal style redirect: unknown addresses opened while connected to the gateway's WiFi lead to the UI
- Decodes the two telegram types the Qundis heat meter alternates between (OMS standard records and the
  Qundis manufacturer block)

## Hardware

| Part | Notes |
|---|---|
| ESP32 board | Any ESP32 (tested: Wemos D1 Mini ESP32) |
| CC1101 module, **868 MHz** | The common green 8-pin module with a spring antenna |

Wiring (module pin order `VCC GND MOSI SCK MISO GDO2 GDO0 CSN`):

| CC1101 | ESP32 |
|---|---|
| VCC | 3.3 V (**not 5 V**) |
| GND | GND |
| MOSI | GPIO 23 |
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| GDO2 | GPIO 32 (not used by the firmware) |
| GDO0 | GPIO 25 (not used by the firmware) |
| CSN | GPIO 5 |

The pins are defined in [`config.h`](config.h). If you are unsure how your module is wired, the sketch in
[`tools/pinprobe`](tools/pinprobe) searches the pin combination for you.

## Build and flash

Requirements: [arduino-cli](https://arduino.github.io/arduino-cli/) (or the Arduino IDE), ESP32 core
**3.x** (tested with 3.3.12) and the library **PubSubClient** (2.8).

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install PubSubClient
arduino-cli compile -b esp32:esp32:esp32 .
arduino-cli upload  -b esp32:esp32:esp32 -p /dev/ttyUSB0 .
```

Use the default partition scheme (it contains two app slots, needed for OTA updates).

## First start

1. Power the gateway and connect to the WiFi network **`wmBus-Gateway`** with the initial password **`wmbus1234`**.
2. Open `http://192.168.4.1` in the browser and log in (user `admin`, password `wmbus1234`).
3. The UI asks you to **choose a new password** first. It replaces the initial password for the web login **and** the
   WiFi access point. Until you have done this, all changes are blocked. Log in again afterwards with the new password.
4. *Settings → Meters*: enter the **8-digit serial number** printed on each meter and choose its type.
5. *Settings → WLAN* (2.4 GHz) and *Settings → Home Assistant* (MQTT broker), then save. The gateway restarts.

Afterwards the UI is also available at `http://wmbus.local` (or the IP address shown in the header).

Lost the password? Connect GPIO 0 (BOOT) to GND for 10 seconds while the gateway is running. This
**factory reset** deletes all settings and history; the initial password applies again.

## Home Assistant

With MQTT configured the entities appear automatically (device per meter). Entity names are German.

| Entity | Description |
|---|---|
| Zählerstand | Total (m³ or kWh), `total_increasing` – usable in the Energy dashboard |
| Wert / Datum Jahresstichtag | Reading and date at the end of last year |
| Wert / Datum Monatsstichtag | Reading and date at the end of last month |
| Signalstärke | RSSI in dBm (diagnostic) |

Home Assistant has no heat category in the Energy dashboard. A heat meter can be added as a *gas source without
costs*, or shown on a dashboard of its own.

Topics: `<prefix>/status` (online/offline), `<prefix>/<serial>/state` (JSON, retained), discovery under
`homeassistant/sensor/...`. The default prefix is `wmbus`.

## Web API

All routes need HTTP Basic auth (`admin` + password). POST routes also need the header
`X-Requested-With: wmbus`.

| Route | Method | Purpose |
|---|---|---|
| `/` | GET | web UI |
| `/api/state` | GET | status and all values |
| `/api/hist?m=<slot>` | GET | history of meter slot 0 or 1 |
| `/api/log` | GET | recent valid telegrams |
| `/api/csv?m=<slot>` | GET | history as CSV |
| `/api/debug` | GET | last undecodable telegram of each meter (hex) |
| `/api/config` | GET / POST | settings (passwords are never returned) |
| `/api/scan` | GET | visible WiFi networks |
| `/api/restart`, `/api/factory` | POST | restart / factory reset |
| `/api/update` | POST | firmware update (multipart field `fw`) |

Update from the command line:

```bash
curl -u admin:<password> -H "X-Requested-With: wmbus" -F "fw=@wmbus-gateway.ino.bin" http://wmbus.local/api/update
```

## Security

The gateway is meant for a **trusted home network**. What is protected, and what is not:

- The public initial password has to be replaced on first login: until then every state-changing route (including
  firmware update) is rejected. Login throttling: after 5 failed attempts every request is rejected for 60 s.
- CSRF protection through the mandatory `X-Requested-With` header on all state-changing routes; strict
  `Content-Security-Policy`, `X-Frame-Options`, `nosniff`.
- All input is validated; strings are JSON-escaped and inserted into the UI as text only.
- Firmware updates require the login.
- **Not protected:** the connection is plain **HTTP** and **MQTT without TLS**, so credentials are visible to anyone who
  can sniff your network. Settings (including WiFi and MQTT passwords) are stored unencrypted in the ESP32's NVS
  flash; anybody with physical access can read them. Firmware images are not signed.
- Do not expose the gateway to the internet. Use a separate WLAN/VLAN and an MQTT user with minimum rights.

See [SECURITY.md](SECURITY.md).

## Updating

- **Web UI:** *System → Firmware update*, drop the `wmbus-gateway.ino.bin` file there. Settings and history are kept.
- **Command line:** the `curl` example above.
- **USB:** `arduino-cli upload` as in *Build and flash*.

Built binaries are not part of the repository; build them yourself or attach them to a release.

## Troubleshooting

- **A meter shows "waiting for telegram"**: meters send every few seconds to several minutes. Check the serial number and
  the signal strength; bring the antenna closer to the meters and keep it away from metal.
- **System page → "Format unknown" counter > 0**: the meter is heard, but its telegram is not understood (often
  because it is **encrypted**). `/api/debug` shows the raw telegram; please open an issue with it.
- **Many CRC errors**: normal in an environment with many wM-Bus meters; they are mostly weak telegrams of neighbours.
- **Radio chip error** (red *Funk* chip): check the wiring of the CC1101.
- **Locked out?** After 5 wrong passwords wait one minute.

## Limitations

- wM-Bus modes T1/C1 at 868.95 MHz only.
- **Only unencrypted meters.** AES-128 encrypted telegrams (OMS security mode 5) cannot be decoded because the gateway has
  no way to store or use a meter key. This is a limitation of this firmware, not of the radio hardware.
- Two meter slots. Only water (volume) and heat (energy in kWh) values are decoded.
- **No encryption / no certificates:** the web UI works over plain **HTTP only** (no HTTPS) and MQTT works **without TLS**
  (no certificates, no `mqtts://`). A broker that enforces TLS cannot be used. See *Security* and *Ideas*.
- The UI and the Home Assistant entity names are in German.

## Project layout

| File | Content |
|---|---|
| `wmbus-gateway.ino` | `setup()` / `loop()` |
| `config.h` | pins, settings in NVS, input validation |
| `meters.h` | meter state, history, flash persistence |
| `radio_cc1101.h` | CC1101 driver |
| `decoder.h` | wM-Bus frame and telegram decoding |
| `net.h` | WiFi, NTP, mDNS, MQTT + discovery |
| `web.h` | web server, API, OTA |
| `webui.h` | the single-page UI |

## Ideas / possible extensions

- **MQTT over TLS** (`mqtts://`, port 8883): feasible with `WiFiClientSecure` and a CA certificate stored on the device;
  the most useful security improvement. Costs about 40 KB of RAM during the handshake.
- **HTTPS for the web UI**: possible with a TLS-capable server library, but a self-signed certificate triggers browser
  warnings, it needs a lot of RAM per connection and the firmware already uses 86 % of the app partition.
  Practical alternative: put the gateway behind a reverse proxy that terminates TLS.
- **AES decryption of wM-Bus telegrams** (OMS security mode 5, AES-128-CBC): feasible with the mbedTLS library that is already part of
  the ESP32 core. It would need a 32-digit key field per meter (stored like the other secrets and never returned by the API), the
  IV built from manufacturer, address and access number, and a plausibility check (decrypted data starts with `2F 2F`). The meter
  key itself has to come from the meter owner or utility.
- More than two meter slots, an English UI, an option to switch the access point off
  while the home network is connected.

Contributions are welcome.

## Credits

The layout of the Qundis telegrams was worked out with the help of the documentation and driver sources of the
[wmbusmeters](https://github.com/wmbusmeters/wmbusmeters) project (GPL-3.0). The code of this project is an
independent implementation.

## About this project

Developed with the assistance of Claude (Anthropic) and tested on real hardware (ESP32, CC1101, Qundis Q water 5.5 /
Q heat 5.5) by the maintainer. It has been tested with these two meters only; other meters may need changes.

## License

[MIT](LICENSE)
