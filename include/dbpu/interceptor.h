#pragma once
#include <functional>
#include <string>
#include <vector>

namespace dbpu {

struct SearchRequest {
  const float* vectors = nullptr;
  int n_vectors = 0;

  const float* queries = nullptr;
  int n_queries = 0;

  int dim = 0;
  int top_k = 0;

  std::string index_type;     // "FLAT", "IVF_FLAT", "IVF_PQ", ...
  std::string index_params;   // json string (optional)
  std::string search_params;  // json string (optional)
};

struct SearchResult {
  std::vector<float> distances;
  std::vector<int64_t> indices;
};

class SearchInterceptor {
public:
  SearchInterceptor();
  ~SearchInterceptor();

  // Milvus/Knowhere hook에서 호출할 API
  SearchResult intercept_search(
    const SearchRequest& request,
    std::function<SearchResult(const SearchRequest&)> cpu_fallback
  );

  static bool is_dbpu_available();
};

} // namespace dbpu