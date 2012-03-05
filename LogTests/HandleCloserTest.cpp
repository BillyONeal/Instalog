#include "pch.hpp"
#include <LogCommon/File.hpp>
#include <LogCommon/HandleCloser.hpp>

TEST(HandleCloser, DoesCloseHandle)
{
	HANDLE hFile = ::CreateFileW(L"IDoNotExist.txt", GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, nullptr);
	Instalog::SystemFacades::HandleCloser h;
	h(hFile);
	ASSERT_FALSE(Instalog::SystemFacades::File::Exists(L"IDoNotExist.txt"));
}
