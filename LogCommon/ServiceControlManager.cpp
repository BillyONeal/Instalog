#include "pch.hpp"
#include <algorithm>
#include "Win32Exception.hpp"
#include "ServiceControlManager.hpp"
#include "Path.hpp"

namespace Instalog { namespace SystemFacades {

	Service::Service( std::wstring const& serviceName, std::wstring const& displayName, SERVICE_STATUS const& status, SC_HANDLE scmHandle )
	{
		// Set the service and display name
		this->serviceName = serviceName;
		this->displayName = displayName;

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
		std::vector<char> queryServiceConfigBuffer;
		DWORD bufferSize = 0;
		DWORD bytesNeeded = 0;
		QueryServiceConfig(serviceHandle, NULL, 0, &bytesNeeded);
		DWORD queryServiceConfigError = GetLastError();
		if (queryServiceConfigError != ERROR_INSUFFICIENT_BUFFER)
		{
			CloseServiceHandle(serviceHandle);
			Win32Exception::Throw(queryServiceConfigError);
		}
		queryServiceConfigBuffer.resize(bytesNeeded);
		if (QueryServiceConfig(serviceHandle, reinterpret_cast<QUERY_SERVICE_CONFIGW*>(queryServiceConfigBuffer.data()), static_cast<DWORD>(queryServiceConfigBuffer.size()), &bytesNeeded) == false)
		{
			CloseServiceHandle(serviceHandle);
			Win32Exception::ThrowFromLastError();
		}
		CloseServiceHandle(serviceHandle);
		QUERY_SERVICE_CONFIGW *queryServiceConfig = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(&(*queryServiceConfigBuffer.begin()));

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

		std::vector<char> servicesBuffer;
		DWORD bytesNeeded = 0;
		DWORD servicesReturned = 0;
		DWORD resumeHandle = 0;
		BOOL status = EnumServicesStatusW(scmHandle, SERVICE_DRIVER | SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &bytesNeeded, &servicesReturned, &resumeHandle);
		DWORD error = GetLastError();
		if (error != ERROR_INSUFFICIENT_BUFFER && error != ERROR_MORE_DATA)
		{
			Win32Exception::Throw(error);
		}

		while (status == false)
		{
			servicesBuffer.resize(bytesNeeded);
			status = EnumServicesStatusW(scmHandle, SERVICE_DRIVER | SERVICE_WIN32, SERVICE_STATE_ALL, reinterpret_cast<ENUM_SERVICE_STATUSW*>(servicesBuffer.data()), static_cast<DWORD>(servicesBuffer.size()), &bytesNeeded, &servicesReturned, &resumeHandle);
			error = GetLastError();
			if (status == false && error != ERROR_MORE_DATA)
			{
				Win32Exception::Throw(error);
			}

			for (std::vector<char>::iterator it = servicesBuffer.begin(); servicesReturned > 0; --servicesReturned, it += sizeof(ENUM_SERVICE_STATUSW))
			{
				ENUM_SERVICE_STATUS *enumServiceStatus = reinterpret_cast<ENUM_SERVICE_STATUSW*>(&(*it));
				services.push_back(Service(enumServiceStatus->lpServiceName, enumServiceStatus->lpDisplayName, enumServiceStatus->ServiceStatus, scmHandle));
			}
		}

		return services;
	}
}}