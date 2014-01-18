// Copyright © 2014 Billy O'Neal III
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

    struct IErrorReporter
    {
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall) = 0;
        virtual void ReportNtError(std::int32_t errorCode, boost::string_ref apiCall) = 0;
        virtual void ReportHresult(std::int32_t errorCode, boost::string_ref apiCall) = 0;
    protected:
        ~IErrorReporter();
    };

    struct IgnoreErrorReporter : public IErrorReporter
    {
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportNtError(std::int32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportHresult(std::int32_t errorCode, boost::string_ref apiCall) override;
    };

    struct LoggingErrorReporter : public IErrorReporter
    {
        std::string const& Get() const;
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportNtError(std::int32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportHresult(std::int32_t errorCode, boost::string_ref apiCall) override;
    private:
        std::string errorLog;
    };

    struct ThrowingErrorReporter : public IErrorReporter
    {
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportNtError(std::int32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportHresult(std::int32_t errorCode, boost::string_ref apiCall) override;
    };
}
