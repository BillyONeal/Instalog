#include <ostream>

#ifdef TRACE_ENABLED
#define INSTALOG_TRACE(x) std::wcerr << L"TRACE in " << __FILE__ << L" on " << __LINE__ << L' ' << x << std::endl;
#else
#define INSTALOG_TRACE(x)
#endif
