#pragma once
#include <string>

namespace Instalog { namespace SystemFacades {

	class Path
	{
		std::wstring underlyingPath_;
	public:
		std::wstring const& AsString() const;
		//Returns a new path
		Path operator/(Path const& rhs) const;
		//Changes the current path
		Path operator/=(Path const& rhs) const;
		/*implicit*/ Path(std::wstring const&);
		/*implicit*/ Path(wchar_t const*);
		Path(Path &&);
		static Path ResolveFromCommandLine(std::wstring const&);
		static Path ResolveFromCommandLine(std::wstring &&);
	};

}}
