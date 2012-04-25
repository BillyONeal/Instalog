call bootstrap.bat
b2 -j4 --with-locale address-model=32 runtime-link=static link=static --stagedir=stage32
b2 -j4 --with-locale address-model=64 runtime-link=static link=static --stagedir=stage64
del /f /s /q bin.v2
