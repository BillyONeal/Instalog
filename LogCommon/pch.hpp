#define _VARIADIC_MAX 10
#define _SCL_SECURE_NO_WARNINGS
#include <cstdlib>
#include <cstring>
#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include <iterator>
#include <functional>
#include <array>
#include <iomanip>
#include <stdexcept>
#include <iostream>
#include <sstream>

#define NOMINMAX
#define NTDDI_VERSION 0x05010200
#define _WIN32_WINNT 0x0501
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <boost/iterator/iterator_facade.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
