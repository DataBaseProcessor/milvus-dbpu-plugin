# 🔌 milvus-dbpu-plugin

> **Milvus Search Interceptor**: Profiles FAISS operations and decides when to offload to DBPU hardware

## 🎯 Purpose

This plugin sits **inside Milvus** (as a submodule) to:
1. Profile FAISS search operations
2. Log scan_codes bottleneck data
3. Decide whether to offload to DBPU hardware
4. Dynamically load dbpu-runtime if available

**Part of DataStream's 4-component stack:**
- vdb-accel-lab (workload generation)
- **milvus-dbpu-plugin** ← You are here (intelligence layer)
- dbpu-runtime (hardware abstraction)
- milvus fork (integration point)

---

## 🏗️ Architecture
```
Milvus Process
│
├─ milvus-dbpu-plugin (This Repo, as submodule)
│  ├─ Intercept search calls
│  ├─ Profile scan_codes timing
│  ├─ Decide: offload or not?
│  └─ RuntimeLoader
│     └─ dlopen("libdbpu-runtime.so")
│        ├─ Found → Use DBPU acceleration
│        └─ Not found → CPU fallback (FAISS)
```

---

## 🔗 Integration Modes

### Mode 1: Profiling Only (No Runtime)
```bash
# Plugin logs profiling data, no acceleration
# Runtime is NOT required for this mode
cmake .. -DENABLE_DBPU_PROFILING=ON
make
```

**Result:**
- ✅ Profiling hooks active
- ✅ Logs to `/tmp/dbpu-knowhere.jsonl`
- ❌ No hardware acceleration (CPU fallback)

### Mode 2: Full Acceleration (With Runtime)
```bash
# Install dbpu-runtime first
cd ~/dbpu-runtime && mkdir build && cd build
cmake .. && make && sudo make install

# Then build Milvus with plugin
cd ~/milvus
cmake .. -DENABLE_DBPU_PROFILING=ON
make
```

**Result:**
- ✅ Profiling hooks active
- ✅ Logs to `/tmp/dbpu-knowhere.jsonl`
- ✅ Hardware acceleration enabled (auto-detected)

---

## 🚀 Quick Start

### As a Submodule (Recommended)
```bash
# In Milvus fork
cd ~/milvus
git submodule add \
    https://github.com/DatabaseProcessor/milvus-dbpu-plugin.git \
    internal/core/thirdparty/dbpu-plugin

git submodule update --init --recursive
```

### CMake Integration
```cmake
# In Milvus's internal/core/CMakeLists.txt
option(ENABLE_DBPU_PROFILING "Enable DBPU profiling" OFF)

if(ENABLE_DBPU_PROFILING)
    add_subdirectory(thirdparty/dbpu-plugin)
    target_link_libraries(milvus_segcore PRIVATE dbpu-plugin)
    add_definitions(-DENABLE_DBPU_PROFILING)
endif()
```

### Knowhere Integration
```cpp
// In Milvus search code
#ifdef ENABLE_DBPU_PROFILING
#include <dbpu/interceptor.h>

SearchResult search(...) {
    dbpu::SearchInterceptor interceptor;
    return interceptor.intercept_search(request, 
        [&](auto& r) { return faiss_search(r); }
    );
}
#endif
```

---

## 🔧 Runtime Detection Logic
```cpp
// Plugin automatically detects runtime at startup
RuntimeLoader::RuntimeLoader() {
    handle_ = dlopen("libdbpu-runtime.so", RTLD_LAZY);
    if (handle_) {
        std::cout << "✅ DBPU runtime detected\n";
        // Load functions and initialize device
    } else {
        std::cout << "⚠️  DBPU runtime not found\n";
        std::cout << "   Profiling-only mode\n";
    }
}
```

**No compilation dependency on dbpu-runtime!**
- Plugin builds successfully without runtime installed
- Runtime is dynamically loaded at program startup
- Graceful degradation to CPU if runtime unavailable

---

## 📊 Output Format
```json
{
  "timestamp": "2026-02-11T10:01:00.000Z",
  "operation": "search",
  "index_type": "IVF_FLAT",
  "total_time_us": 120000,
  "scan_codes_time_us": 95000,
  "scan_codes_percentage": 79.2,
  "used_dbpu": true,
  "offload_reason": "scan_codes > 70% threshold"
}
```

---

## 🎯 Development Roadmap

- [x] Hook placement design
- [x] Dynamic runtime loading
- [x] JSONL logging
- [ ] Integration with Milvus 2.4
- [ ] Offload decision logic
- [ ] Performance validation

---

## 📝 License

Apache-2.0

**Status**: Early Development 🚧  
**Last Updated**: February 11, 2026  
**Version**: 0.1.0-alpha
