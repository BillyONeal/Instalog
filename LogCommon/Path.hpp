#pragma once
#include <string>

namespace Instalog { namespace Path {

	std::wstring Append(std::wstring path, std::wstring const& more);

	std::wstring ResolveFromCommandLine(std::wstring const& path);

}}
