// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "stdafx.h"
#include <vector>
#include <string>
#include "../LogCommon/Scripting.hpp"

using namespace Instalog;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

struct TestingSectionDefinition : public ISectionDefinition
{
    virtual void Execute(std::wostream& logOutput, ScriptSection const& section, std::vector<std::wstring> const& vect) const
    {
        std::wstring vectWritten;
        if (vect.size())
        {
            std::size_t size = vect.size() * 3;
            for (auto it = vect.cbegin(); it != vect.cend(); ++it)
            {
                size += it->size();
            }
            vectWritten.reserve(size);
            vectWritten.assign(L"{");
            vectWritten.append(vect[0]);
            for (std::size_t idx = 1; idx < vect.size(); ++idx)
            {
                vectWritten.append(L"}\n{").append(vect[idx]);
            }
            vectWritten.append(L"}");
        }

        logOutput << section.GetDefinition().GetName() << L" section has.GetArgument() \"" << section.GetArgument() << L"\" and options \n" << vectWritten << std::endl;
    }
};

struct OneSectionDefinition : public TestingSectionDefinition
{
	virtual std::wstring GetScriptCommand() const
	{
		return L"one";
	}
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
	virtual std::wstring GetScriptCommand() const
	{
		return L"twosies";
	}
    virtual std::wstring GetName() const
    {
        return L"Twosies";
    }
    virtual LogSectionPriorities GetPriority() const
    {
        return DISK_PERSISTENT;
    }
};

TEST_CLASS(ScriptFactoryTest)
{
    ScriptParser dispatcher;
    ISectionDefinition *one;
    ISectionDefinition *two;
public:
    TEST_METHOD_INITIALIZE(SetUp)
    {
        std::unique_ptr<ISectionDefinition> oneTemp(new OneSectionDefinition);
        one = oneTemp.get();
        std::unique_ptr<ISectionDefinition> twoTemp(new TwoSectionDefinition);
        two = twoTemp.get();
        dispatcher.AddSectionDefinition(std::move(oneTemp));
        dispatcher.AddSectionDefinition(std::move(twoTemp));
    }

	TEST_METHOD(StartingWhitespace)
	{
		const wchar_t example[] = L"\r\n"
			L":one\r\n"
			L":twosies";
		Script s(dispatcher.Parse(example));
		Assert::AreEqual<std::size_t>(2, s.GetSections().size());
		ScriptSection ss(one);
		auto it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"", it->first.GetArgument());
		Assert::IsTrue(it->second.empty());
		ss = ScriptSection(two);
		it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"", it->first.GetArgument());
		Assert::IsTrue(it->second.empty());
	}

	TEST_METHOD(TrailingWhitespace)
	{
		const wchar_t example[] =
			L":one\r\n"
			L":twosies\r\n\r\n\r\n";
		Script s(dispatcher.Parse(example));
		Assert::AreEqual<std::size_t>(2, s.GetSections().size());
		ScriptSection ss(one);
		auto it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"", it->first.GetArgument());
		Assert::IsTrue(it->second.empty());
		ss = ScriptSection(two);
		it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"", it->first.GetArgument());
		Assert::IsTrue(it->second.empty());
	}

	TEST_METHOD(ContainedWhitespace)
	{
		const wchar_t example[] =
			L"\r\n\r\n\r\n:one\r\nexample\nexample2\r\n\r\n"
			L":twosies\r\n\r\n\r\n";
		Script s(dispatcher.Parse(example));
		Assert::AreEqual<std::size_t>(2, s.GetSections().size());
		ScriptSection ss(one);
		auto it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"", it->first.GetArgument());
		std::vector<std::wstring> answer;
		answer.push_back(L"example");
		answer.push_back(L"example2");
		Assert::IsTrue(answer == it->second);
		ss = ScriptSection(two);
		it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"", it->first.GetArgument());
		Assert::IsTrue(it->second.empty());
	}

	TEST_METHOD(ArgumentsParsed)
	{
		const wchar_t example[] =
			L"\r\n\r\n\r\n:one example\nexample2\r\n\r\n"
			L":twosies\r\n\r\n\r\n";
		Script s(dispatcher.Parse(example));
		Assert::AreEqual<std::size_t>(2, s.GetSections().size());
		ScriptSection ss(one, L"example");
		auto it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"example", it->first.GetArgument());
		std::vector<std::wstring> answer;
		answer.push_back(L"example2");
		Assert::IsTrue(answer == it->second);
		ss = ScriptSection(two);
		it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"", it->first.GetArgument());
		Assert::IsTrue(it->second.empty());
	}

	TEST_METHOD(ArgumentsWhitespaceSignificant)
	{
		const wchar_t example[] =
			L"\r\n\r\n\r\n:one    example\nexample2\r\n\r\n"
			L":twosies\r\n\r\n\r\n";
		Script s(dispatcher.Parse(example));
		Assert::AreEqual<std::size_t>(2, s.GetSections().size());
		ScriptSection ss(one, L"   example");
		auto it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"   example", it->first.GetArgument());
		std::vector<std::wstring> answer;
		answer.push_back(L"example2");
		Assert::IsTrue(answer == it->second);
		ss = ScriptSection(two);
		it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"", it->first.GetArgument());
		Assert::IsTrue(it->second.empty());
	}

	TEST_METHOD(TakesSingleArgument)
	{
		const wchar_t example[] =
			L":one";
		Script s(dispatcher.Parse(example));
		Assert::AreEqual<std::size_t>(1, s.GetSections().size());
		ScriptSection ss(one);
		auto it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"", it->first.GetArgument());
	}

	TEST_METHOD(Merges)
	{
		const wchar_t example[] =
			L":one\nexample\nexample2\r\nmerged\r\n"
			L":twosies\r\n\r\n\r\n"
			L":one\nmerged\nmerged2";
		Script s(dispatcher.Parse(example));
		Assert::AreEqual<std::size_t>(2, s.GetSections().size());
		ScriptSection ss(one);
		auto it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		std::vector<std::wstring> answer;
		answer.push_back(L"example");
		answer.push_back(L"example2");
		answer.push_back(L"merged");
		answer.push_back(L"merged");
		answer.push_back(L"merged2");
		Assert::IsTrue(answer == it->second);
		ss = ScriptSection(two);
		it = s.GetSections().find(ss);
		Assert::IsFalse(s.GetSections().end() == it);
		Assert::AreEqual<std::wstring>(L"", it->first.GetArgument());
		Assert::IsTrue(it->second.empty());
	}

	TEST_METHOD(UnknownThrows)
	{
		const wchar_t example[] =
			L":unknown";
		Assert::ExpectException<UnknownScriptSectionException>([&] { Script s(dispatcher.Parse(example)); } );
	}

	TEST_METHOD(EmptyThrows)
	{
		const wchar_t example[] =
			L":";
		Assert::ExpectException<UnknownScriptSectionException>([&] { Script s(dispatcher.Parse(example)); } );
	}
};

TEST_CLASS(ScriptTests)
{
public:
	TEST_METHOD(CanExecute)
	{
		ScriptParser dispatcher;
		ISectionDefinition *one;
		ISectionDefinition *two;
		std::unique_ptr<ISectionDefinition> oneTemp(new OneSectionDefinition);
		one = oneTemp.get();
		std::unique_ptr<ISectionDefinition> twoTemp(new TwoSectionDefinition);
		two = twoTemp.get();
		dispatcher.AddSectionDefinition(std::move(oneTemp));
		dispatcher.AddSectionDefinition(std::move(twoTemp));
		Script s(dispatcher.Parse(L":one argArg\nOptionOne\n:TwOsIeS\nOptionTwo\nOptionThree"));
		std::unique_ptr<IUserInterface> ui(new DoNothingUserInterface);
		std::wostringstream logOutput;
		s.Run(logOutput, ui.get());
		std::wstring out(logOutput.str());
		out.pop_back(); // \n
		out.erase(std::find(out.rbegin(), out.rend(), L'\n').base(), out.end());
		out.erase(out.begin(), std::find(out.begin(), out.end(), L'='));
		out.pop_back();
		Assert::AreEqual<std::wstring>(L"======================= OnE ======================\n\nOnE "
			L"section has.GetArgument() \"argArg\" and options \n{OptionOne}\n\n"
			L"===================== Twosies ====================\n\nTwosies "
			L"section has.GetArgument() \"\" and options \n{OptionTwo}\n{OptionThree}\n",
			out);
	}
};
