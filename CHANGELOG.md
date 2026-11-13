# Changelog

## 2.2.1
- UI: a single enabled meter is shown centered instead of left-aligned next to an empty column.

## 2.2.0
- Meters are optional: a slot with an empty serial number is disabled (no card, no Home Assistant entities).
  Each slot can be set to *water* or *heat*.
- When a meter is removed or its serial number changes, its Home Assistant entities and stored history are removed.
- Generic OMS record parser (DIF/VIF): supports the long header (CI 0x72) and short header (CI 0x7A); telegrams with
  encryption are reported as "unknown format" instead of showing wrong values.
- Security: forced password change on first login (all changes, including firmware update, are blocked until then),
  login throttling, CSRF header on all POST routes, security headers, strict input validation, hardware factory reset.
- Code reorganised into modules; all comments in English.

## 2.1.1
- Consumption chart: bars sit on fixed time windows, the last bar is no longer clipped, round times on the axis.
- Integer axis labels for small value ranges; firmware version with build date in the UI.

## 2.1.0
- Heat meter: standard OMS telegram (CI 0x72) is decoded. Before, it was discarded and the meter seemed silent at times.
- Diagnostics: per meter "heard" and "unknown format", `/api/debug`.
- System page: list of recent valid telegrams, reception quality with legend.

## 2.0.0
- Own access point, web UI with charts and settings, login.
- MQTT with Home Assistant auto-discovery.
- History and last values in flash (LittleFS).
- Over-the-air updates, watchdog, radio chip health check, mDNS (`wmbus.local`).
