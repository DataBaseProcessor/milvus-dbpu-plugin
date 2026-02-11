# 🔌 milvus-dbpu-plugin

> **Milvus Search Interceptor**: Profiles FAISS operations and decides when to offload to DBPU hardware

## 🎯 Purpose

This plugin sits **inside Milvus** to intercept vector search operations, profile their performance, and intelligently decide whether to offload computation to DBPU hardware or use CPU fallback.

**Part of DataStream's 3-layer DBPU stack:**
- **vdb-accel-lab** (workload generation & analysis)
- **milvus-dbpu-plugin** ← You are here (intelligence layer inside Milvus)
- **dbpu-runtime** (hardware abstraction API)

---

## 🏗️ Architecture
```
External World (vdb-accel-lab, clients)
              │
              ▼ PyMilvus API
┌─────────────────────────────────────────┐
│          Milvus Vector Database         │
│                                         │
│  ┌───────────────────────────────────┐ │
│  │  milvus-dbpu-plugin (This Repo)   │ │
│  │  ┌──────────────────────────────┐ │ │
│  │  │ 1. Intercept Search Request  │ │ │
│  │  └──────────┬───────────────────┘ │ │
│  │             │                      │ │
│  │             ▼                      │ │
│  │  ┌──────────────────────────────┐ │ │
│  │  │ 2. Profile FAISS Operation   │ │ │
│  │  │    • Measure scan_codes time │ │ │
│  │  │    • Calculate bottleneck %  │ │ │
│  │  └──────────┬───────────────────┘ │ │
│  │             │                      │ │
│  │             ▼                      │ │
│  │  ┌──────────────────────────────┐ │ │
│  │  │ 3. Decide: DBPU or CPU?      │ │ │
│  │  │    if (scan_codes > 70%)     │ │ │
│  │  └───┬──────────────────────┬───┘ │ │
│  │      │                      │     │ │
│  │      ▼ YES                  ▼ NO  │ │
│  │  ┌────────┐          ┌──────────┐│ │
│  │  │ DBPU   │          │  FAISS   ││ │
│  │  │ Offload│          │  (CPU)   ││ │
│  │  └───┬────┘          └────┬─────┘│ │
│  │      │                    │      │ │
│  │      ▼                    ▼      │ │
│  │  ┌──────────────────────────────┐ │ │
│  │  │ 4. Log to /tmp/dbpu-*.jsonl  │ │ │
│  │  └──────────────────────────────┘ │ │
│  └───────────────────────────────────┘ │
└─────────────────────────────────────────┘
         │                    │
         ▼                    ▼
    dbpu-runtime           FAISS
    (Hardware API)       (CPU fallback)
```

---

## 🔬 What It Captures

### C++ Profiling Data Structure
```cpp
struct DBPUProfileData {
    std::string timestamp;        // ISO 8601 format
    std::string operation;        // "search", "insert", etc.
    std::string index_type;       // "IVF_FLAT", "HNSW", "FLAT"
    json index_params;            // {"nlist": 128, ...}
    json search_params;           // {"nprobe": 10, ...}
    
    // Timing breakdown
    uint64_t total_time_us;       // Total search time
    uint64_t scan_codes_time_us;  // Time in FAISS scan_codes
    uint64_t other_time_us;       // Preprocessing, postprocessing
    double scan_codes_percentage; // Bottleneck ratio
    
    // Query metadata
    int nq;                       // Number of queries
    int dim;                      // Vector dimension
    int top_k;                    // Results per query
    
    // Offload decision
    bool used_dbpu;               // Was DBPU used?
    std::string offload_reason;   // Why DBPU was/wasn't used
};
```

### Output Format (JSONL)
```json
{
  "timestamp": "2026-02-11T10:01:00.000Z",
  "operation": "search",
  "index_type": "IVF_FLAT",
  "index_params": {"nlist": 128},
  "search_params": {"nprobe": 10},
  "total_time_us": 120000,
  "scan_codes_time_us": 95000,
  "other_time_us": 25000,
  "scan_codes_percentage": 79.2,
  "nq": 10,
  "dim": 128,
  "top_k": 10,
  "used_dbpu": true,
  "offload_reason": "scan_codes > 70% threshold"
}
```

---

## 🚀 Quick Start

### Prerequisites
- Milvus 2.4+ source code
- C++ compiler (gcc 9+ or clang 10+)
- CMake 3.18+
- dbpu-runtime library (if hardware offload enabled)

### Build & Install
```bash
# Clone this repository
git clone https://github.com/DatabaseProcessor/milvus-dbpu-plugin.git
cd milvus-dbpu-plugin

# Clone Milvus
git clone https://github.com/milvus-io/milvus.git
cd milvus

# Apply plugin patches
../scripts/apply_patches.sh

# Build with plugin
mkdir build && cd build
cmake .. \
  -DENABLE_DBPU_PROFILING=ON \
  -DDBPU_RUNTIME_PATH=/path/to/dbpu-runtime \
  -DCMAKE_BUILD_TYPE=Release

make -j$(nproc)
```

### Configuration
```yaml
# milvus.yaml
dbpu:
  profiling:
    enabled: true
    log_file: /tmp/dbpu-knowhere.jsonl
    log_level: INFO
    
  offload:
    enabled: true
    scan_codes_threshold: 70.0  # Use DBPU if > 70%
    min_batch_size: 10          # Offload only for nq >= 10
    
  runtime:
    library_path: /usr/local/lib/libdbpu.so
    device_id: 0
```

### Docker Deployment
```bash
# Build custom Milvus image with plugin
docker build -t milvus-dbpu:latest .

# Run with plugin enabled
docker run -d \
  --name milvus-dbpu \
  -p 19530:19530 \
  -p 9091:9091 \
  -v $(pwd)/milvus.yaml:/milvus/configs/milvus.yaml \
  -v /tmp:/tmp \
  --device /dev/dbpu0 \
  milvus-dbpu:latest standalone

# Check logs
docker logs -f milvus-dbpu
tail -f /tmp/dbpu-knowhere.jsonl | jq '.'
```

---

## 🔗 Integration Points

### With vdb-accel-lab
```bash
# vdb-accel-lab automatically uses the plugin
cd vdb-accel-lab
python workloads/lab_gen.py  # Sends queries via PyMilvus

# Plugin intercepts and logs to /tmp/dbpu-knowhere.jsonl

# Analyze profiling data
python analyzer/analyze_hooks.py
```

### With dbpu-runtime
```cpp
// Inside plugin code
#include <dbpu/runtime.h>

if (should_offload(profile_data)) {
    // Call dbpu-runtime API
    auto results = dbpu::search(
        vectors, n_vectors,
        queries, n_queries,
        dim, top_k,
        dbpu::Metric::L2
    );
    
    profile_data.used_dbpu = true;
    profile_data.offload_reason = "scan_codes threshold exceeded";
} else {
    // Fallback to FAISS
    auto results = faiss_search(...);
    
    profile_data.used_dbpu = false;
    profile_data.offload_reason = "below threshold";
}
```

---

## 🎯 Offload Decision Logic
```cpp
bool should_offload(const SearchRequest& req, const ProfileData& profile) {
    // Rule 1: Check if DBPU is available
    if (!dbpu::is_available()) return false;
    
    // Rule 2: Check bottleneck percentage
    if (profile.scan_codes_percentage < config.threshold) return false;
    
    // Rule 3: Check batch size (avoid PCIe overhead for small batches)
    if (req.nq < config.min_batch_size) return false;
    
    // Rule 4: Check index type (HNSW doesn't benefit much)
    if (req.index_type == "HNSW") return false;
    
    return true;  // All checks passed, use DBPU!
}
```

---

## 🎯 Development Roadmap

### Phase 1: Basic Profiling ✅
- [x] Hook placement design
- [x] Patch generation scripts
- [x] JSONL logging format
- [ ] Integration with Milvus 2.4
- [ ] Unit tests for hook logic

### Phase 2: Offload Logic 🚧
- [ ] Implement offload decision engine
- [ ] Integrate dbpu-runtime API
- [ ] Fallback handling
- [ ] Performance comparison (DBPU vs CPU)

### Phase 3: Advanced Features
- [ ] Adaptive thresholds (learned from history)
- [ ] Query routing optimization
- [ ] Multi-device support
- [ ] Power-aware scheduling

### Phase 4: Production
- [ ] Zero-overhead compilation mode
- [ ] Dynamic sampling (profile 1% of queries)
- [ ] Distributed tracing integration
- [ ] Prometheus metrics export

---

## 📊 Metrics & Monitoring
```bash
# Real-time profiling log
tail -f /tmp/dbpu-knowhere.jsonl | jq '.scan_codes_percentage'

# DBPU usage rate
grep '"used_dbpu":true' /tmp/dbpu-knowhere.jsonl | wc -l

# Average speedup
jq -s 'map(select(.used_dbpu == true)) | 
       map(.total_time_us) | 
       add / length' /tmp/dbpu-knowhere.jsonl
```

---

## 🤝 Contributing

Internal contributors:
1. Test patches against Milvus main branch
2. Ensure zero overhead when profiling disabled
3. Document all hook locations
4. Add integration tests

---

