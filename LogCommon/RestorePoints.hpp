#pragma once

#include <string>
#include <vector>

namespace Instalog { namespace SystemFacades {

	struct RestorePoint
	{
		std::wstring Description;
		unsigned int RestorePointType;
		unsigned int EventType;
		unsigned int SequenceNumber;
		std::wstring CreationTime;

		SYSTEMTIME CreationTimeAsSystemTime();
	};

	std::vector<RestorePoint> EnumerateRestorePoints();

}}