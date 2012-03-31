// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <cstddef>
#include <string>
#include <functional>

namespace Instalog
{

	struct IUserInterface
	{
		virtual ~IUserInterface() {}
		virtual void ReportProgressPercent(std::size_t progress) = 0;
		virtual void ReportFinished() = 0;
		virtual void LogMessage(std::wstring const&) = 0;
	};

	struct DoNothingUserInterface : public IUserInterface
	{
		virtual void ReportProgressPercent(std::size_t) {}
		virtual void ReportFinished() {}
		virtual void LogMessage(std::wstring const&) {}
	};

}
