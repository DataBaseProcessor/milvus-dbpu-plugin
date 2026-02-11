#include <dbpu/interceptor.h>
#include <dbpu/runtime_loader.h>
#include <dbpu/logger.h>
#include <dbpu/profiler.h>
#include <dbpu/offload.h>

namespace dbpu {

SearchInterceptor::SearchInterceptor() {
  Logger::instance().init("/tmp/dbpu-knowhere.jsonl");
}

SearchInterceptor::~SearchInterceptor() = default;

bool SearchInterceptor::is_dbpu_available() {
  return RuntimeLoader::instance().is_available();
}

SearchResult SearchInterceptor::intercept_search(
  const SearchRequest& request,
  std::function<SearchResult(const SearchRequest&)> cpu_fallback
) {
  Profiler prof;
  prof.start();
  prof.set_basic(request.index_type, request.n_queries, request.dim, request.top_k);

  OffloadConfig cfg = OffloadConfig::load_default();
  OffloadDecider dec(cfg);

  // MVP: 간단 rule
  double est_scan = (request.index_type == "FLAT") ? 95.0 :
                    (request.index_type == "IVF_FLAT") ? 80.0 : 10.0;

  const bool want = dec.should_offload(request.index_type, request.n_queries, est_scan);
  SearchResult out;

  if (want && RuntimeLoader::instance().is_available()) {
    try {
      out = RuntimeLoader::instance().search(
        request.vectors, request.n_vectors,
        request.queries, request.n_queries,
        request.dim, request.top_k
      );
      prof.set_decision(true, dec.reason());
    } catch (...) {
      out = cpu_fallback(request);
      prof.set_decision(false, "dbpu failed -> cpu fallback");
    }
  } else {
    out = cpu_fallback(request);
    prof.set_decision(false, want ? "dbpu runtime not available" : dec.reason());
  }

  Logger::instance().log(prof.stop());
  return out;
}

} // namespace dbpu