#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace meter_reliability {

struct DailyState {
  uint32_t version{0};
  int32_t day{0};
  float baseline{0};
  int32_t yesterday_day{0};
  float yesterday{0};
  uint32_t complete{0};
  uint32_t yesterday_valid{0};
};

static_assert(sizeof(DailyState) <= 32, "Keep persistent state small");

class DailyLedger {
 public:
  static constexpr uint32_t VERSION = 0x4d4c0201;
  static constexpr uint32_t BOUNDARY_WINDOW = 10;
  static constexpr uint32_t MAX_AGE = 10;

  static int32_t local_day(uint32_t utc) {
    return static_cast<int32_t>((static_cast<uint64_t>(utc) + 28800) / 86400);
  }

  void unavailable() { ready_ = false; }

  void observe(DailyState &state, uint32_t utc, float total) {
    ready_ = false;
    if (!std::isfinite(total) || total < 0 || utc < 1704067200U) return;
    const int32_t day = local_day(utc);
    if (state.version != VERSION || !std::isfinite(state.baseline) || state.baseline < 0) {
      state = {VERSION, day, total, 0, 0, 0, 0};
    } else if ((last_utc_ != 0 && utc < last_utc_ && last_utc_ - utc > 2) || day < state.day ||
               (last_utc_ != 0 && total < last_total_) || total < state.baseline) {
      // A clock/counter discontinuity cannot be allocated to a truthful day.
      state = {VERSION, day, total, 0, 0, 0, 0};
    } else if (day != state.day) {
      const uint32_t midnight = static_cast<uint32_t>(static_cast<int64_t>(day) * 86400 - 28800);
      const bool observed_boundary = day == state.day + 1 && last_utc_ != 0 &&
          last_utc_ <= midnight && midnight - last_utc_ <= BOUNDARY_WINDOW &&
          utc >= midnight && utc - midnight <= BOUNDARY_WINDOW;
      const bool previous_complete = state.complete == 1 && observed_boundary;
      const float previous_usage = previous_complete ? total - state.baseline : 0;
      state = {VERSION, day, total, day - 1, previous_usage,
               observed_boundary ? 1U : 0U, previous_complete ? 1U : 0U};
    }
    last_utc_ = utc;
    last_total_ = total;
    ready_ = true;
  }

  float today(const DailyState &state, uint32_t utc) const {
    if (!fresh(utc) || state.version != VERSION || state.complete != 1 ||
        state.day != local_day(utc) || last_total_ < state.baseline) return missing();
    return last_total_ - state.baseline;
  }

  float yesterday(const DailyState &state, uint32_t utc) const {
    if (utc < 1704067200U || state.version != VERSION || state.yesterday_valid != 1 ||
        state.yesterday_day != local_day(utc) - 1 || !std::isfinite(state.yesterday) ||
        state.yesterday < 0) return missing();
    return state.yesterday;
  }

 private:
  static float missing() { return std::numeric_limits<float>::quiet_NaN(); }
  bool fresh(uint32_t utc) const {
    return ready_ && utc >= last_utc_ && utc - last_utc_ <= MAX_AGE;
  }
  bool ready_{false};
  uint32_t last_utc_{0};
  float last_total_{0};
};

}  // namespace meter_reliability
