#pragma once
#include <vector>
#include <string>

namespace Instalog {

	class Whitelist
	{
		std::vector<std::wstring> innards;
	public:
		Whitelist(__int32 whitelistId);
		bool IsOnWhitelist(std::wstring const& checked) const;
	};

}
