// Copyright © Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

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
        IgnoreErrorReporter() = default;
        virtual void ReportWinError(std::uint32_t, boost::string_ref) override
        {}
        virtual void ReportNtError(std::int32_t, boost::string_ref) override
        {}
        virtual void ReportHresult(std::int32_t, boost::string_ref) override
        {}
        virtual void ReportGenericError(boost::string_ref) override
        {}
    };

    struct ThrowingErrorReporter final : public IErrorReporter
    {
        ThrowingErrorReporter() = default;
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref) override
        {
            Win32Exception::Throw(errorCode);
        }
        virtual void ReportNtError(std::int32_t errorCode, boost::string_ref) override
        {
            Win32Exception::ThrowFromNtError(errorCode);
        }
        virtual void ReportHresult(std::int32_t errorCode, boost::string_ref) override
        {
            ThrowFromHResult(errorCode);
        }
        virtual void ReportGenericError(boost::string_ref errorMessage) override
        {
            throw std::runtime_error(errorMessage.to_string());
        }
    };

    IErrorReporter& GetIgnoreReporter()
    {
        static IgnoreErrorReporter reporter;
        return reporter;
    }

    IErrorReporter& GetThrowingErrorReporter()
    {
        static ThrowingErrorReporter reporter;
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

    BasicFilteringErrorReporter::BasicFilteringErrorReporter(IErrorReporter& wrappedReporter)
        : wrappedReporter(&wrappedReporter)
    { }

    void BasicFilteringErrorReporter::ReportGenericError(boost::string_ref errorMessage)
    {
        this->wrappedReporter->ReportGenericError(errorMessage);
    }

    void BasicFilteringErrorReporter::ReportHresult(std::int32_t errorCode, boost::string_ref apiCall)
    {
        this->wrappedReporter->ReportHresult(errorCode, apiCall);
    }

    void BasicFilteringErrorReporter::ReportNtError(std::int32_t errorCode, boost::string_ref apiCall)
    {
        this->wrappedReporter->ReportNtError(errorCode, apiCall);
    }

    void BasicFilteringErrorReporter::ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall)
    {
        this->wrappedReporter->ReportWinError(errorCode, apiCall);
    }


    Win32FilteringReporter::Win32FilteringReporter(IErrorReporter& wrappedReporter, std::uint32_t filteredErrorCode)
        : BasicFilteringErrorReporter(wrappedReporter)
        , filteredErrorCode(filteredErrorCode)
    { }

    void Win32FilteringReporter::ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall)
    {
        if (errorCode == this->filteredErrorCode)
        {
            return;
        }

        BasicFilteringErrorReporter::ReportWinError(errorCode, apiCall);
    }

}
