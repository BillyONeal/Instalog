// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include <vector>
#include <string>
#include <numeric>
#include "gtest/gtest.h"
#include "../LogCommon/Scripting.hpp"

using namespace Instalog;

struct TestingSectionDefinition : public ISectionDefinition
{
    virtual void Execute(ExecutionOptions options) const override
    {
        std::string vectWritten;
        std::vector<std::string> const& scriptLines = options.GetOptions();
        if (!scriptLines.empty())
        {
            std::size_t size = std::accumulate(scriptLines.cbegin(), scriptLines.cend(), scriptLines.size() * 3,
                [](std::size_t val, std::string const& x) { return val + x.size(); });
            vectWritten.reserve(size);
            vectWritten.assign("{");
            vectWritten.append(scriptLines[0]);
            for (std::size_t idx = 1; idx < scriptLines.size(); ++idx)
            {
                vectWritten.append("}\r\n{").append(scriptLines[idx]);
            }
            vectWritten.append("}");
        }

        writeln(options.GetOutput(), options.GetSectionData().GetDefinition().GetName(), " section has.GetArgument() \"",
            options.GetSectionData().GetArgument(), "\" and options \r\n", vectWritten);
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
    out.pop_back(); // \r
    out.erase(std::find(out.rbegin(), out.rend(), '\r').base(), out.end());
    out.erase(out.begin(), std::find(out.begin(), out.end(), '='));
    out.pop_back();
    ASSERT_STREQ("======================= OnE ======================\r\n\r\nOnE "
        "section has.GetArgument() \"argArg\" and options \r\n{OptionOne}\r\n\r\n"
        "===================== Twosies ====================\r\n\r\nTwosies "
        "section has.GetArgument() \"\" and options \r\n{OptionTwo}\r\n{OptionThree}\r\n",
        out.c_str());
}
