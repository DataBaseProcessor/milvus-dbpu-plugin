#pragma once
#include <cstdint>
#include <vector>
#include "interceptor.h"

// ---- Minimal ABI declarations (no runtime.h dependency) ----
extern "C" {
  typedef struct dbpu_device* dbpu_device_t;

  typedef enum {
    DBPU_SUCCESS = 0,
    DBPU_ERROR_DEVICE_NOT_FOUND = -1,
    DBPU_ERROR_OUT_OF_MEMORY = -2,
    DBPU_ERROR_INVALID_ARGUMENT = -3,
    DBPU_ERROR_TIMEOUT = -4,
    DBPU_ERROR_NOT_IMPLEMENTED = -5
  } dbpu_status_t;

  typedef enum {
    DBPU_METRIC_L2 = 0,
    DBPU_METRIC_IP,
    DBPU_METRIC_COSINE
  } dbpu_metric_t;

  typedef struct {
    uint64_t compute_time_us;
    uint64_t memcpy_h2d_time_us;
    uint64_t memcpy_d2h_time_us;
    uint64_t total_time_us;
    double throughput_gflops;
  } dbpu_perf_t;

  typedef enum {
    DBPU_DEVICE_CPU = 0,
    DBPU_DEVICE_SIMULATOR,
    DBPU_DEVICE_FPGA,
    DBPU_DEVICE_ASIC
  } dbpu_device_type_t;
}

namespace dbpu {

class RuntimeLoader {
public:
  static RuntimeLoader& instance();
  bool is_available() const;

  SearchResult search(
    const float* database, int n_vectors,
    const float* queries, int n_queries,
    int dim, int top_k
  );

  bool get_perf(dbpu_perf_t* perf);

private:
  RuntimeLoader();
  ~RuntimeLoader();
  RuntimeLoader(const RuntimeLoader&) = delete;
  RuntimeLoader& operator=(const RuntimeLoader&) = delete;
};

} // namespace dbpu