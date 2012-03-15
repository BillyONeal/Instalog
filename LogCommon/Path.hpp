#pragma once
#include <string>

namespace Instalog { namespace Path {

	std::wstring Append(std::wstring path, std::wstring const& more);
	void Prettify(std::wstring::iterator first, std::wstring::iterator last);
	void ResolveFromCommandLine(std::wstring &path);

}}
