#pragma once
#include <vector>
#include <windows.h>

namespace Instalog { namespace SystemFacades {

	class Service
	{
		std::wstring serviceName;
		std::wstring displayName;
		SERVICE_STATUS status;
	public:
		Service(std::wstring const& serviceName, std::wstring const& displayName, SERVICE_STATUS const& status)
		{
			this->serviceName = serviceName;
			this->displayName = displayName;
			this->status = status;
		}
	};

	class ServiceControlManager
	{
		SC_HANDLE scmHandle;
	public:
		ServiceControlManager(DWORD desiredAccess = SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
		~ServiceControlManager();
		std::vector<Service> GetServices();
	};

}}