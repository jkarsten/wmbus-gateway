// =============================================================================
//  pinprobe - finds the ESP32 pins a CC1101 module is wired to
//
//  Tries many SPI pin combinations with bit-banged SPI and reads the CC1101 VERSION register
//  (status register 0x31, expected 0x14 or 0x04). Prints every combination that answers.
//  Run it with the CC1101 wired and open the serial monitor at 115200 baud (takes ~20 s).
//
//  Note: while probing, pins are driven as outputs; this is harmless for a CC1101 module but
//  do not run it with other hardware connected to the probed pins.
// =============================================================================
const int outPins[] = {2,4,5,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33};          // CS, SCK, MOSI candidates
const int inPins[]  = {2,4,5,13,14,15,16,17,18,19,21,22,23,25,26,27,32,33,34,35,36,39}; // MISO candidates (incl. input-only pins)
#define N_OUT (sizeof(outPins) / sizeof(int))
#define N_IN  (sizeof(inPins) / sizeof(int))

// Transfer one byte (mode 0, MSB first) and return the byte read back.
uint8_t xfer(int mosi, int miso, int sck, uint8_t b) {
  uint8_t r = 0;
  for (int i = 7; i >= 0; i--) {
    digitalWrite(mosi, (b >> i) & 1);
    digitalWrite(sck, HIGH); delayMicroseconds(1);
    r = (r << 1) | digitalRead(miso);
    digitalWrite(sck, LOW); delayMicroseconds(1);
  }
  return r;
}

// Read the VERSION status register using the given pins.
uint8_t readVersion(int cs, int sck, int mosi, int miso) {
  pinMode(cs, OUTPUT); pinMode(sck, OUTPUT); pinMode(mosi, OUTPUT); pinMode(miso, INPUT);
  digitalWrite(sck, LOW); digitalWrite(cs, HIGH); delayMicroseconds(5);
  digitalWrite(cs, LOW);  delayMicroseconds(5);
  xfer(mosi, miso, sck, 0xF1);                 // read burst of status register 0x31
  uint8_t v = xfer(mosi, miso, sck, 0x00);
  digitalWrite(cs, HIGH);
  return v;
}

void setup() {
  Serial.begin(115200); delay(1500);
  Serial.println("PROBE START");
  int found = 0;
  for (int a = 0; a < N_OUT; a++) for (int b = 0; b < N_OUT; b++) for (int c = 0; c < N_OUT; c++) {
    int cs = outPins[a], sck = outPins[b], mosi = outPins[c];
    if (cs == sck || cs == mosi || sck == mosi) continue;
    for (int d = 0; d < N_IN; d++) {
      int miso = inPins[d];
      if (miso == cs || miso == sck || miso == mosi) continue;
      uint8_t v = readVersion(cs, sck, mosi, miso);
      if ((v == 0x14 || v == 0x04 || v == 0x03) && readVersion(cs, sck, mosi, miso) == v) {   // confirm with a second read
        Serial.printf("FOUND CS=%d SCK=%d MOSI=%d MISO=%d version=0x%02X\n", cs, sck, mosi, miso, v);
        found++;
      }
    }
  }
  Serial.printf("PROBE DONE found=%d\n", found);
}

void loop() { delay(1000); }
