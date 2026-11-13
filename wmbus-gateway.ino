// =============================================================================
//  wMBus Gateway - ESP32 + CC1101 receiver for wireless M-Bus water / heat meters
//
//  * Receives wM-Bus C1 telegrams (868.95 MHz) of up to two meters (Qundis Q water 5.5,
//    Q heat 5.5 and compatible OMS meters)
//  * Web UI on its own WiFi access point and in the home network (charts, settings, OTA)
//  * MQTT with Home Assistant auto-discovery, history kept in flash
//
//  Modules (header-only, included in this order):
//    config.h        constants, persistent settings, input validation
//    meters.h        meter state, history, telegram log
//    radio_cc1101.h  CC1101 driver
//    decoder.h       frame / telegram decoding
//    net.h           WiFi, NTP, mDNS, MQTT + Home Assistant discovery
//    web.h           web server, API, firmware update
//    webui.h         the single-page web UI (HTML/CSS/JS)
// =============================================================================
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "meters.h"
#include "radio_cc1101.h"
#include "decoder.h"
#include "net.h"
#include "web.h"

static uint32_t lastHealth = 0, resetPressStart = 0;

// Everything that must keep running while the radio is being polled.
static void service() {
  esp_task_wdt_reset();
  dns.processNextRequest();
  web.handleClient();
  mqttService();
  wifiService();

  if (millis() - lastHealth > 30000) { lastHealth = millis(); radioHealthCheck(); }
  if (!histRestored && timeOk()) restoreHistory();      // needs the wall clock (NTP)

  // Optional hardware factory reset: pin held low for 10 s
  if (digitalRead(PIN_FACTORY_RESET) == LOW) {
    if (!resetPressStart) resetPressStart = millis();
    else if (millis() - resetPressStart > FACTORY_RESET_HOLD_MS) {
      Serial.println("Factory reset requested via pin");
      factoryResetStorage(); delay(200); ESP.restart();
    }
  } else resetPressStart = 0;

  if (restartAt && (int32_t)(millis() - restartAt) > 0) ESP.restart();
}

void setup() {
  Serial.begin(115200); delay(300);
  Serial.println("wMBus Gateway " FW_VERSION);
  pinMode(PIN_FACTORY_RESET, INPUT_PULLUP);
  LittleFS.begin(true);                  // format on first use
  loadConfig();
  applyMeterConfig();
  radioBegin();
  setupWifi();
  setupWeb();
  esp_task_wdt_config_t wdt = { .timeout_ms = 30000, .idle_core_mask = 0, .trigger_panic = true };
  if (esp_task_wdt_reconfigure(&wdt) != ESP_OK) esp_task_wdt_init(&wdt);
  esp_task_wdt_add(NULL);                // restart automatically if the main loop hangs for 30 s
  Serial.println(anyMeterEnabled() ? "Receiver ready" : "No meter configured yet - open the web UI to set one up");
}

// The two sync words are armed alternately, 6 s each.
void loop() {
  for (int s = 0; s < 2; s++) {
    startRx(s);
    uint32_t t0 = millis();
    while (millis() - t0 < 6000) {
      service();
      uint8_t buf[300]; int len, rssi;
      if (radioPoll(buf, sizeof buf, len, rssi)) decodeFrame(buf, len, rssi);
      delayMicroseconds(200);
    }
  }
}
