# wMBus Gateway

Ein ESP32 + CC1101 als Empfänger für **Wireless-M-Bus-Wasser- und Wärmezähler (868 MHz)** mit eingebauter
Weboberfläche und **MQTT / Home-Assistant-Auto-Discovery**. Ohne Cloud, ohne zusätzlichen Hub, ohne YAML.

> [English](README.md) | Deutsch

Entwickelt und getestet mit **Qundis Q water 5.5** und **Qundis Q heat 5.5** (wM-Bus Modus C1, unverschlüsselt).
Andere OMS-Zähler mit gleichem Telegrammaufbau funktionieren eventuell ebenfalls.

> **Funktioniert nur mit unverschlüsselten Zählern.** Das Gerät kann keine AES-verschlüsselten wM-Bus-Telegramme lesen und hat
> keine Möglichkeit, einen Zählerschlüssel einzutragen. Viele Zähler vom Vermieter oder Versorger senden verschlüsselt; dafür
> braucht man den Schlüssel (beim Anbieter erfragen). Ein solcher Zähler wird gehört, erscheint aber als „Format unbekannt".

## Funktionen

- Bis zu **zwei Zähler** (jeweils Wasser oder Wärme, in der Oberfläche wählbar)
- **Weboberfläche** im eigenen WLAN des Geräts und im Heimnetz: Live-Werte, Diagramme (Stand / Verbrauch,
  6 Std – 3 Tage), Signalstärke, CSV-Export, Empfangsstatistik, Telegramm-Liste
- **Home Assistant** per MQTT-Auto-Discovery: Zählerstand, Stichtagswerte (Jahres- und Monatsende) samt Datum
  und Signalstärke je Zähler
- Verlauf und letzte Werte werden im **Flash gespeichert** und überstehen Neustarts
- **Firmware-Update über das Netz**, Watchdog, automatische Prüfung des Funkchips
- Captive-Portal-Weiterleitung: unbekannte Adressen führen im WLAN des Geräts zur Oberfläche

## Hardware und Verdrahtung

ESP32-Board plus CC1101-Modul **868 MHz** (das übliche grüne 8-polige Modul mit Federantenne).
Pin-Reihenfolge des Moduls: `VCC GND MOSI SCK MISO GDO2 GDO0 CSN`

| CC1101 | ESP32 |
|---|---|
| VCC | 3,3 V (**nicht 5 V**) |
| GND | GND |
| MOSI | GPIO 23 |
| SCK | GPIO 18 |
| MISO | GPIO 19 |
| GDO2 / GDO0 | GPIO 32 / 25 (von der Firmware nicht genutzt) |
| CSN | GPIO 5 |

Die Pins stehen in [`config.h`](config.h). Wenn du die Belegung deines Moduls nicht kennst, sucht das Programm in
[`tools/pinprobe`](tools/pinprobe) sie für dich.

## Bauen und flashen

Voraussetzungen: [arduino-cli](https://arduino.github.io/arduino-cli/) (oder Arduino IDE), ESP32-Core **3.x**
(getestet mit 3.3.12) und die Bibliothek **PubSubClient**.

```bash
arduino-cli core install esp32:esp32
arduino-cli lib install PubSubClient
arduino-cli compile -b esp32:esp32:esp32 .
arduino-cli upload  -b esp32:esp32:esp32 -p /dev/ttyUSB0 .
```

Das Standard-Partitionsschema verwenden (es enthält zwei Programmplätze, die für Updates über das Netz nötig sind).

## Erster Start

1. Gerät mit Strom versorgen und mit dem WLAN **`wmBus-Gateway`** verbinden, Anfangspasswort **`wmbus1234`**.
2. `http://192.168.4.1` im Browser öffnen und anmelden (Benutzer `admin`, Passwort `wmbus1234`).
3. Die Oberfläche verlangt zuerst ein **neues Passwort**. Es ersetzt das Anfangspasswort für die Anmeldung **und** für das
   WLAN des Geräts. Bis dahin sind alle Änderungen gesperrt. Danach mit dem neuen Passwort neu anmelden.
4. *Einstellungen → Zähler*: **Seriennummer** (8 Ziffern, steht auf dem Zähler) eintragen und die Art wählen.
5. *Einstellungen → WLAN* (2,4 GHz) und *Home Assistant* (MQTT-Broker), speichern. Das Gerät startet neu.

Danach ist die Oberfläche auch unter `http://wmbus.local` erreichbar.

Passwort vergessen? GPIO 0 (BOOT) im laufenden Betrieb 10 Sekunden mit GND verbinden: **Werkseinstellungen**
(löscht Einstellungen und Verlauf, danach gilt wieder das Anfangspasswort).

## Home Assistant

Mit konfiguriertem MQTT erscheinen die Entitäten automatisch (ein Gerät je Zähler): Zählerstand
(`total_increasing`, für das Energie-Dashboard geeignet), Wert und Datum der Jahres- und Monatsstichtage sowie die Signalstärke.

Das Energie-Dashboard kennt keine Wärme-Kategorie. Einen Wärmezähler kannst du dort als *Gasquelle ohne Kosten*
eintragen oder in einem eigenen Dashboard anzeigen.

## Sicherheit

Das Gerät ist für ein **vertrauenswürdiges Heimnetz** gedacht.

- Das öffentliche Anfangspasswort muss beim ersten Login ersetzt werden; bis dahin sind alle Änderungen (auch
  Firmware-Updates) gesperrt. Nach 5 falschen Anmeldungen werden alle Anfragen 60 Sekunden abgelehnt.
- CSRF-Schutz über den Pflicht-Header `X-Requested-With`, strenge `Content-Security-Policy`.
- Alle Eingaben werden geprüft, Texte werden maskiert und nur als Text angezeigt.
- **Funktioniert nur ohne Zertifikate:** Die Oberfläche läuft nur über **HTTP** (kein HTTPS), MQTT nur **ohne TLS**
  (keine Zertifikate, kein `mqtts://`). Ein Broker, der TLS erzwingt, kann nicht genutzt werden.
- **Nicht geschützt:** Die Verbindung ist **HTTP** und **MQTT ohne TLS** – Zugangsdaten sind für jeden lesbar, der
  im Netz mitschneiden kann. Einstellungen (auch WLAN- und MQTT-Passwort) liegen unverschlüsselt im Flash.
  Firmware-Abbilder sind nicht signiert.
- Nicht ins Internet stellen. Besser: eigenes WLAN/VLAN und einen MQTT-Benutzer mit minimalen Rechten.

Siehe [SECURITY.md](SECURITY.md). Alles Weitere (Web-API, Fehlersuche, Grenzen) steht in der [englischen README](README.md).

## Mögliche Erweiterungen

- **AES-Entschlüsselung** (OMS Sicherheitsmodus 5, AES-128-CBC): machbar mit mbedTLS, das der ESP32-Core schon enthält. Pro Zähler
  ein Schlüsselfeld (32 Ziffern, nie über die API ausgeben), der Startwert aus Hersteller, Adresse und Zählnummer, und eine
  Plausibilitätsprüfung (entschlüsselte Daten beginnen mit `2F 2F`). Den Schlüssel selbst bekommt man vom Besitzer oder Versorger.
- **MQTT über TLS** (`mqtts://`, Port 8883): machbar mit `WiFiClientSecure` und einem CA-Zertifikat auf dem Gerät – die
  nützlichste Verbesserung der Sicherheit.
- **HTTPS für die Oberfläche**: möglich, aber ein selbst signiertes Zertifikat löst Browser-Warnungen aus, es braucht viel
  Arbeitsspeicher, und die Firmware belegt schon 86 % des Programmplatzes. Praktisch: Reverse-Proxy davor, der TLS übernimmt.
- Verschlüsselte (AES) Telegramme, mehr als zwei Zähler, englische Oberfläche, AP automatisch aus bei Heimnetz-Verbindung.

## Über dieses Projekt

Entwickelt mit Unterstützung von Claude (Anthropic) und vom Maintainer auf echter Hardware getestet (ESP32, CC1101,
Qundis Q water 5.5 / Q heat 5.5). Getestet wurde nur mit diesen beiden Zählern; andere Zähler brauchen eventuell Anpassungen.

## Lizenz

[MIT](LICENSE)
