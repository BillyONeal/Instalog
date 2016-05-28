cmake_minimum_required(VERSION 3.4)
set(CMAKE_USER_MAKE_RULES_OVERRIDE
    ${CMAKE_CURRENT_SOURCE_DIR}/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
    ${CMAKE_CURRENT_SOURCE_DIR}/cxx_flag_overrides.cmake)
project (Instalog)
include_directories(ThirdParty/utf8_v2_3_4)

find_package(Boost)
include_directories(${Boost_INCLUDE_DIRS})

set(InstalogMajorVersion 0)
set(InstalogMinorVersion 0)
set(InstalogRevision 8)
set(InstalogBuild 0)

add_definitions(-DINSTALOG_VERSION=${InstalogMajorVersion}.${InstalogMinorVersion}.${InstalogRevision}.${InstalogBuild})
add_definitions(-DINSTALOG_RC_VERSION=${InstalogMajorVersion},${InstalogMinorVersion},${InstalogRevision},${InstalogBuild})

add_definitions(-D_SCL_SECURE_NO_WARNINGS)
add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
add_definitions(-D_WINSOCK_DEPRECATED_NO_WARNINGS)
add_definitions(-DNOMINMAX)
add_definitions(-DNTDDI_VERSION=0x05010200)
add_definitions(-D_WIN32_WINNT=0x0501)
add_definitions(-DBUILD_WINDOWS)

add_compile_options(/MP /GR- /W4 /EHsc)

add_subdirectory(LogCommon)
add_subdirectory(LogTests)
add_subdirectory(Instalog)
