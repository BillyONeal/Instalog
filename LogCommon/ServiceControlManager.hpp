#pragma once
#include <vector>
#include <windows.h>

namespace Instalog { namespace SystemFacades {

	class Service
	{
		SC_HANDLE serviceHandle;

		std::wstring serviceName;
		std::wstring displayName;
		std::wstring state;
		DWORD start;
		std::wstring filepath;
		std::wstring svchostGroup;
		std::wstring svchostDll;
	public:
		Service(std::wstring const& serviceName, std::wstring const& displayName, SERVICE_STATUS const& status, SC_HANDLE scmHandle);
		~Service();
		std::wstring const& getServiceName() const { return serviceName; }
		std::wstring const& getDisplayName() const { return displayName; }
		std::wstring const& getState() const { return state; }
		DWORD const& getStart() const { return start; }
		std::wstring const& getFilepath() const { return filepath; }
		std::wstring const& getSvchostGroup() const { return svchostGroup; }
		std::wstring const& getSvchostDll() const { return svchostDll; }
		bool isSvchostService() const { return !svchostGroup.empty() || !svchostDll.empty(); }
	};

	class ServiceControlManager
	{
		SC_HANDLE scmHandle;

		ServiceControlManager(ServiceControlManager const&);
		ServiceControlManager& operator=(ServiceControlManager const&);
	public:
		ServiceControlManager(DWORD desiredAccess = SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE);
		~ServiceControlManager();
		std::vector<Service> GetServices() const;
	};

}}