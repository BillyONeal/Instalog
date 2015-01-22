// Copyright © Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <boost/utility/string_ref.hpp>

namespace Instalog
{
    struct LineInfo
    {
        char const* file;
        std::size_t line;
    };

#define INSTALOG_LINE LineInfo {__FILE__, __LINE__}

    // Generic interface for error reporting; allows code to delegate how errors
    // are handled to callers.
    struct IErrorReporter
    {
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall) = 0;
        virtual void ReportNtError(std::int32_t errorCode, boost::string_ref apiCall) = 0;
        virtual void ReportHresult(std::int32_t errorCode, boost::string_ref apiCall) = 0;
        virtual void ReportGenericError(boost::string_ref errorMessage) = 0;
    protected:
        IErrorReporter() = default;
        IErrorReporter(IErrorReporter const&) = default;
        IErrorReporter& operator=(IErrorReporter const&) = default;
        ~IErrorReporter();
    };

    // Returns an implementation of IErrorReporter which does nothing. (Ignores errors)
    IErrorReporter& GetIgnoreReporter();

    // Returns an implementation of IErrorReporter which throws exceptions.
    IErrorReporter& GetThrowingErrorReporter();

    struct LoggingErrorReporter final : public IErrorReporter
    {
        std::string const& Get() const;
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportNtError(std::int32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportHresult(std::int32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportGenericError(boost::string_ref errorMessage) override;
        LoggingErrorReporter() = default;
        LoggingErrorReporter(LoggingErrorReporter const&) = default;
        LoggingErrorReporter& operator=(LoggingErrorReporter const&) = default;
    private:
        std::string errorLog;
    };

    // An implementation of IErrorReporter that forwards to another implementation of IErrorReporter.
    struct BasicFilteringErrorReporter abstract : public IErrorReporter
    {
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportNtError(std::int32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportHresult(std::int32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportGenericError(boost::string_ref errorMessage) override;
    protected:

        BasicFilteringErrorReporter(IErrorReporter& wrappedReporter);
        BasicFilteringErrorReporter(BasicFilteringErrorReporter const&) = default;
        BasicFilteringErrorReporter& operator=(BasicFilteringErrorReporter const&) = default;
        IErrorReporter* wrappedReporter;
    };

    // An implementation of IErrorReporter that ignores a specific error code.
    struct Win32FilteringReporter final : public BasicFilteringErrorReporter
    {
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall) override;
        Win32FilteringReporter(IErrorReporter& wrappedReporter, std::uint32_t filteredErrorCode);
    private:
        std::uint32_t filteredErrorCode;
    };

    // Gets an implementation of IErrorReporter that ignores errors except for the one indicated.
    template <unsigned BlockedErrorCode>
    IErrorReporter& GetThrowingReporterExcept()
    {
        static Win32FilteringReporter reporter(GetThrowingErrorReporter(), BlockedErrorCode);
        return reporter;
    }
}
