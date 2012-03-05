#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <windows.h>

int wmain(int argc, wchar_t *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: inputFile outputFile";
		return -1;
	}
	argc--; argv++;
	std::wifstream input(argv[0]);
	std::ofstream output(argv[1], std::ios::out | std::ios::binary | std::ios::trunc);

	std::vector<std::wstring> lines;
	std::wstring line;
	while(std::getline(input, line))
	{
		lines.emplace_back(line);
	}
	std::sort(lines.begin(), lines.end());

	union
	{
		unsigned __int32 intPart;
		char charPart[4];
	} intConverter;
	intConverter.intPart = static_cast<unsigned __int32>(lines.size());
	output.write(&intConverter.charPart[0], sizeof(intConverter));
	for (std::size_t idx = 0; idx < lines.size(); ++idx)
	{
		intConverter.intPart = static_cast<unsigned __int32>(lines[idx].size());
		output.write(&intConverter.charPart[0], sizeof(intConverter));
		const char* str = reinterpret_cast<const char*>(lines[idx].c_str());
		output.write(str, lines[idx].size() * sizeof(wchar_t));
	}
}
