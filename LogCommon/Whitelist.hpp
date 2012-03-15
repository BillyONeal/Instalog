#pragma once
#include <vector>
#include <string>

namespace Instalog {

	class Whitelist
	{
		std::vector<std::wstring> innards;
	public:
		Whitelist(__int32 whitelistId, std::vector<std::pair<std::wstring, std::wstring>> const& replacements = std::vector<std::pair<std::wstring, std::wstring>>() );
		bool IsOnWhitelist(std::wstring checked) const;
	};

}
