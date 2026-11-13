// =============================================================================
//  config.h - build-time constants, persistent settings (NVS) and input validation
// =============================================================================
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <LittleFS.h>

#define FW_VERSION "2.2.1"

// ---- CC1101 wiring (ESP32 VSPI). Change here if your board is wired differently. ----
#define PIN_SCK   18
#define PIN_MISO  19
#define PIN_MOSI  23
#define PIN_CS     5
#define PIN_GDO0  25   // sync / packet signal (not used by this firmware, kept for reference)
#define PIN_GDO2  32   // (not used by this firmware, kept for reference)

// Optional hardware factory reset: connect this pin to GND for 10 s while the device
// is running. GPIO0 is the BOOT pin of most ESP32 boards.
#define PIN_FACTORY_RESET 0
#define FACTORY_RESET_HOLD_MS 10000

// Initial password of a new / factory-reset device (WiFi access point and web login "admin").
// It is public, therefore the web UI forces the user to choose a new one before anything can be changed.
#define INITIAL_PASSWORD "wmbus1234"

#define NUM_SLOTS 2          // number of meters the gateway can track
#define KIND_WATER 0
#define KIND_HEAT  1

// ----------------------------------------------------------------------------
//  Settings
// ----------------------------------------------------------------------------
struct SlotConfig {
  String  id;                // 8-digit serial number as printed on the meter; empty = slot disabled
  String  name;              // display name (UI + Home Assistant)
  uint8_t kind = KIND_WATER; // KIND_WATER or KIND_HEAT
};

struct Config {
  String   apSsid = "wmBus-Gateway", apPass = INITIAL_PASSWORD, webPass = INITIAL_PASSWORD;   // access point + web login ("admin")
  String   staSsid, staPass;                            // home WiFi (optional)
  String   mqttHost, mqttUser, mqttPass, mqttBase = "wmbus";
  uint16_t mqttPort = 1883;
  SlotConfig slot[NUM_SLOTS];
};

static Config cfg;
static Preferences prefs;

// NVS key names. The slot keys keep their historic names ("iw"/"ih" ...) so that
// settings of firmware 2.0/2.1 survive an upgrade.
static const char* const KEY_ID[NUM_SLOTS]   = {"iw", "ih"};
static const char* const KEY_NAME[NUM_SLOTS] = {"nw", "nh"};
static const char* const KEY_KIND[NUM_SLOTS] = {"k0", "k1"};
static const char* const KEY_RM[NUM_SLOTS]   = {"rm0", "rm1"};   // meter IDs whose HA entities must be removed

// True while the well-known initial password is still active: the UI then forces a password change.
static bool mustChangePassword() { return cfg.webPass == INITIAL_PASSWORD; }

static String defaultName(uint8_t kind) { return kind == KIND_HEAT ? "Wärmezähler" : "Wasserzähler"; }

// ----------------------------------------------------------------------------
//  Input validation (all values coming from the web API are checked here)
// ----------------------------------------------------------------------------
static bool hasControlChars(const String& s) {
  for (size_t i = 0; i < s.length(); i++) { uint8_t c = s[i]; if (c < 0x20 || c == 0x7F) return true; }
  return false;
}
static bool validId(const String& s) {                     // exactly 8 decimal digits
  if (s.length() != 8) return false;
  for (size_t i = 0; i < 8; i++) if (s[i] < '0' || s[i] > '9') return false;
  return true;
}
static bool validName(const String& s)  { return s.length() >= 1 && s.length() <= 32 && !hasControlChars(s); }
static bool validSsid(const String& s)  { return s.length() >= 1 && s.length() <= 32 && !hasControlChars(s); }
static bool validWpaPass(const String& s) { return s.length() >= 8 && s.length() <= 63 && !hasControlChars(s); }
static bool validHost(const String& s) {                    // hostname or IPv4/IPv6 literal
  if (s.length() < 1 || s.length() > 64) return false;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (!(isalnum((unsigned char)c) || c == '.' || c == '-' || c == '_' || c == ':')) return false;
  }
  return true;
}
static bool validTopic(const String& s) {                   // a/b/c, no leading/trailing slash, no wildcards
  if (s.length() < 1 || s.length() > 48 || s[0] == '/' || s[s.length() - 1] == '/') return false;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (!(isalnum((unsigned char)c) || c == '_' || c == '-' || c == '/')) return false;
    if (c == '/' && i > 0 && s[i - 1] == '/') return false;
  }
  return true;
}

// ----------------------------------------------------------------------------
//  Persistence
// ----------------------------------------------------------------------------
static void loadConfig() {
  prefs.begin("wmbus", false);
  cfg.apSsid   = prefs.getString("as", cfg.apSsid);
  cfg.staSsid  = prefs.getString("ss", "");   cfg.staPass  = prefs.getString("sp", "");
  cfg.mqttHost = prefs.getString("mh", "");   cfg.mqttUser = prefs.getString("mu", "");
  cfg.mqttPass = prefs.getString("mw", "");   cfg.mqttBase = prefs.getString("mb", cfg.mqttBase);
  cfg.mqttPort = prefs.getUShort("mp", 1883);
  for (int i = 0; i < NUM_SLOTS; i++) {
    cfg.slot[i].kind = prefs.getUChar(KEY_KIND[i], i == 0 ? KIND_WATER : KIND_HEAT);
    cfg.slot[i].id   = prefs.getString(KEY_ID[i], "");
    cfg.slot[i].name = prefs.getString(KEY_NAME[i], defaultName(cfg.slot[i].kind));
  }
  cfg.webPass = prefs.getString("wp", INITIAL_PASSWORD);
  cfg.apPass  = prefs.getString("ap", INITIAL_PASSWORD);
  prefs.end();
}

static void saveConfig() {
  prefs.begin("wmbus", false);
  prefs.putString("as", cfg.apSsid);   prefs.putString("ap", cfg.apPass);   prefs.putString("wp", cfg.webPass);
  prefs.putString("ss", cfg.staSsid);  prefs.putString("sp", cfg.staPass);
  prefs.putString("mh", cfg.mqttHost); prefs.putString("mu", cfg.mqttUser);
  prefs.putString("mw", cfg.mqttPass); prefs.putString("mb", cfg.mqttBase);
  prefs.putUShort("mp", cfg.mqttPort);
  for (int i = 0; i < NUM_SLOTS; i++) {
    prefs.putUChar(KEY_KIND[i], cfg.slot[i].kind);
    prefs.putString(KEY_ID[i], cfg.slot[i].id);
    prefs.putString(KEY_NAME[i], cfg.slot[i].name);
  }
  prefs.end();
}

// Remember an old meter ID so its Home Assistant entities are removed on the next MQTT connect.
static void queueDiscoveryCleanup(int slot, const String& oldId) {
  prefs.begin("wmbus", false); prefs.putString(KEY_RM[slot], oldId); prefs.end();
}

// Delete stored history of a slot (used when a slot gets another meter).
static void deleteSlotHistory(int slot) {
  LittleFS.remove("/h" + String(slot) + ".bin");
  LittleFS.remove("/l" + String(slot) + ".bin");
}

static void factoryResetStorage() {
  prefs.begin("wmbus", false); prefs.clear(); prefs.end();
  for (int i = 0; i < NUM_SLOTS; i++) deleteSlotHistory(i);
}
