#pragma once
#include <functional>

namespace Instalog { namespace SystemFacades {

	struct HandleCloser : public std::unary_function<void, void*>
	{
		void operator()(void* hClosed)
		{
			::CloseHandle(reinterpret_cast<HANDLE>(hClosed));
		}
	};

}}
