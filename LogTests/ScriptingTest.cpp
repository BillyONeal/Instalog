#include "pch.hpp"
#include <vector>
#include <string>
#include "gtest/gtest.h"
#include "LogCommon/Scripting.hpp"

using namespace Instalog;

struct TestingSectionDefinition : public ISectionDefinition
{
	std::wstring argumentSupplied;
	std::vector<std::wstring> optionsSupplied;
	virtual void Execute(std::ostream& logOutput, IUserInterface *ui, ScriptSection const& sectionData, std::vector<std::wstring> const& options)
	{
		argumentSupplied = sectionData.argument;
		optionsSupplied = options;
	}
};

struct OneSectionDefinition : public TestingSectionDefinition
{
	virtual std::wstring GetName() const
	{
		return L"OnE";
	}
	virtual LogSectionPriorities GetPriority() const
	{
		return MEMORY;
	}
};

struct TwoSectionDefinition : public TestingSectionDefinition
{
	virtual std::wstring GetName() const
	{
		return L"Twosies";
	}
	virtual LogSectionPriorities GetPriority() const
	{
		return DISK_PERSISTENT;
	}
};

struct ScriptDispatcherTest : public ::testing::Test
{
	ScriptDispatcher dispatcher;
	ISectionDefinition *one;
	ISectionDefinition *two;
	virtual void SetUp()
	{
		std::unique_ptr<ISectionDefinition> oneTemp(new OneSectionDefinition);
		one = oneTemp.get();
		std::unique_ptr<ISectionDefinition> twoTemp(new TwoSectionDefinition);
		two = twoTemp.get();
		dispatcher.AddSectionType(std::move(oneTemp));
		dispatcher.AddSectionType(std::move(twoTemp));
	}

};

TEST_F(ScriptDispatcherTest, StartingWhitespace)
{
	const wchar_t example[] = L"\r\n"
		L":one\r\n"
		L":twosies";
	Script s(dispatcher.Parse(example));
	ASSERT_EQ(2, s.GetSections().size());
	ScriptSection ss;
	ss.targetSection = one;
	auto it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"", it->first.argument);
	ASSERT_TRUE(it->second.empty());
	ss.targetSection = two;
	it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"", it->first.argument);
	ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptDispatcherTest, TrailingWhitespace)
{
	const wchar_t example[] =
		L":one\r\n"
		L":twosies\r\n\r\n\r\n";
	Script s(dispatcher.Parse(example));
	ASSERT_EQ(2, s.GetSections().size());
	ScriptSection ss;
	ss.targetSection = one;
	auto it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"", it->first.argument);
	ASSERT_TRUE(it->second.empty());
	ss.targetSection = two;
	it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"", it->first.argument);
	ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptDispatcherTest, ContainedWhitespace)
{
	const wchar_t example[] =
		L"\r\n\r\n\r\n:one\r\nexample\nexample2\r\n\r\n"
		L":twosies\r\n\r\n\r\n";
	Script s(dispatcher.Parse(example));
	ASSERT_EQ(2, s.GetSections().size());
	ScriptSection ss;
	ss.targetSection = one;
	auto it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"", it->first.argument);
	std::vector<std::wstring> answer;
	answer.push_back(L"example");
	answer.push_back(L"example2");
	ASSERT_EQ(answer, it->second);
	ss.targetSection = two;
	it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"", it->first.argument);
	ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptDispatcherTest, ArgumentsParsed)
{
	const wchar_t example[] =
		L"\r\n\r\n\r\n:one example\nexample2\r\n\r\n"
		L":twosies\r\n\r\n\r\n";
	Script s(dispatcher.Parse(example));
	ASSERT_EQ(2, s.GetSections().size());
	ScriptSection ss;
	ss.targetSection = one;
	ss.argument = L"example";
	auto it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"example", it->first.argument);
	std::vector<std::wstring> answer;
	answer.push_back(L"example2");
	ASSERT_EQ(answer, it->second);
	ss.targetSection = two;
	ss.argument.clear();
	it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"", it->first.argument);
	ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptDispatcherTest, ArgumentsWhitespaceSignificant)
{
	const wchar_t example[] =
		L"\r\n\r\n\r\n:one    example\nexample2\r\n\r\n"
		L":twosies\r\n\r\n\r\n";
	Script s(dispatcher.Parse(example));
	ASSERT_EQ(2, s.GetSections().size());
	ScriptSection ss;
	ss.targetSection = one;
	ss.argument = L"   example";
	auto it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"   example", it->first.argument);
	std::vector<std::wstring> answer;
	answer.push_back(L"example2");
	ASSERT_EQ(answer, it->second);
	ss.targetSection = two;
	ss.argument.clear();
	it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"", it->first.argument);
	ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptDispatcherTest, TakesSingleArgument)
{
	const wchar_t example[] =
		L":one";
	Script s(dispatcher.Parse(example));
	ASSERT_EQ(1, s.GetSections().size());
	ScriptSection ss;
	ss.targetSection = one;
	auto it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"", it->first.argument);
}

TEST_F(ScriptDispatcherTest, Merges)
{
	const wchar_t example[] =
		L":one\nexample\nexample2\r\nmerged\r\n"
		L":twosies\r\n\r\n\r\n"
		L":one\nmerged\nmerged2";
	Script s(dispatcher.Parse(example));
	ASSERT_EQ(2, s.GetSections().size());
	ScriptSection ss;
	ss.targetSection = one;
	auto it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	std::vector<std::wstring> answer;
	answer.push_back(L"example");
	answer.push_back(L"example2");
	answer.push_back(L"merged");
	answer.push_back(L"merged");
	answer.push_back(L"merged2");
	ASSERT_EQ(answer, it->second);
	ss.targetSection = two;
	it = s.GetSections().find(ss);
	ASSERT_NE(s.GetSections().end(), it);
	ASSERT_EQ(L"", it->first.argument);
	ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptDispatcherTest, UnknownThrows)
{
	const wchar_t example[] =
		L":unknown";
	EXPECT_THROW(Script s(dispatcher.Parse(example)), UnknownScriptSectionException);
}

TEST_F(ScriptDispatcherTest, EmptyThrows)
{
	const wchar_t example[] =
		L":";
	EXPECT_THROW(Script s(dispatcher.Parse(example)), UnknownScriptSectionException);
}
