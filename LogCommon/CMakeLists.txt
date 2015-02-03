add_library(LogCommon STATIC
    ../ThirdParty/sqlite-amalgamation/sqlite3.h
    ../ThirdParty/sqlite-amalgamation/sqlite3.c
    Com.cpp
    Com.hpp
    DdkStructures.h
    Dns.cpp
    Dns.hpp
    EnumClassOperators.hpp
    ErrorReporter.cpp
    ErrorReporter.hpp
    EventLog.cpp
    EventLog.hpp
    Expected.hpp
    File.cpp
    File.hpp
    Library.cpp
    Library.hpp
    LoadPointsReport.cpp
    LoadPointsReport.hpp
    LogAlgorithm.hpp
    LogSink.cpp
    LogSink.hpp
    LogSink_Windows.cpp
    OptimisticBuffer.hpp
    Path.cpp
    Path.hpp
    Process.cpp
    Process.hpp
    Registry.cpp
    Registry.hpp
    RestorePoints.cpp
    RestorePoints.hpp
    ScanningSections.cpp
    ScanningSections.hpp
    ScopedPrivilege.hpp
    ScopeExit.hpp
    Scripting.cpp
    Scripting.hpp
    SecurityCenter.cpp
    SecurityCenter.hpp
    ServiceControlManager.cpp
    ServiceControlManager.hpp
    StockOutputFormats.cpp
    StockOutputFormats.hpp
    StringUtilities.cpp
    StringUtilities.hpp
    UserInterface.hpp
    Utf8.hpp
    Win32Exception.cpp
    Win32Exception.hpp
    Win32Glue.cpp
    Win32Glue.hpp
    Wmi.cpp
    Wmi.hpp
    Wow64.hpp
)

set(SQLITE_COMMAND ${CMAKE_SOURCE_DIR}/ThirdParty/sqlite-amalgamation/sqlite3.exe CACHE STRING "SQLite Executable Path")
set(WHITELIST_SQL_FILE ${CMAKE_CURRENT_LIST_DIR}/Whitelist.sql)
set(WHITELIST_DB_FILE ${CMAKE_CURRENT_BINARY_DIR}/Whitelist.db)

add_custom_command(
    OUTPUT ${WHITELIST_DB_FILE}
    COMMAND ${SQLITE_COMMAND} ${WHITELIST_DB_FILE} -init ${WHITELIST_SQL_FILE} ".exit"
    MAIN_DEPENDENCY ${WHITELIST_SQL_FILE}
    )

add_custom_target(BuildWhitelist
    DEPENDS ${WHITELIST_SQL_FILE} ${WHITELIST_DB_FILE}
    )

add_dependencies(LogCommon BuildWhitelist)

