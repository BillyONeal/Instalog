set(CMAKE_EXE_LINKER_FLAGS /manifestinput:\"${CMAKE_CURRENT_LIST_DIR}/SupportedOS.manifest\" )

add_executable(Instalog
    Instalog.rc
    Main.cpp
    resource.h
    )

target_link_libraries(Instalog LogCommon)
