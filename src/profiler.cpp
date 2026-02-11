#include <dbpu/profiler.h>
#include <sstream>

namespace dbpu {

void Profiler::start() {
  t0_ = std::chrono::high_resolution_clock::now();
}

void Profiler::set_basic(const std::string& index_type, int nq, int dim, int topk) {
  index_type_ = index_type;
  nq_ = nq; dim_ = dim; topk_ = topk;
}

void Profiler::set_decision(bool offload, const std::string& reason) {
  offload_ = offload;
  reason_ = reason;
}

std::string Profiler::stop() {
  auto t1 = std::chrono::high_resolution_clock::now();
  auto us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0_).count();

  std::ostringstream oss;
  oss << "{"
      << "\"ts_us\":" << us << ","
      << "\"index_type\":\"" << index_type_ << "\","
      << "\"nq\":" << nq_ << ","
      << "\"dim\":" << dim_ << ","
      << "\"topk\":" << topk_ << ","
      << "\"decision\":\"" << (offload_ ? "dbpu" : "cpu") << "\","
      << "\"reason\":\"" << reason_ << "\""
      << "}";
  return oss.str();
}

} // namespace dbpu