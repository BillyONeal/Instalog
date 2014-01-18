// Copyright © 2014 Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#include "pch.hpp"
#include "ErrorReporter.hpp"
#include "LogSink.hpp"
#include "Win32Exception.hpp"

using namespace Instalog;
using namespace Instalog::SystemFacades;

namespace Instalog
{
    IErrorReporter::~IErrorReporter()
    {
    }

    void IgnoreErrorReporter::ReportWinError(std::uint32_t, boost::string_ref)
    {
    }

    void IgnoreErrorReporter::ReportNtError(std::int32_t, boost::string_ref)
    {
    }

    void IgnoreErrorReporter::ReportHresult(std::int32_t, boost::string_ref)
    {
    }

    std::string const& LoggingErrorReporter::Get() const
    {
        return this->errorLog;
    }

    void LoggingErrorReporter::ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall)
    {
        write(this->errorLog, "WIN32 ", apiCall, ": (0x", hex(errorCode), ") ", GetWin32ErrorMessage(errorCode));
    }

    void LoggingErrorReporter::ReportNtError(std::int32_t errorCode, boost::string_ref apiCall)
    {
        std::uint32_t win32ErrorCode = GetWin32ErrorFromNtError(errorCode);
        write(this->errorLog, "NTSTATUS ", apiCall, ": (0x", hex(errorCode), ") ", GetWin32ErrorMessage(win32ErrorCode));
    }

    void LoggingErrorReporter::ReportHresult(std::int32_t errorCode, boost::string_ref apiCall)
    {
        write(this->errorLog, "HRESULT ", apiCall, ": (0x", hex(errorCode), ") ", GetHresultErrorMessage(errorCode));
    }

    void ThrowingErrorReporter::ReportWinError(std::uint32_t errorCode, boost::string_ref)
    {
        Win32Exception::Throw(errorCode);
    }

    void ThrowingErrorReporter::ReportNtError(std::int32_t errorCode, boost::string_ref)
    {
        Win32Exception::ThrowFromNtError(errorCode);
    }

    void ThrowingErrorReporter::ReportHresult(std::int32_t errorCode, boost::string_ref)
    {
        ThrowFromHResult(errorCode);
    }
}
