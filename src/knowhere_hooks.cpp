#include <unistd.h>
#include <string.h>
static inline void __dbpu_log(const char* s){ ssize_t r = write(2,s,strlen(s)); (void)r; }

#if defined(__GNUC__)
  #define DBPU_KN_EXPORT __attribute__((visibility("default")))
#else
  #define DBPU_KN_EXPORT
#endif

extern "C" {

// NOTE: keep signatures C-exported & stable.
// These are called by knowhere (via dlsym/RTLD_DEFAULT or explicit linkage).
DBPU_KN_EXPORT void dbpu_kn_pre_search_hook() {
    __dbpu_log("[DBPU] pre_search_hook\n");
}

DBPU_KN_EXPORT void dbpu_kn_post_search_hook() {
    __dbpu_log("[DBPU] post_search_hook\n");
}

} // extern "C"
