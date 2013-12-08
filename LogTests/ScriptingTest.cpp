// Copyright © 2012-2013 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include <vector>
#include <string>
#include "gtest/gtest.h"
#include "../LogCommon/Scripting.hpp"

using namespace Instalog;

struct TestingSectionDefinition : public ISectionDefinition
{
    virtual void Execute(log_sink& logOutput,
                         ScriptSection const& section,
                         std::vector<std::string> const& vect) const
    {
        std::string vectWritten;
        if (vect.size())
        {
            std::size_t size = vect.size() * 3;
            for (auto it = vect.cbegin(); it != vect.cend(); ++it)
            {
                size += it->size();
            }
            vectWritten.reserve(size);
            vectWritten.assign("{");
            vectWritten.append(vect[0]);
            for (std::size_t idx = 1; idx < vect.size(); ++idx)
            {
                vectWritten.append("}\r\n{").append(vect[idx]);
            }
            vectWritten.append("}");
        }

        writeln(logOutput, section.GetDefinition().GetName(), " section has.GetArgument() \"", section.GetArgument(), "\" and options \r\n", vectWritten);
    }
};

struct OneSectionDefinition : public TestingSectionDefinition
{
    virtual std::string GetScriptCommand() const
    {
        return "one";
    }
    virtual std::string GetName() const
    {
        return "OnE";
    }
    virtual LogSectionPriorities GetPriority() const
    {
        return MEMORY;
    }
};

struct TwoSectionDefinition : public TestingSectionDefinition
{
    virtual std::string GetScriptCommand() const
    {
        return "twosies";
    }
    virtual std::string GetName() const
    {
        return "Twosies";
    }
    virtual LogSectionPriorities GetPriority() const
    {
        return DISK_PERSISTENT;
    }
};

struct ScriptFactoryTest : public ::testing::Test
{
    ScriptParser dispatcher;
    ISectionDefinition* one;
    ISectionDefinition* two;
    virtual void SetUp()
    {
        std::unique_ptr<ISectionDefinition> oneTemp(new OneSectionDefinition);
        one = oneTemp.get();
        std::unique_ptr<ISectionDefinition> twoTemp(new TwoSectionDefinition);
        two = twoTemp.get();
        dispatcher.AddSectionDefinition(std::move(oneTemp));
        dispatcher.AddSectionDefinition(std::move(twoTemp));
    }
};

TEST_F(ScriptFactoryTest, StartingWhitespace)
{
    const char example[] = "\r\n"
        ":one\r\n"
        ":twosies";
    Script s(dispatcher.Parse(example));
    ASSERT_EQ(2, s.GetSections().size());
    ScriptSection ss(one);
    auto it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("", it->first.GetArgument());
    ASSERT_TRUE(it->second.empty());
    ss = ScriptSection(two);
    it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("", it->first.GetArgument());
    ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptFactoryTest, TrailingWhitespace)
{
    const char example[] =
        ":one\r\n"
        ":twosies\r\n\r\n\r\n";
    Script s(dispatcher.Parse(example));
    ASSERT_EQ(2, s.GetSections().size());
    ScriptSection ss(one);
    auto it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("", it->first.GetArgument());
    ASSERT_TRUE(it->second.empty());
    ss = ScriptSection(two);
    it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("", it->first.GetArgument());
    ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptFactoryTest, ContainedWhitespace)
{
    const char example[] =
        "\r\n\r\n\r\n:one\r\nexample\nexample2\r\n\r\n"
        ":twosies\r\n\r\n\r\n";
    Script s(dispatcher.Parse(example));
    ASSERT_EQ(2, s.GetSections().size());
    ScriptSection ss(one);
    auto it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("", it->first.GetArgument());
    std::vector<std::string> answer;
    answer.emplace_back("example");
    answer.emplace_back("example2");
    ASSERT_EQ(answer, it->second);
    ss = ScriptSection(two);
    it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("", it->first.GetArgument());
    ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptFactoryTest, ArgumentsParsed)
{
    const char example[] =
        "\r\n\r\n\r\n:one example\nexample2\r\n\r\n"
        ":twosies\r\n\r\n\r\n";
    Script s(dispatcher.Parse(example));
    ASSERT_EQ(2, s.GetSections().size());
    ScriptSection ss(one, "example");
    auto it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("example", it->first.GetArgument());
    std::vector<std::string> answer;
    answer.emplace_back("example2");
    ASSERT_EQ(answer, it->second);
    ss = ScriptSection(two);
    it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("", it->first.GetArgument());
    ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptFactoryTest, ArgumentsWhitespaceSignificant)
{
    const char example[] =
        "\r\n\r\n\r\n:one    example\nexample2\r\n\r\n"
        ":twosies\r\n\r\n\r\n";
    Script s(dispatcher.Parse(example));
    ASSERT_EQ(2, s.GetSections().size());
    ScriptSection ss(one, "   example");
    auto it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("   example", it->first.GetArgument());
    std::vector<std::string> answer;
    answer.emplace_back("example2");
    ASSERT_EQ(answer, it->second);
    ss = ScriptSection(two);
    it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("", it->first.GetArgument());
    ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptFactoryTest, TakesSingleArgument)
{
    const char example[] =
        ":one";
    Script s(dispatcher.Parse(example));
    ASSERT_EQ(1, s.GetSections().size());
    ScriptSection ss(one);
    auto it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("", it->first.GetArgument());
}

TEST_F(ScriptFactoryTest, Merges)
{
    const char example[] =
        ":one\nexample\nexample2\r\nmerged\r\n"
        ":twosies\r\n\r\n\r\n"
        ":one\nmerged\nmerged2";
    Script s(dispatcher.Parse(example));
    ASSERT_EQ(2, s.GetSections().size());
    ScriptSection ss(one);
    auto it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    std::vector<std::string> answer;
    answer.emplace_back("example");
    answer.emplace_back("example2");
    answer.emplace_back("merged");
    answer.emplace_back("merged");
    answer.emplace_back("merged2");
    ASSERT_EQ(answer, it->second);
    ss = ScriptSection(two);
    it = s.GetSections().find(ss);
    ASSERT_NE(s.GetSections().end(), it);
    ASSERT_EQ("", it->first.GetArgument());
    ASSERT_TRUE(it->second.empty());
}

TEST_F(ScriptFactoryTest, UnknownThrows)
{
    const char example[] = ":unknown";
    EXPECT_THROW(Script s(dispatcher.Parse(example)),
                 UnknownScriptSectionException);
}

TEST_F(ScriptFactoryTest, EmptyThrows)
{
    const char example[] = ":";
    EXPECT_THROW(Script s(dispatcher.Parse(example)),
                 UnknownScriptSectionException);
}

TEST(ScriptTest, CanExecute)
{
    ScriptParser dispatcher;
    ISectionDefinition* one;
    ISectionDefinition* two;
    std::unique_ptr<ISectionDefinition> oneTemp(new OneSectionDefinition);
    one = oneTemp.get();
    std::unique_ptr<ISectionDefinition> twoTemp(new TwoSectionDefinition);
    two = twoTemp.get();
    dispatcher.AddSectionDefinition(std::move(oneTemp));
    dispatcher.AddSectionDefinition(std::move(twoTemp));
    Script s(dispatcher.Parse(
        ":one argArg\nOptionOne\n:TwOsIeS\nOptionTwo\nOptionThree"));
    std::unique_ptr<IUserInterface> ui(new DoNothingUserInterface);
    string_sink outSink;
    s.Run(outSink, ui.get());
    std::string out(outSink.get());
    out.pop_back(); // \n
    out.erase(std::find(out.rbegin(), out.rend(), '\n').base(), out.end());
    out.erase(out.begin(), std::find(out.begin(), out.end(), '='));
    out.pop_back();
    ASSERT_STREQ("======================= OnE ======================\n\nOnE "
        "section has.GetArgument() \"argArg\" and options \n{OptionOne}\n\n"
        "===================== Twosies ====================\n\nTwosies "
        "section has.GetArgument() \"\" and options \n{OptionTwo}\n{OptionThree}\n",
        out.c_str());
}
