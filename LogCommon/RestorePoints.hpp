// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once

#include <string>
#include <vector>

namespace Instalog { namespace SystemFacades {

    /// @brief    Restore point WMI wrapper
    struct RestorePoint
    {
        /// @summary    The description.
        std::wstring Description;

        /// @summary    Type of the restore point.
        unsigned int RestorePointType;

        /// @summary    Type of the event.
        unsigned int EventType;

        /// @summary    The sequence number.
        unsigned int SequenceNumber;

        /// @summary    Time of the creation as a WMI date string
        std::wstring CreationTime;
    };

    /// @brief    Enumerates all available restore points
    ///
    /// @return    A vector of Restore Points
    std::vector<RestorePoint> EnumerateRestorePoints();

}}