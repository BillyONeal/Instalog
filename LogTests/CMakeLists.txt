add_executable(LogTests
    gtest/gtest.h
    DnsTest.cpp
    ErrorReporterTest.cpp
    EventLogTest.cpp
    ExpectedTest.cpp
    FileTest.cpp
    gtest-all.cc
    gtest_main.cc
    LibraryTest.cpp
    LogAlgorithmTest.cpp
    LogSinkTest.cpp
    PathTest.cpp
    ProcessTest.cpp
    RegistryTest.cpp
    ScanningSectionsTest.cpp
    ScriptingTest.cpp
    ServiceControlManagerTest.cpp
    StockOutputFormatsTest.cpp
    StringUtilitiesTest.cpp
    TestSupport.hpp
    Win32ExceptionTest.cpp
    Win32GlueTest.cpp
)

target_link_libraries(LogTests LogCommon)

file(COPY TestData/TestVerInfoApp.exe DESTINATION TestData)
