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

}
