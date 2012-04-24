// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <functional>

namespace Instalog { namespace SystemFacades {

	/// @brief	Handle closer.
	struct HandleCloser : public std::unary_function<void, void*>
	{
		void operator()(void* hClosed)
		{
			HANDLE handle = reinterpret_cast<HANDLE>(hClosed);
			if (handle != INVALID_HANDLE_VALUE)
				::CloseHandle(handle);
		}
	};

}}
