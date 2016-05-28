add_executable(Instalog
    Instalog.rc
    Main.cpp
    resource.h
    SupportedOS.manifest
    )

target_link_libraries(Instalog LogCommon)
