#include "../daily_ledger.h"

#include <cassert>
#include <cstring>
#include <iostream>

using meter_reliability::DailyLedger;
using meter_reliability::DailyState;

int main() {
  constexpr uint32_t m = 20000U * 86400U - 28800U;
  DailyState state{};
  DailyLedger ledger;
  unsigned checks = 0;
  auto check = [&](bool value) { assert(value); ++checks; };
  ledger.observe(state, 0, 1000);
  check(state.version == 0);
  ledger.observe(state, m - 5, 1000);
  check(std::isnan(ledger.today(state, m - 5)));
  check(std::isnan(ledger.yesterday(state, m - 5)));
  ledger.observe(state, m + 1, 1005);
  check(ledger.today(state, m + 1) == 0);
  check(std::isnan(ledger.yesterday(state, m + 1)));
  ledger.observe(state, m + 61, 1006);
  check(ledger.today(state, m + 61) == 1);
  const DailyState persisted = state;
  ledger.observe(state, m + 62, 1007);
  check(std::memcmp(&persisted, &state, sizeof(state)) == 0);
  check(std::isnan(ledger.today(state, m + 73)));
  check(std::isnan(ledger.today(state, m + 60)));
  DailyLedger restarted;
  restarted.observe(state, m + 63, 1007);
  check(restarted.today(state, m + 63) == 2);
  restarted.observe(state, m + 62, 1007);
  check(restarted.today(state, m + 62) == 2);
  restarted.observe(state, m + 86400 - 1, 1029);
  restarted.observe(state, m + 86400 + 1, 1030);
  check(restarted.yesterday(state, m + 86400 + 1) == 25);
  check(restarted.today(state, m + 86400 + 1) == 0);
  const auto valid = state;
  restarted.observe(state, m + 86400 + 2, NAN);
  check(std::memcmp(&valid, &state, sizeof(state)) == 0);
  check(std::isnan(restarted.today(state, m + 86400 + 2)));
  restarted.observe(state, m + 86400 + 3, 1031);
  check(restarted.today(state, m + 86400 + 3) == 1);
  restarted.observe(state, m + 86400 + 4, 2);
  check(std::isnan(restarted.today(state, m + 86400 + 4)));
  check(std::isnan(restarted.yesterday(state, m + 86400 + 4)));
  restarted.observe(state, m + 86400 - 100, 3);
  check(std::isnan(restarted.today(state, m + 86400 - 100)));
  restarted.observe(state, m + 3 * 86400 + 1, 4);
  check(std::isnan(restarted.today(state, m + 3 * 86400 + 1)));
  check(std::isnan(restarted.yesterday(state, m + 3 * 86400 + 1)));
  DailyState stale{DailyLedger::VERSION, 20000, 100, 19999, 12, 1, 1};
  DailyLedger after_outage;
  after_outage.observe(stale, m + 86400 + 300, 125);
  check(std::isnan(after_outage.today(stale, m + 86400 + 300)));
  check(std::isnan(after_outage.yesterday(stale, m + 86400 + 300)));
  check(DailyLedger::local_day(m) == 20000);
  check(DailyLedger::local_day(m - 1) == 19999);
  state.baseline = INFINITY;
  restarted.observe(state, m + 4 * 86400, 200);
  check(std::isfinite(state.baseline));
  check(std::isnan(restarted.today(state, m + 4 * 86400)));
  restarted.unavailable();
  check(std::isnan(restarted.today(state, m + 4 * 86400)));
  std::cout << checks << " ledger boundary checks passed\n";
}
