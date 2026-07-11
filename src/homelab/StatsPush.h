#pragma once

// Homelab reading-stats push (Dustpine integration).
//
// Optional, compile-time feature enabled with -DHOMELAB_STATS (see
// platformio.local.ini, which is git-ignored). When enabled, the current
// cumulative reading stats are POSTed as HMAC-signed JSON to a private
// collector on the LAN whenever WiFi is *already* connected. It never wakes
// the radio, never blocks beyond a short timeout, and ignores all failures.
// No secrets are baked in beyond a low-value shared HMAC key.

#ifdef HOMELAB_STATS

struct GlobalReadingStats;

namespace homelab {
// Best-effort, fire-and-forget. Safe to call from GlobalReadingStats::save();
// returns immediately if WiFi is not connected.
void pushGlobalStats(const GlobalReadingStats& stats);
}  // namespace homelab

#endif  // HOMELAB_STATS
