# Security

## Threat model

The gateway is designed for a **trusted home network**. It protects against casual access from other devices on
the same network and against browser-based attacks (CSRF, clickjacking, XSS). It does **not** protect against an
attacker who can read your network traffic or touch the device.

## Measures

| Area | Measure |
|---|---|
| Authentication | HTTP Basic auth (user `admin`) on every route, including the firmware update |
| Initial password | A new device starts with the public password `wmbus1234`. The first login forces a new password (8-63 characters, not the initial one); until then all state-changing routes, including firmware update, are rejected. The new password is used for the web login and the WiFi access point |
| Brute force | 5 failed logins lock all requests for 60 seconds |
| CSRF | Every state-changing request must carry `X-Requested-With: wmbus` (cannot be set by cross-site forms or images) |
| Browser hardening | `Content-Security-Policy`, `X-Frame-Options: DENY`, `X-Content-Type-Options: nosniff`, `Referrer-Policy: no-referrer`, `Cache-Control: no-store` |
| Input handling | Length and character checks for every setting (`config.h`), JSON escaping of strings, UI renders values as text only |
| Secrets in the API | Passwords are write-only: the API reports only whether one is set |
| Firmware update | Requires login and the CSRF header; a failed or aborted upload does not replace the running firmware |
| Recovery | Hardware factory reset (GPIO 0 low for 10 s); the initial password applies again afterwards |

## Known limitations

- **No TLS, no certificates.** The web UI is plain HTTP (no HTTPS) and MQTT is unencrypted (no `mqtts://`); a broker that
  requires TLS cannot be used. Credentials can be read by anyone who can sniff the network.
- Settings, including the WiFi and MQTT passwords, are kept **unencrypted in NVS flash**. Anyone with physical access
  can read them (ESP32 flash encryption is not enabled by this project).
- Firmware images are **not signed**; anybody with the web password can install any firmware.
- Until the user has chosen a new password, the WiFi access point and the web login use the public initial password.
  Anyone within WiFi range can connect during that time, so do the first setup right after powering the device.
- The WiFi access point is WPA2-PSK and uses the same password as the web login unless it is changed separately.
- Only **unencrypted** wireless M-Bus telegrams are supported. Unencrypted telegrams can be read by anybody in radio range; the gateway adds no protection there.
- Wireless M-Bus telegrams are received unauthenticated; a nearby transmitter can spoof values for a known serial number.

## Recommendations

- Keep the gateway in a separate VLAN / guest WLAN; do not forward it to the internet.
- Use a dedicated MQTT user that may only publish to `<prefix>/#` and `homeassistant/#`.
- Use a long, unique password.

## Possible improvements

- MQTT over TLS with a CA certificate stored on the device (`WiFiClientSecure`).
- HTTPS for the web UI (RAM and flash cost; self-signed certificates cause browser warnings) or a TLS-terminating reverse proxy.
- ESP32 flash encryption and signed firmware images (secure boot).
- If AES decryption for encrypted meters is added: store the meter keys like the other secrets (write-only API, never logged).

## Reporting a vulnerability

Please open a private security advisory on the repository (or contact the maintainer) instead of a public issue.
