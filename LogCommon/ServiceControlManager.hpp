// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <vector>
#include <boost/noncopyable.hpp>
#include <windows.h>
#include "Expected.hpp"

namespace Instalog
{
namespace SystemFacades
{

/// @brief    A simple wrapper around Services for the Services/Driver scanning
/// section
class Service : boost::noncopyable
{
    SC_HANDLE serviceHandle;

    std::string serviceName;
    std::string displayName;
    std::string state;
    DWORD start;
    std::string filepath;
    std::string svchostGroup;
    expected<std::string> svchostDll;
    bool svchostDamaged;

    public:
    /// Constructor.  Opens a handle to the Service and populates the various
    /// member variables
    ///
    /// @param serviceName Service name of the service.
    /// @param displayName Display name of the display.
    /// @param status       The status returned from the ServiceControlManager.
    /// @param scmHandle   Handle to the ServiceControlManager.
    ///
    /// @throw    Win32Exception on error
    Service(std::string const& serviceName,
            std::string const& displayName,
            SERVICE_STATUS const& status,
            SC_HANDLE scmHandle);

    /// @brief    Move constructor.
    ///
    /// @param [in,out]    s    The Service to move.
    Service(Service&& s);

    /// Destructor.  Frees the handle to the Service
    ~Service();

    /// @brief    Copy assignment operator.
    ///
    /// @param    s    The Service to copy.
    ///
    /// @return    A shallow copy of this instance.
    Service& operator=(Service s);

    /// Gets the service name.
    ///
    /// @return The service name.
    std::string const& GetServiceName() const
    {
        return serviceName;
    }

    /// Gets the display name.
    ///
    /// @return The display name.
    std::string const& GetDisplayName() const
    {
        return displayName;
    }

    /// Gets the state.
    ///
    /// @return The state.
    std::string const& GetState() const
    {
        return state;
    }

    /// Gets the start status.
    ///
    /// @return The start status.
    DWORD const& GetStart() const
    {
        return start;
    }

    bool IsDamagedSvchost() const
    {
        return svchostDamaged;
    }

    /// Gets the filepath.
    ///
    /// @return The filepath.
    std::string const& GetFilepath() const
    {
        return filepath;
    }

    /// Gets the svchost group.
    ///
    /// @return The svchost group.
    std::string const& GetSvchostGroup() const
    {
        return svchostGroup;
    }

    /// Gets the svchost dll path.
    ///
    /// @return The svchost dll path.
    expected<std::string> const& GetSvchostDll() const
    {
        return svchostDll;
    }

    /// Query if this object is svchost service.
    ///
    /// @return true if svchost service, false if not.
    bool IsSvchostService() const
    {
        return !svchostGroup.empty();
    }
};

/// @brief    Wrapper around the Windows Service Control Manager
class ServiceControlManager
{
    SC_HANDLE scmHandle;

    ServiceControlManager(ServiceControlManager const&);
    ServiceControlManager& operator=(ServiceControlManager const&);

    public:
    /// @brief    Constructor.  Opens a handle to the service control manager
    ///
    /// @param    desiredAccess    (optional) the desired access.  Defaults are
    /// enough to support normal operations
    ///
    /// @throw    Win32Exception on error
    ServiceControlManager(DWORD desiredAccess = SC_MANAGER_CONNECT |
                                                SC_MANAGER_ENUMERATE_SERVICE);

    /// @brief    Destructor.  Closes the handle
    ~ServiceControlManager();

    /// @brief    Enumerates all of the services running on the machine.
    ///
    /// @return    A vector of Service objects
    ///
    /// @throw    Win32Exception on error
    std::vector<Service> GetServices() const;
};
}
}
