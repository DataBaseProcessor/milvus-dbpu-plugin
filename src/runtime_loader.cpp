#include <dbpu/runtime_loader.h>

#include <dlfcn.h>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>

namespace dbpu {

struct RuntimeLoaderImpl {
    void* handle = nullptr;
    dbpu_device_t device = nullptr;

    // ABI functions
    int (*get_devices)(dbpu_device_type_t*, int) = nullptr;
    dbpu_status_t (*init_device)(int, dbpu_device_t*) = nullptr;
    void (*destroy_device)(dbpu_device_t) = nullptr;
    dbpu_status_t (*search)(
        dbpu_device_t,
        const float*, const float*,
        int, int, int, int,
        dbpu_metric_t,
        float*, int64_t*) = nullptr;

    dbpu_status_t (*get_perf)(dbpu_device_t, dbpu_perf_t*) = nullptr;

    bool available() const {
        return handle && device && search;
    }
};

static RuntimeLoaderImpl g_rt;

RuntimeLoader& RuntimeLoader::instance() {
    static RuntimeLoader inst;
    return inst;
}

RuntimeLoader::RuntimeLoader() {
    const char* libs[] = {
        "libdbpu-runtime.dylib",
        "libdbpu-runtime.so",
        "/usr/local/lib/libdbpu-runtime.dylib",
        "/usr/local/lib/libdbpu-runtime.so",
        "/opt/homebrew/lib/libdbpu-runtime.dylib",
        nullptr
    };

    for (int i = 0; libs[i]; ++i) {
        g_rt.handle = dlopen(libs[i], RTLD_LAZY | RTLD_LOCAL);
        if (g_rt.handle) break;
    }

    if (!g_rt.handle) return;

#define LOAD_FIELD(field, symname) \
  *(void**)(&g_rt.field) = dlsym(g_rt.handle, symname)

LOAD_FIELD(get_devices,    "dbpu_get_devices");
LOAD_FIELD(init_device,    "dbpu_init_device");
LOAD_FIELD(destroy_device, "dbpu_destroy_device");
LOAD_FIELD(search,         "dbpu_search");
LOAD_FIELD(get_perf,       "dbpu_get_perf");

#undef LOAD_FIELD

    if (!g_rt.init_device || !g_rt.search) {
        dlclose(g_rt.handle);
        g_rt.handle = nullptr;
        return;
    }

    // Initialize device 0
    if (g_rt.init_device(0, &g_rt.device) != DBPU_SUCCESS) {
        dlclose(g_rt.handle);
        g_rt.handle = nullptr;
        g_rt.device = nullptr;
        return;
    }
}

RuntimeLoader::~RuntimeLoader() {
    if (g_rt.destroy_device && g_rt.device) {
        g_rt.destroy_device(g_rt.device);
    }
    if (g_rt.handle) {
        dlclose(g_rt.handle);
    }
}

bool RuntimeLoader::is_available() const {
    return g_rt.available();
}

SearchResult RuntimeLoader::search(
    const float* database, int n_vectors,
    const float* queries, int n_queries,
    int dim, int top_k
) {
    SearchResult result;

    result.distances.resize(n_queries * top_k);
    result.indices.resize(n_queries * top_k);

    if (!g_rt.available()) {
        throw std::runtime_error("DBPU runtime unavailable");
    }

    auto st = g_rt.search(
        g_rt.device,
        queries,
        database,
        n_queries,
        n_vectors,
        dim,
        top_k,
        DBPU_METRIC_L2,
        result.distances.data(),
        result.indices.data()
    );

    if (st != DBPU_SUCCESS) {
        throw std::runtime_error("dbpu_search failed");
    }

    return result;
}

bool RuntimeLoader::get_perf(dbpu_perf_t* perf) {
    if (!g_rt.available() || !g_rt.get_perf) return false;
    return g_rt.get_perf(g_rt.device, perf) == DBPU_SUCCESS;
}

} // namespace dbpu