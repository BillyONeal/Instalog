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

		/// Constructor.
		///
		/// @param serviceName Name of the service.
		/// @param displayName Name of the display.
		/// @param status	   The status.
		/// @param scmHandle   Handle of the scm.
		Service(std::wstring const& serviceName, std::wstring const& displayName, SERVICE_STATUS const& status, SC_HANDLE scmHandle);
		/// Destructor.
		~Service();

		/// Gets the service name.
		///
		/// @return The service name.
		std::wstring const& getServiceName() const { return serviceName; }

		/// Gets the display name.
		///
		/// @return The display name.
		std::wstring const& getDisplayName() const { return displayName; }

		/// Gets the state.
		///
		/// @return The state.
		std::wstring const& getState() const { return state; }

		/// Gets the start.
		///
		/// @return The start.
		DWORD const& getStart() const { return start; }

		/// Gets the filepath.
		///
		/// @return The filepath.
		std::wstring const& getFilepath() const { return filepath; }

		/// Gets the svchost group.
		///
		/// @return The svchost group.
		std::wstring const& getSvchostGroup() const { return svchostGroup; }

		/// Gets the svchost dll.
		///
		/// @return The svchost dll.
		std::wstring const& getSvchostDll() const { return svchostDll; }

		/// Query if this object is svchost service.
		///
		/// @return true if svchost service, false if not.
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