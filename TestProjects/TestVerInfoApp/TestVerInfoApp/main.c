#include <windows.h>

void entryPoint()
{
    DWORD useless = 0;
    wchar_t const message[] = L"This is a test program. It exists for its"
                              L" VS_VERSION_INFO resource.";
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), message,
                    (sizeof(message) / sizeof(wchar_t)) - 1, &useless, 0);
}
