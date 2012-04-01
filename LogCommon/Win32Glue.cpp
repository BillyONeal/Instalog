#include "pch.hpp"
#include "Win32Glue.hpp"
#include "Win32Exception.hpp"

namespace Instalog {

	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724928.aspx
	DWORD SecondsSince1970( FILETIME const& time )
	{
		// 2. Copy the contents of the FILETIME structure to a ULARGE_INTEGER structure.
		ULARGE_INTEGER inftime;
		inftime.LowPart = time.dwLowDateTime;
		inftime.HighPart = time.dwHighDateTime;	

		static ULARGE_INTEGER largejan1970;
		if (largejan1970.QuadPart == 0)
		{
			// 3. Initialize a SYSTEMTIME structure with the date and time of the first second of January 1, 1970.
			SYSTEMTIME jan1970 = { 1970, 1, 4,1,0,0,0,0};

			// 4. Call SystemTimeToFileTime, passing the SYSTEMTIME structure initialized in Step 3 to the call.
			FILETIME ftjan1970;
			if (SystemTimeToFileTime(&jan1970, &ftjan1970) == false)
			{
				SystemFacades::Win32Exception::ThrowFromLastError();
			}

			// 5. Copy the contents of the FILETIME structure returned by SystemTimeToFileTime in Step 4 to a second ULARGE_INTEGER. The copied value should be less than or equal to the value copied in Step 2.
			largejan1970;
			largejan1970.LowPart = ftjan1970.dwLowDateTime;
			largejan1970.HighPart = ftjan1970.dwHighDateTime;		
		}

		// 6. Subtract the 64-bit value in the ULARGE_INTEGER structure initialized in Step 5(January 1, 1970) from the 64-bit value of the ULARGE_INTEGER structure initialized in Step 2 (the current system time). This produces a value in 100-nanosecond intervals since January 1, 1970. To convert this value to seconds, divide by 10,000,000.
		ULONGLONG value100NS = inftime.QuadPart - largejan1970.QuadPart;
		ULONGLONG result = value100NS / 10000000;

		return static_cast<DWORD>(result);
	}

	// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724928.aspx
	DWORD SecondsSince1970( SYSTEMTIME const& time )
	{
		// 1. Call SystemTimeToFileTime to copy the system time to a FILETIME structure. Call GetSystemTime to get the current system time to pass to SystemTimeToFileTime.
		FILETIME filetime;
		if (SystemTimeToFileTime(&time, &filetime) == false)
		{
			SystemFacades::Win32Exception::ThrowFromLastError();
		}
		return SecondsSince1970(filetime);
	}

}