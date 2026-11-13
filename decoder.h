// =============================================================================
//  decoder.h - wireless M-Bus frame decoding (format A, mode C1) for Qundis
//              Q water 5.5 / Q heat 5.5 and other OMS meters that use the same layout.
//
//  Frame layout (after the 0xCD marker):  L C M(2) A(6) | CRC | data blocks of 16 bytes + CRC ...
//  Each block is followed by a 16-bit CRC (EN 13757-4, polynomial 0x3D65). The CI field
//  (byte 10 of the cleaned frame) selects the payload type:
//    0x72  OMS long header (12 bytes), then DIF/VIF data records
//    0x7A  OMS short header (4 bytes: ACC, status, config word), then data records
//    0x78  no header; Qundis manufacturer block 0DFF5F with a fixed 53-byte layout
// =============================================================================
#pragma once
#include <Arduino.h>
#include "config.h"
#include "meters.h"

static String dbgHex[NUM_SLOTS];   // last frame of a known meter that could not be decoded (diagnostics)

// CRC-16 as used by wireless M-Bus (poly 0x3D65, init 0, result inverted).
static uint16_t crc16(const uint8_t* d, int n) {
  uint16_t c = 0;
  for (int i = 0; i < n; i++) {
    c ^= (uint16_t)d[i] << 8;
    for (int b = 0; b < 8; b++) c = (c & 0x8000) ? ((c << 1) ^ 0x3D65) : (c << 1);
  }
  return c ^ 0xFFFF;
}

// Verify all block CRCs and copy the payload (without CRCs) to out. b starts at the L field.
static bool stripCrc(const uint8_t* b, int len, uint8_t* out, int outMax, int& outLen) {
  int remaining = b[0] + 1, pos = 0; outLen = 0; bool first = true;
  while (remaining > 0) {
    int n = first ? (remaining < 10 ? remaining : 10) : (remaining < 16 ? remaining : 16);
    first = false;
    if (pos + n + 2 > len || outLen + n > outMax) return false;       // truncated or too long
    uint16_t crc = (b[pos + n] << 8) | b[pos + n + 1];
    if (crc != crc16(b + pos, n)) return false;
    memcpy(out + outLen, b + pos, n); outLen += n; pos += n + 2; remaining -= n;
  }
  return true;
}

// 4 BCD bytes (little-endian) -> number, -1 if a nibble is not a decimal digit.
static int32_t bcd4(const uint8_t* p) {
  int32_t v = 0;
  for (int i = 3; i >= 0; i--) { int h = p[i] >> 4, l = p[i] & 15; if (h > 9 || l > 9) return -1; v = v * 100 + h * 10 + l; }
  return v;
}

// Decoded values of one telegram. -1 = not present.
struct Reading { int32_t tot = -1, yearVal = -1, monthVal = -1; uint16_t yearDate = 0, monthDate = 0; };

// Parse standard DIF/VIF data records. Storage 0 = current total, 1 = reference value at the
// end of last year, 17 = reference value at the end of last month. Water meters are scaled to
// 1/1000 m3, heat meters to whole kWh. Returns true if a current total was found.
static bool parseRecords(const uint8_t* p, int pl, bool water, Reading& r) {
  int i = 0; bool gotTotal = false;
  while (i < pl) {
    uint8_t dif = p[i++];
    if (dif == 0x2F) continue;                       // idle filler
    if ((dif & 0x0F) == 0x0F) break;                 // manufacturer specific data follows
    int storage = (dif >> 6) & 1, shift = 1; uint8_t d = dif;
    while ((d & 0x80) && i < pl) { d = p[i++]; storage |= (d & 0x0F) << shift; shift += 4; }   // DIFE chain
    if (i >= pl) break;
    uint8_t vif = p[i++], v = vif;
    while ((v & 0x80) && i < pl) v = p[i++];         // VIFE chain
    int code = dif & 0x0F, n;                        // data field length
    switch (code) {
      case 0: n = 0; break;  case 1: n = 1; break;  case 2: n = 2; break;  case 3: n = 3; break;
      case 4: case 5: case 12: n = 4; break;         case 6: case 14: n = 6; break;  case 7: n = 8; break;
      case 9: n = 1; break;  case 10: n = 2; break;  case 11: n = 3; break;
      default: return gotTotal;                      // variable length / special: stop parsing
    }
    if (i + n > pl) break;
    const uint8_t* q = p + i; i += n;

    if (vif == 0x6C && n == 2) {                     // date (type G)
      uint16_t w = q[0] | (q[1] << 8);
      if (storage == 1) r.yearDate = w; else if (storage == 17) r.monthDate = w;
      continue;
    }
    if (n != 4) continue;                            // only 32 bit values are of interest
    int32_t x = (code == 12) ? bcd4(q) : (int32_t)(q[0] | (q[1] << 8) | (q[2] << 16) | ((uint32_t)q[3] << 24));
    if (x < 0) continue;
    if (water) {
      if ((vif & 0xF8) != 0x10) continue;            // volume: 10^(n-6) m3
      for (int e = (vif & 7) - 3; e > 0; e--) x *= 10;   // -> 1/1000 m3
      for (int e = (vif & 7) - 3; e < 0; e++) x /= 10;
    } else {
      if ((vif & 0xF8) != 0x00) continue;            // energy: 10^(n-3) Wh
      for (int e = (vif & 7) - 6; e > 0; e--) x *= 10;   // -> kWh
      for (int e = (vif & 7) - 6; e < 0; e++) x /= 10;
    }
    if (storage == 0) { r.tot = x; gotTotal = true; }
    else if (storage == 1) r.yearVal = x;
    else if (storage == 17) r.monthVal = x;
  }
  return gotTotal;
}

// Qundis "walk-by" block (CI 0x78): fixed 53-byte layout inside manufacturer record 0DFF5F.
static bool parseQundisBlock(const uint8_t* p, int pl, Reading& r) {
  for (int i = 0; i + 4 < pl; i++) {
    if (p[i] != 0x0D || p[i + 1] != 0xFF || p[i + 2] != 0x5F) continue;
    const uint8_t* b = p + i + 4; int bl = p[i + 3];
    if (bl != 53 || i + 4 + bl > pl || b[1] != 0x82) return false;   // unknown layout
    r.tot = bcd4(b + 12);
    r.yearDate = b[16] | (b[17] << 8);  r.yearVal = bcd4(b + 18);
    r.monthDate = b[22] | (b[23] << 8); r.monthVal = bcd4(b + 24);
    return r.tot >= 0;
  }
  return false;
}

// Handle one raw burst from the radio: validate, identify the meter, decode, store.
static void decodeFrame(const uint8_t* raw, int len, int rssi) {
  if (len > 0 && raw[0] == 0x54) { raw++; len--; }          // part of the sync pattern, depends on bit alignment
  if (len < 14 || raw[0] != 0xCD) return;                   // 0xCD = frame format A marker of mode C1
  st.frames++;

  uint8_t d[200]; int dl;
  if (!stripCrc(raw + 1, len - 1, d, sizeof d, dl) || dl < 12) { st.crcFail++; return; }

  int slot = -1;                                            // does the address belong to one of our meters?
  for (int i = 0; i < NUM_SLOTS; i++) if (M[i].enabled && !memcmp(d + 4, M[i].id, 4)) slot = i;
  if (slot < 0) { st.foreign++; return; }
  Meter& m = M[slot];
  m.heard = nowSec();                                       // a CRC-valid frame of this meter was received

  const uint8_t ci = d[10];
  const uint8_t* p = d + 11; int pl = dl - 11;
  Reading r; bool ok = false;
  if (ci == 0x78) {
    ok = parseQundisBlock(p, pl, r);
  } else if ((ci == 0x72 && pl > 12) || (ci == 0x7A && pl > 4)) {
    int hdr = (ci == 0x72) ? 12 : 4;
    uint16_t config = p[hdr - 2] | (p[hdr - 1] << 8);       // bits 8..12 = encryption mode, 0 = none
    if (((config >> 8) & 0x1F) == 0) ok = parseRecords(p + hdr, pl - hdr, m.isWater(), r);
  }
  if (!ok || r.tot < 0) {                                   // known meter but unknown / encrypted format
    m.unparsed++; dbgHex[slot] = "ci=" + String(ci, HEX) + " len=" + String(dl) + " ";
    for (int i = 0; i < dl && i < 90; i++) { char b[3]; snprintf(b, sizeof b, "%02X", d[i]); dbgHex[slot] += b; }
    return;
  }

  st.ok++;
  logPush(slot, ci, rssi, r.tot);
  m.have = true; m.tot = r.tot; m.rssi = rssi; m.last = nowSec(); m.cnt++; m.dirty = true;
  if (r.yearVal >= 0 && r.monthVal >= 0) {
    m.hasPeriods = true; m.yearVal = r.yearVal; m.monthVal = r.monthVal; m.yearDate = r.yearDate; m.monthDate = r.monthDate;
  }
  addHistory(slot);

  Serial.printf("%s %s: total=%s %s", m.isWater() ? "Water" : "Heat", m.idStr, fv(r.tot, m.isWater()).c_str(), m.isWater() ? "m3" : "kWh");
  if (m.hasPeriods) Serial.printf(" | %s=%s | %s=%s", dateDe(m.yearDate).c_str(), fv(m.yearVal, m.isWater()).c_str(), dateDe(m.monthDate).c_str(), fv(m.monthVal, m.isWater()).c_str());
  Serial.printf(" | rssi=%d dBm\n", rssi);
}
