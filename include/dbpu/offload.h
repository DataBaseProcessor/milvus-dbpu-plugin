#pragma once
#include <string>

namespace dbpu {

struct OffloadConfig {
  bool enabled = true;
  double scan_codes_threshold = 70.0;
  int min_batch_size = 10;

  static OffloadConfig load_default() { return OffloadConfig{}; }
};

class OffloadDecider {
public:
  explicit OffloadDecider(const OffloadConfig& c) : c_(c) {}

  bool should_offload(const std::string& index_type, int nq, double scan_pct) {
    if (!c_.enabled) { reason_ = "disabled"; return false; }
    if (nq < c_.min_batch_size) { reason_ = "nq too small"; return false; }
    if (scan_pct < c_.scan_codes_threshold) { reason_ = "scan_pct below threshold"; return false; }
    if (index_type == "HNSW") { reason_ = "HNSW not supported"; return false; }
    reason_ = "ok";
    return true;
  }

  std::string reason() const { return reason_; }

private:
  OffloadConfig c_;
  std::string reason_;
};

} // namespace dbpu