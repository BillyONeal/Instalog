#pragma once
#include <functional>

namespace Instalog { namespace SystemFacades {

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
