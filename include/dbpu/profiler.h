#pragma once
#include <string>
#include <chrono>

namespace dbpu {

class Profiler {
public:
  void start();
  void set_basic(const std::string& index_type, int nq, int dim, int topk);
  void set_decision(bool offload, const std::string& reason);
  std::string stop();

private:
  std::chrono::high_resolution_clock::time_point t0_;
  std::string index_type_;
  int nq_ = 0, dim_ = 0, topk_ = 0;
  bool offload_ = false;
  std::string reason_;
};

} // namespace dbpu