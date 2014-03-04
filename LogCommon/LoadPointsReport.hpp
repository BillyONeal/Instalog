// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "Scripting.hpp"
#include "LogSink.hpp"

namespace Instalog
{

/// @brief    Pseudo HijackThis report generator.
class LoadPointsReport : public ISectionDefinition
{
    virtual std::string GetScriptCommand() const;
    virtual std::string GetName() const;
    virtual LogSectionPriorities GetPriority() const;

    virtual void Execute(ExecutionOptions options) const;
};
}
