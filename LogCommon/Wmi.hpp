#pragma once

#include <atlbase.h>
# pragma comment(lib, "wbemuuid.lib")
#include <wbemidl.h>

namespace Instalog { namespace SystemFacades {
	
	CComPtr<IWbemServices> GetWbemServices();

}}