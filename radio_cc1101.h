// =============================================================================
//  radio_cc1101.h - minimal CC1101 driver for wireless M-Bus (868.95 MHz, mode T1/C1)
//
//  The radio is configured for 100 kcps 2-FSK and is read as a raw byte stream.
//  Framing (3-of-6 coding, CRC blocks) is handled in decoder.h.
// =============================================================================
#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "config.h"
#include "meters.h"

static SPIClass spi(VSPI);
static int curSync = 0;        // index into syncWords[] currently armed
static bool radioOk = true;    // CC1101 answered the last health check

// ---- low level SPI access ----
static inline void csLow()  { digitalWrite(PIN_CS, LOW); delayMicroseconds(2); }
static inline void csHigh() { digitalWrite(PIN_CS, HIGH); }
static void strobe(uint8_t cmd) { csLow(); spi.transfer(cmd); csHigh(); }
static void writeReg(uint8_t addr, uint8_t val) { csLow(); spi.transfer(addr); spi.transfer(val); csHigh(); }
static uint8_t readStatus(uint8_t addr) { csLow(); spi.transfer(addr | 0xC0); uint8_t v = spi.transfer(0); csHigh(); return v; }

// RXBYTES must be read until two consecutive reads agree (CC1101 errata).
static uint8_t rxBytes() {
  uint8_t a = readStatus(0x3B), b = readStatus(0x3B);
  while (a != b) { a = b; b = readStatus(0x3B); }
  return a;
}

// Register values: 868.95 MHz, 100 kcps data rate, 2-FSK, ~47 kHz deviation, 325 kHz RX
// bandwidth, infinite packet length, no CRC / whitening / address filtering (raw stream).
static const uint8_t cc1101Regs[][2] = {
  {0x00,0x2E},{0x02,0x06},{0x03,0x00},{0x06,0xFF},{0x07,0x00},{0x08,0x02},   // IOCFG2/0, FIFOTHR, PKTLEN, PKTCTRL1/0
  {0x0B,0x08},{0x0D,0x21},{0x0E,0x6B},{0x0F,0xD0},                           // FSCTRL1, FREQ2..0 (868.95 MHz)
  {0x10,0x5B},{0x11,0xF8},{0x12,0x02},{0x13,0x22},{0x14,0xF8},{0x15,0x44},   // MDMCFG4..0, DEVIATN
  {0x17,0x00},{0x18,0x18},{0x19,0x1D},{0x1A,0x1C},                           // MCSM1/0, FOCCFG, BSCFG
  {0x1B,0xC7},{0x1C,0x00},{0x1D,0xB2},{0x21,0xB6},{0x22,0x10},               // AGCCTRL2..0, FREND1/0
  {0x23,0xEA},{0x24,0x2A},{0x25,0x00},{0x26,0x1F},{0x2C,0x81},{0x2D,0x35},{0x2E,0x09}};  // FSCAL3..0, TEST2..0

// Two 16-bit sync words are tried alternately (the wM-Bus sync pattern in both bit orders).
static const uint8_t syncWords[2][2] = {{0x54,0x3D},{0x3D,0x54}};

// Reset the chip and write the register set. Returns true if the chip identifies itself.
static bool initRadio() {
  digitalWrite(PIN_CS, LOW); delay(1); digitalWrite(PIN_CS, HIGH); delay(1);   // power-on reset sequence
  strobe(0x30); delay(10);                                                     // SRES
  for (auto& r : cc1101Regs) writeReg(r[0], r[1]);
  uint8_t version = readStatus(0x31);
  return version == 0x14 || version == 0x04;
}

static void radioBegin() {
  pinMode(PIN_CS, OUTPUT); digitalWrite(PIN_CS, HIGH);
  pinMode(PIN_GDO0, INPUT); pinMode(PIN_GDO2, INPUT);
  spi.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  spi.setFrequency(2000000); spi.setDataMode(SPI_MODE0);
  radioOk = initRadio();
  Serial.printf("CC1101: %s\n", radioOk ? "ok" : "NOT RESPONDING - check wiring");
}

// Flush FIFO, arm the given sync word and enter RX.
static void startRx(int syncIdx) {
  curSync = syncIdx;
  strobe(0x36); strobe(0x3A);                                   // SIDLE, SFRX
  writeReg(0x04, syncWords[syncIdx][0]); writeReg(0x05, syncWords[syncIdx][1]);
  strobe(0x34);                                                 // SRX
}

// Periodic health check: re-initialise the chip if it stopped answering.
static void radioHealthCheck() {
  uint8_t v = readStatus(0x31);
  radioOk = (v == 0x14 || v == 0x04);
  if (!radioOk) {
    st.radioResets++; Serial.println("CC1101 not responding - re-initialising");
    radioOk = initRadio(); startRx(curSync);
  }
}

// Non-blocking poll. When a burst of bytes arrived (sync word detected) it reads the whole
// burst, restarts RX and returns true. buf/len/rssi (dBm) describe the raw received bytes.
static bool radioPoll(uint8_t* buf, int bufSize, int& len, int& rssi) {
  if (!(rxBytes() & 0x7F)) return false;
  len = 0; uint32_t lastData = millis();
  int8_t rssiRaw = (int8_t)readStatus(0x34);
  while (millis() - lastData < 4 && len < bufSize) {           // burst ends after 4 ms without new data
    uint8_t rb = rxBytes();
    if (rb & 0x80) break;                                      // RX FIFO overflow
    if (rb > 1) {                                              // keep one byte in the FIFO (CC1101 errata)
      csLow(); spi.transfer(0xFF);                             // burst read of the RX FIFO
      for (int i = 0; i < rb - 1 && len < bufSize; i++) buf[len++] = spi.transfer(0);
      csHigh(); lastData = millis();
    }
  }
  rssi = rssiRaw / 2 - 74;                                     // CC1101 datasheet, 868 MHz, 100 kbps
  startRx(curSync);
  return len > 0;
}
