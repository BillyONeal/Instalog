#include <cstdio>

#ifdef TRACE_ENABLED
#define INSTALOG_TRACE(x) std::fprintf(stderr, "TRACE in %d on %d %s\n", __FILE__, __LINE__, x); std::fflush(stderr);
#else
#define INSTALOG_TRACE(x)
#endif
