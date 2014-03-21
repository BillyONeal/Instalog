// Copyright � Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <windows.h>
#include "gtest/gtest.h"
#include "../LogCommon/ErrorReporter.hpp"
#include "../LogCommon/Win32Exception.hpp"

using namespace Instalog;
using namespace Instalog::SystemFacades;

TEST(IgnoreErrorReporterTests, WinErrorIsANoOp)
{
    GetIgnoreReporter().ReportWinError(0u, "Example API");
}

TEST(IgnoreErrorReporterTests, NtErrorIsANoOp)
{
    GetIgnoreReporter().ReportNtError(0u, "Example API");
}

TEST(IgnoreErrorReporterTests, HresultIsANoOp)
{
    GetIgnoreReporter().ReportHresult(0u, "Example API");
}

TEST(IgnoreErrorReporterTests, GenericErrorIsANoOp)
{
    GetIgnoreReporter().ReportGenericError("Error message");
}

struct LoggingErrorReporterTests : public ::testing::Test
{
    LoggingErrorReporter reporter;
    void Check(std::string const& str)
    {
        EXPECT_EQ(str + "\r\n", reporter.Get());
    }
};

TEST_F(LoggingErrorReporterTests, ReportWinErrorWithSuccessCodeLogsMessage)
{
    reporter.ReportWinError(ERROR_SUCCESS, "SuccessfulApi");
    Check("WIN32 SuccessfulApi: (0x00000000) The operation completed successfully.");
}

TEST_F(LoggingErrorReporterTests, ReportWinErrorWithNotFoundLogsMessage)
{
    reporter.ReportWinError(ERROR_FILE_NOT_FOUND, "CreateFileW");
    Check("WIN32 CreateFileW: (0x00000002) The system cannot find the file specified.");
}

TEST_F(LoggingErrorReporterTests, NtStatusSuccessCodeLogsMessage)
{
    reporter.ReportNtError(STATUS_SUCCESS, "NtCreateFile");
    Check("NTSTATUS NtCreateFile: (0x00000000) The operation completed successfully.");
}

TEST_F(LoggingErrorReporterTests, NtStatusFailureCodeLogsMessage)
{
    reporter.ReportNtError(STATUS_ACCESS_DENIED, "NtCreateFile");
    Check("NTSTATUS NtCreateFile: (0xC0000022) Access is denied.");
}

TEST_F(LoggingErrorReporterTests, HResultSuccessCodeLogsMessage)
{
    ::SetErrorInfo(0, nullptr);
    reporter.ReportHresult(S_OK, "IShellLink::Resolve");
    Check("HRESULT IShellLink::Resolve: (0x00000000) The operation completed successfully.");
}

TEST_F(LoggingErrorReporterTests, HResultFailureCodeLogsMessage)
{
    ::SetErrorInfo(0, nullptr);
    reporter.ReportHresult(E_NOINTERFACE, "IShellLink::QueryInterface");
    Check("HRESULT IShellLink::QueryInterface: (0x80004002) No such interface supported");
}

TEST_F(LoggingErrorReporterTests, GenericErrorLogsMessage)
{
    reporter.ReportGenericError("This is a generic error not associated with an API call.");
    Check("ERROR: This is a generic error not associated with an API call.");
}

struct ThrowingErrorReporterTests : public ::testing::Test
{
    ThrowingErrorReporter reporter;
};

TEST_F(ThrowingErrorReporterTests, Win32Throws)
{
    ASSERT_THROW(reporter.ReportWinError(ERROR_FILE_NOT_FOUND, "FindFirstFileExW"), ErrorFileNotFoundException);
}

TEST_F(ThrowingErrorReporterTests, NtErrorThrows)
{
    ASSERT_THROW(reporter.ReportNtError(STATUS_OBJECT_NAME_NOT_FOUND, "NtQueryDirectoryFile"), ErrorFileNotFoundException);
}

TEST_F(ThrowingErrorReporterTests, HResultThrows)
{
    ASSERT_THROW(reporter.ReportHresult(E_NOINTERFACE, "IShellLink::QueryInterface"), Win32Exception);
}

TEST_F(ThrowingErrorReporterTests, GenericErrorThrows)
{
    ASSERT_THROW(reporter.ReportGenericError("This is a generic error message."), std::exception);
}
