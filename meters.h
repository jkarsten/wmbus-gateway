// =============================================================================
//  meters.h - runtime meter state, history ring buffers, flash persistence,
//             recent-telegram log and small formatting helpers
// =============================================================================
#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <time.h>
#include "config.h"

#define HN 288               // history points per meter
#define HIST_INTERVAL 900    // seconds between two history points (288 * 15 min = 3 days)
#define LOGN 40              // recent valid telegrams kept in RAM

// One history point. t is "uptime seconds" (not wall clock) so it works without NTP.
struct Hist { int32_t t; int32_t tot; int8_t rssi; };

struct Meter {
  bool    enabled = false;           // slot has a valid serial number
  uint8_t kind = KIND_WATER;
  uint8_t id[4] = {0};               // serial number as it appears on air (little-endian BCD)
  char    idStr[9] = {0};            // serial number as printed on the meter

  // Latest decoded values. Water is stored in 1/1000 m3, heat in kWh.
  bool    have = false;              // at least one valid telegram (or restored from flash)
  bool    hasPeriods = false;        // year / month reference values are known
  bool    dirty = false;             // new data that was not yet published via MQTT
  int32_t tot = 0, yearVal = 0, monthVal = 0;
  uint16_t yearDate = 0, monthDate = 0;   // raw M-Bus type G dates
  int     rssi = 0;
  int32_t last = 0;                  // uptime seconds of the last valid telegram
  int32_t lastHist = 0;              // uptime seconds of the last history point
  int32_t heard = -1;                // uptime seconds of the last CRC-valid frame (any format), -1 = never
  uint32_t cnt = 0, unparsed = 0;    // valid telegrams / CRC-valid frames with unknown format

  Hist h[HN]; int hn = 0, head = 0;  // ring buffer: hn = used entries, head = oldest when full

  bool isWater() const { return kind == KIND_WATER; }
};

static Meter M[NUM_SLOTS];

// Reception statistics (shown on the System page)
struct Stats { uint32_t frames = 0, ok = 0, crcFail = 0, foreign = 0, radioResets = 0; };
static Stats st;

// ----------------------------------------------------------------------------
//  Setup meters from the configuration
// ----------------------------------------------------------------------------
// Example: serial "12345678" is transmitted as bytes 78 56 34 12 (BCD, least significant byte first).
static void applyMeterConfig() {
  for (int i = 0; i < NUM_SLOTS; i++) {
    Meter& m = M[i];
    const String& id = cfg.slot[i].id;
    m.enabled = validId(id);
    m.kind = cfg.slot[i].kind;
    memset(m.id, 0, sizeof m.id); memset(m.idStr, 0, sizeof m.idStr);
    if (!m.enabled) continue;
    strncpy(m.idStr, id.c_str(), 8);
    for (int b = 0; b < 4; b++) m.id[3 - b] = (uint8_t)strtol(id.substring(b * 2, b * 2 + 2).c_str(), nullptr, 16);
  }
}

static bool anyMeterEnabled() {
  for (int i = 0; i < NUM_SLOTS; i++) if (M[i].enabled) return true;
  return false;
}

// ----------------------------------------------------------------------------
//  Time helpers. The ESP32 has no RTC: wall clock = uptime + offset once NTP is synced.
// ----------------------------------------------------------------------------
static bool timeOk() { return time(nullptr) > 1700000000; }
static int32_t nowSec() { return (int32_t)(millis() / 1000); }
static int32_t epochOffset() { return (int32_t)time(nullptr) - nowSec(); }

// ----------------------------------------------------------------------------
//  Formatting helpers
// ----------------------------------------------------------------------------
// Decode an M-Bus type G date (16 bit) into day / month / year.
static void dateParts(uint16_t w, int& d, int& m, int& y) {
  d = w & 0x1F; m = (w >> 8) & 0x0F; y = 2000 + (((w >> 5) & 7) | ((w >> 9) & 0x78));
}
static String dateDe(uint16_t w)  { int d, m, y; dateParts(w, d, m, y); char b[12]; snprintf(b, sizeof b, "%02d.%02d.%04d", d, m, y); return b; }
static String dateIso(uint16_t w) { int d, m, y; dateParts(w, d, m, y); char b[12]; snprintf(b, sizeof b, "%04d-%02d-%02d", y, m, d); return b; }

// Number as JSON value. milli=true: value is in 1/1000 units (water, m3). Negative = invalid -> "null".
static String fv(int32_t v, bool milli) {
  if (v < 0) return "null";
  char b[20];
  if (milli) snprintf(b, sizeof b, "%ld.%03ld", (long)(v / 1000), (long)(v % 1000));
  else       snprintf(b, sizeof b, "%ld", (long)v);
  return b;
}

// ----------------------------------------------------------------------------
//  History: RAM ring buffer + append-only files in LittleFS (/h<slot>.bin).
//  Only one 12-byte record per meter every 15 minutes is written; LittleFS spreads
//  the writes over the partition, so flash wear is negligible.
// ----------------------------------------------------------------------------
struct Rec     { int32_t epoch; int32_t tot; int8_t rssi; };                                  // history file record
struct LastRec { int32_t epoch; int32_t tot, ty, tm; uint16_t dy, dm; int8_t rssi; uint8_t hasY; };  // /l<slot>.bin

static bool histRestored = false;   // true after restoreHistory() ran (needs NTP time)

static void ringPush(Meter& m, const Hist& h) {
  int idx = (m.hn < HN) ? m.hn++ : m.head;   // overwrite the oldest entry when full
  m.h[idx] = h;
  if (m.hn == HN) m.head = (idx + 1) % HN;
}

// Keep only the newest HN records in a history file (called when the file got too big).
static void compactFile(int slot) {
  String fn = "/h" + String(slot) + ".bin", tmp = "/h" + String(slot) + ".tmp";
  File f = LittleFS.open(fn, "r"); if (!f) return;
  int n = f.size() / sizeof(Rec), keep = n < HN ? n : HN;
  Rec* buf = (Rec*)malloc(keep * sizeof(Rec));
  if (!buf) { f.close(); return; }
  f.seek((n - keep) * sizeof(Rec)); f.read((uint8_t*)buf, keep * sizeof(Rec)); f.close();
  File o = LittleFS.open(tmp, "w");
  if (o) { o.write((uint8_t*)buf, keep * sizeof(Rec)); o.close(); LittleFS.remove(fn); LittleFS.rename(tmp, fn); }
  free(buf);
}

static void persistPoint(int slot) {
  if (!timeOk() || !histRestored) return;    // epoch timestamps need NTP; avoid overwriting before restore
  Meter& m = M[slot]; int32_t off = epochOffset();
  Rec r = { m.lastHist + off, m.tot, (int8_t)m.rssi };
  File f = LittleFS.open("/h" + String(slot) + ".bin", "a");
  if (f) { f.write((uint8_t*)&r, sizeof r); size_t sz = f.size(); f.close(); if (sz > HN * 2 * sizeof(Rec)) compactFile(slot); }
  LastRec l = { m.last + off, m.tot, m.yearVal, m.monthVal, m.yearDate, m.monthDate, (int8_t)m.rssi, (uint8_t)m.hasPeriods };
  File g = LittleFS.open("/l" + String(slot) + ".bin", "w");
  if (g) { g.write((uint8_t*)&l, sizeof l); g.close(); }
}

// Load history and last known values from flash. Runs once, as soon as the wall clock is known,
// and merges points collected since boot behind the restored ones.
static void restoreHistory() {
  int32_t off = epochOffset();
  for (int i = 0; i < NUM_SLOTS; i++) {
    Meter& m = M[i];
    if (!m.enabled) continue;
    // Save points collected since boot
    int cn = m.hn; Hist* cur = (Hist*)malloc((cn ? cn : 1) * sizeof(Hist));
    if (!cur) continue;
    int start = (m.hn < HN) ? 0 : m.head;
    for (int k = 0; k < cn; k++) cur[k] = m.h[(start + k) % HN];
    m.hn = 0; m.head = 0;
    // Restored points first ...
    int32_t lastT = INT32_MIN;
    File f = LittleFS.open("/h" + String(i) + ".bin", "r");
    if (f) {
      int n = f.size() / sizeof(Rec); Rec r;
      int skip = n > HN ? n - HN : 0; f.seek(skip * sizeof(Rec));
      while (f.read((uint8_t*)&r, sizeof r) == sizeof r) {
        Hist h = { r.epoch - off, r.tot, r.rssi };
        if (h.t <= lastT) continue;           // ignore out-of-order records
        ringPush(m, h); lastT = h.t;
      }
      f.close();
    }
    // ... then the points of this boot
    for (int k = 0; k < cn; k++) if (cur[k].t > lastT) { ringPush(m, cur[k]); lastT = cur[k].t; }
    free(cur);
    if (lastT != INT32_MIN) m.lastHist = lastT;
    // Last known values (only if no telegram was received since boot)
    File g = LittleFS.open("/l" + String(i) + ".bin", "r"); LastRec l;
    if (g && !m.have && g.read((uint8_t*)&l, sizeof l) == sizeof l) {
      m.have = true; m.tot = l.tot; m.yearVal = l.ty; m.monthVal = l.tm; m.yearDate = l.dy; m.monthDate = l.dm;
      m.hasPeriods = l.hasY; m.rssi = l.rssi; m.last = l.epoch - off; m.dirty = false;
    }
    if (g) g.close();
  }
  histRestored = true;
  Serial.println("History restored from flash");
}

static void addHistory(int slot) {
  Meter& m = M[slot]; int32_t now = nowSec();
  if (m.hn > 0 && now - m.lastHist < HIST_INTERVAL) return;
  m.lastHist = now;
  ringPush(m, { now, m.tot, (int8_t)m.rssi });
  persistPoint(slot);
}

// ----------------------------------------------------------------------------
//  Recent valid telegrams (RAM only, shown on the System page)
// ----------------------------------------------------------------------------
struct LogEntry { int32_t t; int32_t tot; uint8_t slot; uint8_t ci; int8_t rssi; };
static LogEntry logbuf[LOGN];
static int logN = 0, logHead = 0;

static void logPush(int slot, uint8_t ci, int rssi, int32_t tot) {
  int idx = (logN < LOGN) ? logN++ : logHead;
  logbuf[idx] = { nowSec(), tot, (uint8_t)slot, ci, (int8_t)rssi };
  if (logN == LOGN) logHead = (idx + 1) % LOGN;
}
