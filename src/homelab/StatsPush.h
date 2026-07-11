#pragma once

// Homelab reading-stats push (Dustpine integration).
//
// Optional, compile-time feature enabled with -DHOMELAB_STATS (see
// platformio.local.ini, which is git-ignored). When enabled, the current
// reading stats are POSTed as HMAC-signed JSON to a private collector on the
// LAN whenever WiFi is *already* connected. It never wakes the radio, never
// blocks beyond a short timeout, and ignores all failures.

#ifdef HOMELAB_STATS

#include <cstdint>
#include <string>
#include <vector>

struct GlobalReadingStats;

namespace homelab {

// One recent book's cumulative stats (plain data; caller loads from SD under a
// RenderLock and passes copies so the POST can happen lock-free).
struct BookStat {
  std::string title;
  std::string author;
  std::string startDate;     // "YYYY-MM-DD" or "" if unknown
  std::string finishedDate;  // "YYYY-MM-DD" or "" if unfinished/unknown
  uint32_t seconds = 0;
  uint32_t pages = 0;
  uint32_t estLeftSeconds = 0;
  uint16_t sessions = 0;
  uint16_t avgSecPerPage = 0;
  bool completed = false;
};

// Rich push: global counters + current streak + per-book bookshelf.
void pushGlobalStats(const GlobalReadingStats& stats, uint16_t currentStreak,
                     const std::vector<BookStat>& books);

// Minimal push (save()-hook fallback): global counters only.
void pushGlobalStats(const GlobalReadingStats& stats);

}  // namespace homelab

#endif  // HOMELAB_STATS
