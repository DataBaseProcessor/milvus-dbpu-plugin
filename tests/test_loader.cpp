#include <dbpu/runtime_loader.h>
#include <iostream>
#include <vector>

int main() {
    std::cout << "\n=== DBPU Runtime Loader Smoke Test ===\n";

    auto& rt = dbpu::RuntimeLoader::instance();

    std::cout << "Runtime available: "
              << (rt.is_available() ? "YES" : "NO") << "\n";

    if (!rt.is_available()) return 0;

    const int nq = 2;
    const int nb = 16;
    const int dim = 4;
    const int k = 3;

    float queries[nq * dim] = {
        0,0,0,0,
        1,1,1,1
    };

    float database[nb * dim];
    for (int i=0;i<nb*dim;i++) database[i] = float(i % 7);

    auto res = rt.search(database, nb, queries, nq, dim, k);

    std::cout << "\nTop results:\n";
    for (int i=0;i<k;i++) {
        std::cout << " idx=" << res.indices[i]
                  << " dist=" << res.distances[i] << "\n";
    }

    dbpu_perf_t perf;
    if (rt.get_perf(&perf)) {
        std::cout << "\nPerf:\n";
        std::cout << " compute_us=" << perf.compute_time_us << "\n";
        std::cout << " total_us=" << perf.total_time_us << "\n";
        std::cout << " gflops=" << perf.throughput_gflops << "\n";
    }

    return 0;
}