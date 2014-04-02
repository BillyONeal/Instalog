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

    struct IErrorReporter
    {
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall) = 0;
        virtual void ReportNtError(std::int32_t errorCode, boost::string_ref apiCall) = 0;
        virtual void ReportHresult(std::int32_t errorCode, boost::string_ref apiCall) = 0;
        virtual void ReportGenericError(boost::string_ref errorMessage) = 0;
    protected:
        ~IErrorReporter();
    };

    IErrorReporter& GetIgnoreReporter();

    struct LoggingErrorReporter final : public IErrorReporter
    {
        std::string const& Get() const;
        virtual void ReportWinError(std::uint32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportNtError(std::int32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportHresult(std::int32_t errorCode, boost::string_ref apiCall) override;
        virtual void ReportGenericError(boost::string_ref errorMessage) override;
    private:
        std::string errorLog;
    };

    IErrorReporter& GetThrowingErrorReporter();
}
