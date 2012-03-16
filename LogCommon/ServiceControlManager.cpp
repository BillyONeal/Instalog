#include "pch.hpp"
#include "Win32Exception.hpp"
#include "ServiceControlManager.hpp"

namespace Instalog { namespace SystemFacades {

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

	std::vector<Service> ServiceControlManager::GetServices()
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
			status = EnumServicesStatusW(scmHandle, SERVICE_DRIVER | SERVICE_WIN32, SERVICE_STATE_ALL, reinterpret_cast<ENUM_SERVICE_STATUSW*>(servicesBuffer.data()), servicesBuffer.size(), &bytesNeeded, &servicesReturned, &resumeHandle);
			error = GetLastError();
			if (status == false && error != ERROR_MORE_DATA)
			{
				Win32Exception::Throw(error);
			}

			for (std::vector<char>::iterator it = servicesBuffer.begin(); servicesReturned > 0; --servicesReturned, it += sizeof(ENUM_SERVICE_STATUSW))
			{
				ENUM_SERVICE_STATUS *enumServiceStatus = reinterpret_cast<ENUM_SERVICE_STATUSW*>(&(*it));
				services.push_back(Service(enumServiceStatus->lpServiceName, enumServiceStatus->lpDisplayName, enumServiceStatus->ServiceStatus));
			}
		}

		return services;
	}

}}