#include "pch.hpp"
#include <algorithm>
#include "Win32Exception.hpp"
#include "ServiceControlManager.hpp"
#include "Path.hpp"

namespace Instalog { namespace SystemFacades {

	Service::Service( std::wstring const& serviceName, std::wstring const& displayName, SERVICE_STATUS const& status, SC_HANDLE scmHandle ) 
		: serviceName(serviceName)
		, displayName(displayName)
	{
		// Set the state
		switch (status.dwCurrentState)
		{
		case SERVICE_STOPPED:			this->state = L"S"; break;
		case SERVICE_START_PENDING:		this->state = L"R?"; break;
		case SERVICE_STOP_PENDING:		this->state = L"S?"; break;
		case SERVICE_RUNNING:			this->state = L"R"; break;
		case SERVICE_CONTINUE_PENDING:	this->state = L"C?"; break;
		case SERVICE_PAUSE_PENDING:		this->state = L"P?"; break;
		case SERVICE_PAUSED:			this->state = L"P"; break;
		default:						this->state = L"?"; break;
		}

		SC_HANDLE serviceHandle = OpenServiceW(scmHandle, this->serviceName.c_str(), SERVICE_QUERY_CONFIG);
		if (serviceHandle == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}
		char queryServiceConfigBuffer[8192 /* (8K bytes maximum size) */];
		DWORD bytesNeeded = 0; // not needed
		if (QueryServiceConfig(serviceHandle, reinterpret_cast<LPQUERY_SERVICE_CONFIGW>(queryServiceConfigBuffer), 8192, &bytesNeeded) == false)
		{
			Win32Exception::ThrowFromLastError();
		}
		QUERY_SERVICE_CONFIGW *queryServiceConfig = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(queryServiceConfigBuffer);

		// Set the start type
		this->start = queryServiceConfig->dwStartType;

		// Set the file
		this->filepath = queryServiceConfig->lpBinaryPathName;
		Path::ResolveFromCommandLine(this->filepath);

		// Set the svchost group and dll if applicable
		if (this->filepath == Path::Append(Path::GetWindowsPath(), L"System32\\Svchost.exe"))
		{
			this->svchostGroup = queryServiceConfig->lpBinaryPathName;
			this->svchostGroup.erase(this->svchostGroup.begin(), boost::ifind_first(this->svchostGroup, L"-k").end());			
			boost::trim(this->svchostGroup);
		}
	}

	Service::~Service()
	{
		CloseServiceHandle(serviceHandle);
	}

	ServiceControlManager::ServiceControlManager( DWORD desiredAccess /* = SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE */ )
	{
		scmHandle = OpenSCManagerW(NULL, NULL, desiredAccess);

		if (scmHandle == NULL)
		{
			Win32Exception::ThrowFromLastError();
		}
	}

	ServiceControlManager::~ServiceControlManager()
	{
		CloseServiceHandle(scmHandle);
	}

	std::vector<Service> ServiceControlManager::GetServices() const
	{
		std::vector<Service> services;

		char servicesBuffer[65536 /* (64K bytes max size) */];
		DWORD bytesNeeded = 0; // not needed
		DWORD servicesReturned = 0;
		DWORD resumeHandle = 0;
		BOOL status = false;
		DWORD error = ERROR_MORE_DATA;

		while (status == false && error == ERROR_MORE_DATA)
		{
			status = EnumServicesStatusW(scmHandle, SERVICE_DRIVER | SERVICE_WIN32, SERVICE_STATE_ALL, reinterpret_cast<ENUM_SERVICE_STATUSW*>(servicesBuffer), 65536, &bytesNeeded, &servicesReturned, &resumeHandle);
			error = GetLastError();
			if (status == false && error != ERROR_MORE_DATA)
			{
				Win32Exception::Throw(error);
			}

			for (char* servicesBufferLocation = servicesBuffer; servicesReturned > 0; --servicesReturned, servicesBufferLocation += sizeof(ENUM_SERVICE_STATUSW))
			{
				ENUM_SERVICE_STATUS *enumServiceStatus = reinterpret_cast<ENUM_SERVICE_STATUSW*>(servicesBufferLocation);
				services.push_back(Service(enumServiceStatus->lpServiceName, enumServiceStatus->lpDisplayName, enumServiceStatus->ServiceStatus, scmHandle));
			}
		}

		return services;
	}
}}