#pragma once
#include <vector>
#include <string>
#include <ostream>

namespace Instalog {

	/// @brief	General whitelisting class
	/// 
	/// @details	This class does general whitelisting with preface replacing.  If more fine-grained whitelisting is required, it should be implemented in-place
	class Whitelist
	{
		std::vector<std::wstring> innards;
	public:
		/// @brief	Constructor.
		///
		/// @param	whitelistId 	Identifier for the whitelist from the resource file.
		/// @param	replacements	(optional) the replacements.  These are in the form of pairs of strings.  For each string in the whitelist, if it matches the 
		/// 						first part of any pair, then it will be replaced with the second part of the pair.
		Whitelist(__int32 whitelistId, std::vector<std::pair<std::wstring, std::wstring>> const& replacements = std::vector<std::pair<std::wstring, std::wstring>>() );

		/// @brief	Query if an item should be whitelisted
		///
		/// @param	checked	The item to check
		///
		/// @return	true if it should be whitelisted
		bool IsOnWhitelist(std::wstring checked) const;

		/// @brief	Print all of the whitelist elements
		///
		/// @param [out]	str	The stream to write to.
		void PrintAll(std::wostream & str) const;
	};

}
