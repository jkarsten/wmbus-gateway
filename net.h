// =============================================================================
//  net.h - WiFi (access point + station), mDNS, captive-portal DNS, NTP and
//          MQTT with Home Assistant auto-discovery
// =============================================================================
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include "config.h"
#include "meters.h"

static DNSServer dns;
static WiFiClient wifiClient;
static PubSubClient mq(wifiClient);
static uint32_t lastWifiTry = 0, lastMqttTry = 0;

static bool mqttConfigured() { return cfg.mqttHost.length() > 0; }
static String chipId() { char b[9]; snprintf(b, sizeof b, "%06X", (unsigned)(ESP.getEfuseMac() >> 24)); return b; }

// Escape a string for use inside a JSON string literal.
static String jsonEscape(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '"' || c == '\\') { o += '\\'; o += c; }
    else if ((uint8_t)c >= 0x20) o += c;
  }
  return o;
}

// ----------------------------------------------------------------------------
//  WiFi
// ----------------------------------------------------------------------------
static void setupWifi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(cfg.apSsid.c_str(), cfg.apPass.c_str(), 1, 0, 4);   // channel 1, visible, max. 4 clients
  if (cfg.staSsid.length()) WiFi.begin(cfg.staSsid.c_str(), cfg.staPass.c_str());
  dns.start(53, "*", WiFi.softAPIP());                           // captive portal: every name resolves to the AP
  Serial.printf("Access point '%s' ready: http://%s\n", cfg.apSsid.c_str(), WiFi.softAPIP().toString().c_str());
  configTime(0, 0, "pool.ntp.org", "time.google.com");           // wall clock for history time stamps
  if (MDNS.begin("wmbus")) MDNS.addService("http", "tcp", 80);   // reachable as http://wmbus.local
}

static void wifiService() {            // reconnect to the home network every 15 s while disconnected
  if (!cfg.staSsid.length() || WiFi.status() == WL_CONNECTED) return;
  if (millis() - lastWifiTry < 15000) return;
  lastWifiTry = millis(); WiFi.disconnect(); WiFi.begin(cfg.staSsid.c_str(), cfg.staPass.c_str());
}

// ----------------------------------------------------------------------------
//  MQTT / Home Assistant discovery
//  Topics:  <base>/status                  online / offline (last will)
//           <base>/<meter id>/state        JSON with all values (retained)
//           homeassistant/sensor/...       discovery configs (retained)
// ----------------------------------------------------------------------------
struct SensorDef { const char* key; const char* name; const char* devClass; const char* unit; const char* stateClass; bool diag; };

static String discoveryTopic(const String& id, const char* key) { return "homeassistant/sensor/wmbus_" + id + "_" + key + "/config"; }

static void publishDiscovery(int slot) {
  Meter& m = M[slot]; bool water = m.isWater();
  String devName = cfg.slot[slot].name, uid = "wmbus_" + String(m.idStr);
  String base = cfg.mqttBase + "/" + m.idStr;
  const char* energyClass = water ? "water" : "energy";
  const char* unit = water ? "m³" : "kWh";
  const SensorDef defs[] = {
    {"total",       "Zählerstand",          energyClass,       unit,  "total_increasing", false},
    {"year_value",  "Wert Jahresstichtag",  energyClass,       unit,  "total",            false},
    {"year_date",   "Datum Jahresstichtag", "date",            "",    "",                 false},
    {"month_value", "Wert Monatsstichtag",  energyClass,       unit,  "total",            false},
    {"month_date",  "Datum Monatsstichtag", "date",            "",    "",                 false},
    {"rssi",        "Signalstärke",         "signal_strength", "dBm", "measurement",      true},
  };
  for (const SensorDef& d : defs) {
    String p = "{\"name\":\"" + jsonEscape(devName) + " " + d.name + "\",\"uniq_id\":\"" + uid + "_" + d.key + "\"";
    p += ",\"stat_t\":\"" + base + "/state\",\"val_tpl\":\"{{ value_json." + d.key + " }}\"";
    p += ",\"avty_t\":\"" + cfg.mqttBase + "/status\",\"dev_cla\":\"" + d.devClass + "\"";
    if (d.unit[0])       p += ",\"unit_of_meas\":\"" + String(d.unit) + "\"";
    if (d.stateClass[0]) p += ",\"stat_cla\":\"" + String(d.stateClass) + "\"";
    if (d.diag)          p += ",\"ent_cat\":\"diagnostic\"";
    p += ",\"dev\":{\"ids\":[\"" + uid + "\"],\"name\":\"" + jsonEscape(devName) + " " + m.idStr + "\",\"mf\":\"Qundis\",\"mdl\":\"" + (water ? "Q water 5.5" : "Q heat 5.5") + "\"}}";
    mq.publish(discoveryTopic(m.idStr, d.key).c_str(), p.c_str(), true);
  }
}

// Remove the entities of meters that were changed or disabled in the settings.
static void publishDiscoveryCleanup() {
  static const char* keys[] = {"total", "year_value", "year_date", "month_value", "month_date", "rssi"};
  prefs.begin("wmbus", false);
  for (int i = 0; i < NUM_SLOTS; i++) {
    String old = prefs.getString(KEY_RM[i], "");
    if (!validId(old)) continue;
    for (const char* k : keys) mq.publish(discoveryTopic(old, k).c_str(), "", true);   // empty retained payload deletes the entity
    mq.publish((cfg.mqttBase + "/" + old + "/state").c_str(), "", true);
    prefs.remove(KEY_RM[i]);
  }
  prefs.end();
}

static void publishState(int slot) {
  Meter& m = M[slot]; bool water = m.isWater();
  String s = "{\"total\":" + fv(m.tot, water);
  if (m.hasPeriods)
    s += ",\"year_value\":" + fv(m.yearVal, water) + ",\"year_date\":\"" + dateIso(m.yearDate) + "\"" +
         ",\"month_value\":" + fv(m.monthVal, water) + ",\"month_date\":\"" + dateIso(m.monthDate) + "\"";
  s += ",\"rssi\":" + String(m.rssi) + "}";
  mq.publish((cfg.mqttBase + "/" + m.idStr + "/state").c_str(), s.c_str(), true);
}

static void mqttService() {
  if (!mqttConfigured() || WiFi.status() != WL_CONNECTED) return;
  if (!mq.connected()) {
    if (lastMqttTry && millis() - lastMqttTry < 10000) return;   // retry every 10 s
    lastMqttTry = millis();
    mq.setServer(cfg.mqttHost.c_str(), cfg.mqttPort); mq.setBufferSize(1024); mq.setSocketTimeout(3);
    String clientId = "wmbus-" + chipId(), will = cfg.mqttBase + "/status";
    bool ok = cfg.mqttUser.length()
      ? mq.connect(clientId.c_str(), cfg.mqttUser.c_str(), cfg.mqttPass.c_str(), will.c_str(), 0, true, "offline")
      : mq.connect(clientId.c_str(), nullptr, nullptr, will.c_str(), 0, true, "offline");
    if (!ok) { Serial.printf("MQTT: connection failed (rc=%d)\n", mq.state()); return; }
    Serial.println("MQTT: connected");
    mq.publish(will.c_str(), "online", true);
    publishDiscoveryCleanup();
    for (int i = 0; i < NUM_SLOTS; i++) if (M[i].enabled) { publishDiscovery(i); M[i].dirty = M[i].have; }
    return;
  }
  mq.loop();
  for (int i = 0; i < NUM_SLOTS; i++) if (M[i].enabled && M[i].dirty && M[i].have) { publishState(i); M[i].dirty = false; }
}
