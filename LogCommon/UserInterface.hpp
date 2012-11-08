// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <cstddef>
#include <string>
#include <functional>

namespace Instalog
{
    /// @brief    Abstract user interface.
    struct IUserInterface
    {
        virtual ~IUserInterface() {}

        /// @brief    Reports the progress, in percent, to the user interface.
        ///
        /// @param    progress    The progress to report, ranged from 0 to 100.
        virtual void ReportProgressPercent(std::size_t progress) = 0;

        /// @brief    Reports execution finished to the user interface.
        virtual void ReportFinished() = 0;

        /// @brief    Logs a message to the user interface.
        ///
        /// @param    message    The message to log
        virtual void LogMessage(std::wstring const& message) = 0;
    };

    /// @brief    User interface which is a no-op for all operations.
    ///         
    /// @remarks Useful for testing.
    struct DoNothingUserInterface : public IUserInterface
    {
        virtual void ReportProgressPercent(std::size_t) {}
        virtual void ReportFinished() {}
        virtual void LogMessage(std::wstring const&) {}
    };

}
