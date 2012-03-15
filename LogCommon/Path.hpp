#pragma once
#include <string>

namespace Instalog { namespace Path {

	std::wstring Append(std::wstring path, std::wstring const& more);

	bool ResolveFromCommandLine(std::wstring &path);

}}
