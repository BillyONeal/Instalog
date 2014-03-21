// Copyright © Billy O'Neal III
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

    struct IgnoreErrorReporter final : public IErrorReporter
    {
        virtual void ReportWinError(std::uint32_t, boost::string_ref) override
        {}
        virtual void ReportNtError(std::int32_t, boost::string_ref) override
        {}
        virtual void ReportHresult(std::int32_t, boost::string_ref) override
        {}
        virtual void ReportGenericError(boost::string_ref) override
        {}
    };

    IErrorReporter& GetIgnoreReporter()
    {
        static IgnoreErrorReporter reporter;
        return reporter;
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

    void LoggingErrorReporter::ReportGenericError(boost::string_ref errorMessage)
    {
        writeln(this->errorLog, "ERROR: ", errorMessage);
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

    void ThrowingErrorReporter::ReportGenericError(boost::string_ref errorMessage)
    {
        throw std::runtime_error(errorMessage.to_string());
    }
}
