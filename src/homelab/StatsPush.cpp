#include "StatsPush.h"

#ifdef HOMELAB_STATS

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Logging.h>
#include <WiFi.h>
#include <esp_random.h>
#include <mbedtls/md.h>

#include <cstdio>
#include <cstring>

#include "../activities/reader/GlobalReadingStats.h"

#ifndef HOMELAB_STATS_URL
#error "HOMELAB_STATS requires -DHOMELAB_STATS_URL=\"http://host:port/path\" (set in platformio.local.ini)"
#endif
#ifndef HOMELAB_STATS_KEY
#error "HOMELAB_STATS requires -DHOMELAB_STATS_KEY=\"<shared hmac key>\" (set in platformio.local.ini)"
#endif
#ifndef HOMELAB_STATS_TIMEOUT_MS
#define HOMELAB_STATS_TIMEOUT_MS 1500
#endif

namespace {

// HMAC-SHA256(msg, key) as lowercase hex. Lets the collector reject payloads
// from anything on the LAN that doesn't hold the shared key.
String hmacSha256Hex(const String& msg, const char* key) {
  uint8_t out[32] = {0};
  const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (info == nullptr) return String();
  mbedtls_md_hmac(info, reinterpret_cast<const uint8_t*>(key), std::strlen(key),
                  reinterpret_cast<const uint8_t*>(msg.c_str()), msg.length(), out);
  char hex[65];
  for (int i = 0; i < 32; ++i) std::snprintf(hex + i * 2, 3, "%02x", out[i]);
  hex[64] = '\0';
  return String(hex);
}

// Lowercase, colon-free MAC — a stable per-device series key for InfluxDB.
String deviceId() {
  String mac = WiFi.macAddress();  // "AA:BB:CC:DD:EE:FF"
  mac.replace(":", "");
  mac.toLowerCase();
  return mac;
}

}  // namespace

namespace homelab {

void pushGlobalStats(const GlobalReadingStats& stats, const uint16_t currentStreak,
                     const std::vector<BookStat>& books) {
  if (WiFi.status() != WL_CONNECTED) return;  // never wake the radio

  JsonDocument doc;
  doc["device_id"] = deviceId();
  doc["schema"] = GlobalReadingStats::CURRENT_FILE_VERSION;
  doc["nonce"] = String(esp_random(), HEX);
  doc["total_sessions"] = stats.totalSessions;
  doc["total_reading_seconds"] = stats.totalReadingSeconds;
  doc["total_pages_turned"] = stats.totalPagesTurned;
  doc["completed_books"] = stats.completedBooks;
  doc["longest_streak"] = stats.longestReadingStreak;
  doc["current_streak"] = currentStreak;
  {
    JsonArray tod = doc["time_of_day_seconds"].to<JsonArray>();
    for (uint32_t v : stats.timeOfDaySeconds) tod.add(v);
    JsonArray dow = doc["day_of_week_seconds"].to<JsonArray>();
    for (uint32_t v : stats.dayOfWeekSeconds) dow.add(v);
  }
  {
    JsonArray arr = doc["books"].to<JsonArray>();
    for (const BookStat& b : books) {
      JsonObject o = arr.add<JsonObject>();
      o["title"] = b.title;
      o["author"] = b.author;
      o["seconds"] = b.seconds;
      o["pages"] = b.pages;
      o["sessions"] = b.sessions;
      o["completed"] = b.completed;
      o["avg_sec_per_page"] = b.avgSecPerPage;
      o["est_left_seconds"] = b.estLeftSeconds;
      o["start_date"] = b.startDate;
      o["finished_date"] = b.finishedDate;
    }
  }

  String body;
  serializeJson(doc, body);
  const String sig = hmacSha256Hex(body, HOMELAB_STATS_KEY);

  WiFiClient client;
  HTTPClient http;
  http.setConnectTimeout(HOMELAB_STATS_TIMEOUT_MS);
  http.setTimeout(HOMELAB_STATS_TIMEOUT_MS);
  if (!http.begin(client, HOMELAB_STATS_URL)) {
    LOG_DBG("HLSTATS", "begin() failed for %s", HOMELAB_STATS_URL);
    return;
  }
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Signature", sig);
  const int code = http.POST(body);
  LOG_DBG("HLSTATS", "POST %s (%u books) -> %d", HOMELAB_STATS_URL, (unsigned)books.size(), code);
  http.end();
}

void pushGlobalStats(const GlobalReadingStats& stats) { pushGlobalStats(stats, 0, {}); }

}  // namespace homelab

#endif  // HOMELAB_STATS
