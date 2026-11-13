// =============================================================================
//  web.h - HTTP server: single-page UI, JSON API, firmware update
//
//  Security model
//   * Every route requires HTTP Basic auth (user "admin", password cfg.webPass). A new device starts
//     with a public initial password; until the user has set a new one, all state-changing routes
//     except "set a new password" are rejected (mustChangePassword()).
//   * After 5 failed logins all requests are rejected for 60 s (brute-force throttle).
//   * State-changing routes (POST) additionally require the header
//     "X-Requested-With: wmbus". Browsers cannot add this header to cross-site form posts
//     or <img>/<script> requests, which blocks CSRF even though Basic credentials are
//     cached by the browser.
//   * Responses carry CSP / X-Frame-Options / nosniff headers. All user supplied strings
//     are validated (config.h) and escaped for JSON; the UI inserts them as text only.
//   * Transport is plain HTTP. Do not expose the gateway to untrusted networks.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <Update.h>
#include "config.h"
#include "meters.h"
#include "radio_cc1101.h"
#include "decoder.h"
#include "net.h"
#include "webui.h"

static WebServer web(80);
static uint32_t restartAt = 0;      // millis() timestamp of a pending restart (0 = none)

// ----------------------------------------------------------------------------
//  Authentication / response helpers
// ----------------------------------------------------------------------------
static uint8_t  authFails = 0;
static uint32_t lockUntil = 0;

static void securityHeaders() {
  web.sendHeader("X-Content-Type-Options", "nosniff");
  web.sendHeader("X-Frame-Options", "DENY");
  web.sendHeader("Referrer-Policy", "no-referrer");
  web.sendHeader("Cache-Control", "no-store");
}
static void sendJson(const String& body, int code = 200) {
  securityHeaders();
  web.send(code, "application/json; charset=utf-8", body);
}

// Check credentials (and the CSRF header for POST) without sending a response.
// Returns 0 = ok, 401 = bad / missing credentials, 403 = missing CSRF header, 429 = locked out.
static int checkAuth(bool needCsrfHeader) {
  if (lockUntil && (int32_t)(millis() - lockUntil) < 0) return 429;
  if (!web.authenticate("admin", cfg.webPass.c_str())) {
    if (web.hasHeader("Authorization") && ++authFails >= 5) { lockUntil = millis() + 60000; authFails = 0; }
    return 401;
  }
  authFails = 0;
  if (needCsrfHeader && web.header("X-Requested-With") != "wmbus") return 403;
  return 0;
}
static bool auth(bool needCsrfHeader = false, bool allowWithInitialPassword = false) {
  int r = checkAuth(needCsrfHeader);
  if (r == 0 && needCsrfHeader && !allowWithInitialPassword && mustChangePassword()) {
    sendJson("{\"ok\":false,\"err\":\"Bitte zuerst ein neues Passwort festlegen\",\"pwchange\":true}", 403);
    return false;
  }
  if (r == 0) return true;
  if (r == 401) web.requestAuthentication(BASIC_AUTH, "wMBus Gateway");
  else sendJson(String("{\"ok\":false,\"err\":\"") + (r == 429 ? "Too many failed logins, try again in a minute" : "Missing request header") + "\"}", r);
  return false;
}

// ----------------------------------------------------------------------------
//  GET handlers (read only)
// ----------------------------------------------------------------------------
static void handleIndex() {
  securityHeaders();
  web.sendHeader("Content-Security-Policy",
    "default-src 'none'; script-src 'unsafe-inline'; style-src 'unsafe-inline'; img-src data:; "
    "connect-src 'self'; base-uri 'none'; form-action 'self'; frame-ancestors 'none'");
  web.send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

static void handleState() {
  int32_t now = nowSec();
  String s; s.reserve(2000);
  s += "{\"up\":" + String(now) + ",\"sta\":" + (WiFi.status() == WL_CONNECTED ? "true" : "false");
  s += ",\"ip\":\"" + WiFi.localIP().toString() + "\",\"mqtt\":" + (mq.connected() ? "true" : "false");
  s += ",\"mqttOn\":" + String(mqttConfigured() ? "true" : "false");
  s += ",\"fw\":\"" FW_VERSION "\",\"build\":\"" __DATE__ " " __TIME__ "\",\"heap\":" + String(ESP.getFreeHeap());
  s += ",\"wrssi\":" + String(WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0);
  s += ",\"radio\":" + String(radioOk ? "true" : "false") + ",\"frames\":" + String(st.frames) + ",\"ok\":" + String(st.ok);
  s += ",\"crc\":" + String(st.crcFail) + ",\"foreign\":" + String(st.foreign) + ",\"resets\":" + String(st.radioResets);
  s += ",\"timeOk\":" + String(timeOk() ? "true" : "false");
  s += ",\"mustChange\":" + String(mustChangePassword() ? "true" : "false");
  s += ",\"apDefault\":" + String(cfg.apPass == INITIAL_PASSWORD ? "true" : "false");
  s += ",\"setup\":" + String(anyMeterEnabled() ? "false" : "true") + ",\"m\":[";
  for (int i = 0; i < NUM_SLOTS; i++) {
    Meter& m = M[i]; bool w = m.isWater();
    if (i) s += ",";
    if (!m.enabled) { s += "{\"en\":false}"; continue; }
    s += "{\"en\":true,\"name\":\"" + jsonEscape(cfg.slot[i].name) + "\",\"id\":\"" + m.idStr + "\",\"water\":" + (w ? "true" : "false");
    s += ",\"unit\":\"" + String(w ? "m³" : "kWh") + "\",\"have\":" + (m.have ? "true" : "false");
    s += ",\"total\":" + (m.have ? fv(m.tot, w) : String("null"));
    s += ",\"yv\":" + (m.hasPeriods ? fv(m.yearVal, w) : String("null")) + ",\"mv\":" + (m.hasPeriods ? fv(m.monthVal, w) : String("null"));
    s += ",\"yd\":" + (m.hasPeriods ? "\"" + dateDe(m.yearDate) + "\"" : String("null"));
    s += ",\"md\":" + (m.hasPeriods ? "\"" + dateDe(m.monthDate) + "\"" : String("null"));
    s += ",\"rssi\":" + String(m.rssi) + ",\"age\":" + String(m.have ? now - m.last : 0) + ",\"cnt\":" + String(m.cnt);
    s += ",\"heard\":" + String(m.heard >= 0 ? now - m.heard : -1) + ",\"unp\":" + String(m.unparsed) + "}";
  }
  sendJson(s + "]}");
}

static int slotArg() { int i = web.arg("m").toInt(); return (i < 0 || i >= NUM_SLOTS) ? 0 : i; }

static void handleHist() {          // history as parallel arrays: age in seconds, value, RSSI
  int i = slotArg(); Meter& m = M[i]; int32_t now = nowSec();
  String t = "[", v = "[", r = "[";
  if (m.enabled) {
    t.reserve(m.hn * 7 + 4); v.reserve(m.hn * 10 + 4); r.reserve(m.hn * 5 + 4);
    int start = (m.hn < HN) ? 0 : m.head;
    for (int k = 0; k < m.hn; k++) {
      Hist& h = m.h[(start + k) % HN];
      if (k) { t += ","; v += ","; r += ","; }
      t += String(now - h.t); v += fv(h.tot, m.isWater()); r += String((int)h.rssi);
    }
  }
  sendJson("{\"t\":" + t + "],\"v\":" + v + "],\"r\":" + r + "]}");
}

static void handleLog() {           // recent valid telegrams, newest first
  int32_t now = nowSec(); String s = "["; int start = (logN < LOGN) ? 0 : logHead;
  for (int k = logN - 1; k >= 0; k--) {
    LogEntry& e = logbuf[(start + k) % LOGN];
    if (k != logN - 1) s += ",";
    s += "{\"a\":" + String(now - e.t) + ",\"m\":" + String(e.slot) + ",\"c\":" + String(e.ci) + ",\"r\":" + String((int)e.rssi);
    s += ",\"v\":" + fv(e.tot, M[e.slot].isWater()) + "}";
  }
  sendJson(s + "]");
}

static void handleCsv() {
  int i = slotArg(); Meter& m = M[i]; int32_t now = nowSec(), off = epochOffset();
  securityHeaders();
  web.sendHeader("Content-Disposition", String("attachment; filename=\"") + (m.isWater() ? "water_" : "heat_") + m.idStr + ".csv\"");
  String s = String("Time (UTC);Total ") + (m.isWater() ? "m3" : "kWh") + ";RSSI dBm\n";
  if (m.enabled) {
    int start = (m.hn < HN) ? 0 : m.head;
    for (int k = 0; k < m.hn; k++) {
      Hist& h = m.h[(start + k) % HN];
      if (timeOk()) { time_t e = h.t + off; struct tm t; gmtime_r(&e, &t); char b[24]; strftime(b, sizeof b, "%Y-%m-%d %H:%M:%S", &t); s += b; }
      else s += String(now - h.t) + " s ago";
      s += ";" + fv(h.tot, m.isWater()) + ";" + String((int)h.rssi) + "\n";
    }
  }
  web.send(200, "text/csv; charset=utf-8", s);
}

static void handleDebug() {         // last undecodable frame per meter (hex), for diagnostics
  String s = "{";
  for (int i = 0; i < NUM_SLOTS; i++) s += String(i ? "," : "") + "\"" + i + "\":\"" + dbgHex[i] + "\"";
  sendJson(s + "}");
}

static void handleScan() {          // visible WiFi networks (blocks for ~2 s)
  int n = WiFi.scanNetworks(false, false);
  String s = "[";
  for (int i = 0; i < n; i++) { if (i) s += ","; s += "{\"s\":\"" + jsonEscape(WiFi.SSID(i)) + "\",\"r\":" + String(WiFi.RSSI(i)) + "}"; }
  WiFi.scanDelete();
  sendJson(s + "]");
}

static void handleGetConfig() {     // passwords are never sent back, only whether one is set
  String s = "{";
  for (int i = 0; i < NUM_SLOTS; i++) {
    s += "\"n" + String(i) + "\":\"" + jsonEscape(cfg.slot[i].name) + "\",\"i" + String(i) + "\":\"" + cfg.slot[i].id + "\",\"k" + String(i) + "\":" + String(cfg.slot[i].kind) + ",";
  }
  s += "\"ss\":\"" + jsonEscape(cfg.staSsid) + "\",\"hasSp\":" + (cfg.staPass.length() ? "true" : "false");
  s += ",\"mh\":\"" + jsonEscape(cfg.mqttHost) + "\",\"mp\":" + String(cfg.mqttPort) + ",\"mu\":\"" + jsonEscape(cfg.mqttUser) + "\"";
  s += ",\"hasMw\":" + String(cfg.mqttPass.length() ? "true" : "false") + ",\"mb\":\"" + jsonEscape(cfg.mqttBase) + "\"";
  s += ",\"as\":\"" + jsonEscape(cfg.apSsid) + "\"}";
  sendJson(s);
}

// ----------------------------------------------------------------------------
//  POST handlers (state changing; CSRF header required)
// ----------------------------------------------------------------------------
// Only fields present in the request are changed (settings are saved section by section).
// Everything is validated first, then applied, so a rejected request changes nothing.
static void handlePostConfig() {
  auto fail = [](const String& msg) { sendJson("{\"ok\":false,\"err\":\"" + jsonEscape(msg) + "\"}", 400); };

  // ---- meters ----
  String id[NUM_SLOTS], name[NUM_SLOTS]; uint8_t kind[NUM_SLOTS];
  for (int i = 0; i < NUM_SLOTS; i++) {
    String a = "i" + String(i), n = "n" + String(i), k = "k" + String(i);
    id[i] = cfg.slot[i].id; name[i] = cfg.slot[i].name; kind[i] = cfg.slot[i].kind;
    if (web.hasArg(a)) { id[i] = web.arg(a); id[i].trim(); if (id[i].length() && !validId(id[i])) return fail("Seriennummer: genau 8 Ziffern (oder leer = Zähler aus)"); }
    if (web.hasArg(n) && web.arg(n).length()) { if (!validName(web.arg(n))) return fail("Name: 1 bis 32 Zeichen"); name[i] = web.arg(n); }
    if (web.hasArg(k)) { int v = web.arg(k).toInt(); if (v != KIND_WATER && v != KIND_HEAT) return fail("Ungültige Zählerart"); kind[i] = v; }
  }
  if (id[0].length() && id[0] == id[1]) return fail("Beide Zähler haben dieselbe Seriennummer");

  // ---- network / security ----
  if (web.hasArg("ss") && web.arg("ss").length() && !validSsid(web.arg("ss"))) return fail("WLAN-Name: 1 bis 32 Zeichen");
  if (web.arg("sp").length() && !validWpaPass(web.arg("sp"))) return fail("WLAN-Passwort: 8 bis 63 Zeichen");
  if (web.hasArg("mh") && web.arg("mh").length() && !validHost(web.arg("mh"))) return fail("MQTT-Broker: ungültiger Name oder IP");
  if (web.arg("mp").length() && (web.arg("mp").toInt() < 1 || web.arg("mp").toInt() > 65535)) return fail("MQTT-Port: 1 bis 65535");
  if (web.arg("mu").length() > 64 || web.arg("mw").length() > 64) return fail("MQTT-Benutzer/Passwort: höchstens 64 Zeichen");
  if (web.arg("mb").length() && !validTopic(web.arg("mb"))) return fail("Topic-Präfix: nur Buchstaben, Ziffern, _ - und /");
  if (web.hasArg("as") && !validSsid(web.arg("as"))) return fail("AP-Name: 1 bis 32 Zeichen");
  if (web.arg("ap").length() && !validWpaPass(web.arg("ap"))) return fail("AP-Passwort: 8 bis 63 Zeichen");
  if (web.arg("wp").length() && !validWpaPass(web.arg("wp"))) return fail("Passwort: 8 bis 63 Zeichen");
  if (web.arg("wp") == INITIAL_PASSWORD || web.arg("ap") == INITIAL_PASSWORD) return fail("Das neue Passwort darf nicht das Anfangspasswort sein");
  if (mustChangePassword() && !web.arg("wp").length()) { sendJson("{\"ok\":false,\"err\":\"Bitte zuerst ein neues Passwort festlegen\",\"pwchange\":true}", 403); return; }

  // ---- apply ----
  for (int i = 0; i < NUM_SLOTS; i++) {
    bool changed = (id[i] != cfg.slot[i].id) || (kind[i] != cfg.slot[i].kind);
    if (changed) {
      if (validId(cfg.slot[i].id) && id[i] != cfg.slot[i].id) queueDiscoveryCleanup(i, cfg.slot[i].id);   // remove old HA entities
      deleteSlotHistory(i);                                                                              // history belongs to the old meter
    }
    cfg.slot[i].id = id[i]; cfg.slot[i].kind = kind[i];
    cfg.slot[i].name = web.arg("n" + String(i)).length() ? name[i] : (changed ? defaultName(kind[i]) : name[i]);
  }
  if (web.hasArg("ss")) cfg.staSsid = web.arg("ss");
  if (web.arg("sp").length()) cfg.staPass = web.arg("sp");
  if (web.hasArg("mh")) cfg.mqttHost = web.arg("mh");
  if (web.arg("mp").length()) cfg.mqttPort = web.arg("mp").toInt();
  if (web.hasArg("mu")) cfg.mqttUser = web.arg("mu");
  if (web.arg("mw").length()) cfg.mqttPass = web.arg("mw");
  if (web.arg("mb").length()) cfg.mqttBase = web.arg("mb");
  if (web.hasArg("as")) cfg.apSsid = web.arg("as");
  if (web.arg("ap").length()) cfg.apPass = web.arg("ap");
  if (web.arg("wp").length()) {
    bool first = mustChangePassword();
    cfg.webPass = web.arg("wp");
    if (first && !web.arg("ap").length()) cfg.apPass = cfg.webPass;   // first setup: one password for login and access point
  }
  saveConfig();
  sendJson("{\"ok\":true,\"restart\":true}");
  restartAt = millis() + 1500;      // restart after the response was delivered
}

static void handleRestart() { sendJson("{\"ok\":true}"); restartAt = millis() + 800; }
static void handleFactoryReset() { factoryResetStorage(); sendJson("{\"ok\":true}"); restartAt = millis() + 800; }

// ----------------------------------------------------------------------------
//  Firmware update (OTA): multipart upload of the application .bin
// ----------------------------------------------------------------------------
static bool otaAllowed = false;     // set at upload start after the auth check, cleared at the end

static void handleUpdateUpload() {
  HTTPUpload& u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    otaAllowed = (checkAuth(true) == 0) && !mustChangePassword();
    if (!otaAllowed) return;
    Serial.printf("OTA update: %s\n", u.filename.c_str());
    Update.begin(UPDATE_SIZE_UNKNOWN);
  } else if (!otaAllowed) {
    return;                         // ignore data of unauthenticated uploads
  } else if (u.status == UPLOAD_FILE_WRITE) {
    Update.write(u.buf, u.currentSize);
  } else if (u.status == UPLOAD_FILE_END) {
    Update.end(true); otaAllowed = false; Serial.printf("OTA finished: %u bytes\n", u.totalSize);
  } else if (u.status == UPLOAD_FILE_ABORTED) {
    Update.abort(); otaAllowed = false;
  }
}
static void handleUpdateDone() {    // the auth check is repeated here: never rely on state of an earlier request
  if (!auth(true)) return;
  if (Update.hasError() || Update.isRunning()) { sendJson("{\"ok\":false,\"err\":\"Update fehlgeschlagen\"}", 500); return; }
  sendJson("{\"ok\":true}");
  restartAt = millis() + 1500;
}

// ----------------------------------------------------------------------------
static void setupWeb() {
  const char* headers[] = {"X-Requested-With"};     // "Authorization" is collected by the library itself
  web.collectHeaders(headers, 1);
  web.on("/",            HTTP_GET,  [] { if (auth()) handleIndex(); });
  web.on("/api/state",   HTTP_GET,  [] { if (auth()) handleState(); });
  web.on("/api/hist",    HTTP_GET,  [] { if (auth()) handleHist(); });
  web.on("/api/log",     HTTP_GET,  [] { if (auth()) handleLog(); });
  web.on("/api/csv",     HTTP_GET,  [] { if (auth()) handleCsv(); });
  web.on("/api/debug",   HTTP_GET,  [] { if (auth()) handleDebug(); });
  web.on("/api/scan",    HTTP_GET,  [] { if (auth()) handleScan(); });
  web.on("/api/config",  HTTP_GET,  [] { if (auth()) handleGetConfig(); });
  web.on("/api/config",  HTTP_POST, [] { if (auth(true, true)) handlePostConfig(); });   // allowed with the initial password (to set a new one)
  web.on("/api/restart", HTTP_POST, [] { if (auth(true)) handleRestart(); });
  web.on("/api/factory", HTTP_POST, [] { if (auth(true)) handleFactoryReset(); });
  web.on("/api/update",  HTTP_POST, handleUpdateDone, handleUpdateUpload);
  web.onNotFound([] {               // captive portal: unknown URLs (connectivity checks) go to the start page
    if (web.uri().startsWith("/api")) { web.send(404, "text/plain", "not found"); return; }
    web.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
    web.send(302, "text/plain", "");
  });
  web.begin();
}
