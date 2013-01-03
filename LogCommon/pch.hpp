// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#define _VARIADIC_MAX 10
#define _SCL_SECURE_NO_WARNINGS
#include <cstdlib>
#include <cstring>
#include <cstdint>
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
#include "ntstatus.h"

#define NOMINMAX
#define NTDDI_VERSION 0x05010200
#define _WIN32_WINNT 0x0501
#define WIN32_NO_STATUS
#pragma comment(lib, "Ws2_32.lib")
#include <Winsock2.h>
#include <windows.h>
#undef WIN32_NO_STATUS
#include <boost/iterator/iterator_facade.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
