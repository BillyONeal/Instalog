// Copyright © Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <boost/noncopyable.hpp>
#include <string>
#include <memory>
#include <vector>
#include <windows.h>
#include "Win32Exception.hpp"
#include "Library.hpp"

namespace Instalog
{
namespace SystemFacades
{

/// @brief    Event log entry interface
struct EventLogEntry
{
    /// @summary    The date and time that the event was logged
    FILETIME date;

    /// @brief    Different log event severities
    typedef enum _EVT_LEVEL
    {
        EvtLevelCritical = 1,
        EvtLevelError = 2,
        EvtLevelWarning = 3,
        EvtLevelInformation = 4
    } EVT_LEVELS;

    /// @summary    The severity according to EVT_LEVELS
    WORD level;

    /// @summary    The event id
    DWORD eventId;

    /// @brief    Default constructor.
    EventLogEntry();

    /// @brief    Move constructor.
    ///
    /// @param [in,out]    e    The EventLogEntry to process.
    EventLogEntry(EventLogEntry&& e);

    /// @brief    Destructor.
    virtual ~EventLogEntry();

    /// @brief    Gets the source name of the creator of the log entry
    ///
    /// @return    The source.
    virtual std::string GetSource() = 0;

    /// @brief    Gets the description of the event
    ///
    /// @return    The description.
    virtual std::string GetDescription() = 0;
};

class OldEventLogEntry : public EventLogEntry
{
    DWORD eventIdWithExtras;
    std::string source;
    std::vector<std::wstring> strings;
    std::wstring dataString;

    public:
    /// @brief    Constructor from an event record structure
    ///
    /// @param    pRecord    The record.
    OldEventLogEntry(PEVENTLOGRECORD pRecord);

    /// @brief    Move constructor.
    ///
    /// @param [in,out]    e    The OldEventLogEntry to process.
    OldEventLogEntry(OldEventLogEntry&& e);

    /// @brief    Gets the source name of the creator of the log entry
    ///
    /// @return    The source.
    virtual std::string GetSource() override;
    /// @brief    Gets the description of the event
    ///
    /// @return    The description.
    virtual std::string GetDescription() override;
};

class XmlEventLogEntry : boost::noncopyable, public EventLogEntry
{
    HANDLE eventHandle;
    std::wstring providerName;

    /// @brief    Formats messages according to the provided flag
    ///
    /// @param    messageFlag    The message flag.
    ///
    /// @return    The formatted message or "" if the message didn't exist
    std::string FormatEventMessage(DWORD messageFlag);

    public:
    /// @brief    Constructor for an event from a handle to the event
    ///
    /// @param    handle    Handle to the event
    XmlEventLogEntry(HANDLE handle);

    /// @brief    Move constructor.
    ///
    /// @param [in,out]    x    The XmlEventLogEntry to process.
    XmlEventLogEntry(XmlEventLogEntry&& x);

    /// @brief    Move assignment operator.
    ///
    /// @param [in,out]    x    The XmlEventLogEntry to process.
    ///
    /// @return    A shallow copy of this instance.
    XmlEventLogEntry& operator=(XmlEventLogEntry&& x);

    /// @brief    Destructor, frees the handle
    ~XmlEventLogEntry();

    /// @brief    Gets the source name of the creator of the log entry
    ///
    /// @return    The source.
    virtual std::string GetSource();

    /// @brief    Gets the description of the event
    ///
    /// @return    The description.
    virtual std::string GetDescription();
};

/// @brief    Event log interface
struct EventLog : boost::noncopyable
{
    /// @brief    Destructor.
    virtual ~EventLog();

    /// @brief    Reads the applicable events
    ///
    /// @return    The events.
    virtual std::vector<std::unique_ptr<EventLogEntry>> ReadEvents() = 0;
};

/// @brief    Wrapper around the old Win32 event log
class OldEventLog : public EventLog
{
    HANDLE handle;

    public:
    /// @brief    Constructor.
    ///
    /// @param    sourceName    (optional) name of the log source.
    OldEventLog(std::string sourceName = "System");

    /// @brief    Destructor, frees the handle
    ~OldEventLog();

    /// @brief    Reads the applicable events
    ///
    /// @return    The events.
    std::vector<std::unique_ptr<EventLogEntry>> ReadEvents();
};

/// @brief    Wrapper around the new (Vista and later) XML Win32 event log
class XmlEventLog : public EventLog
{
    HANDLE queryHandle;

    public:
    /// @brief    Constructor.
    ///
    /// @param    logPath    (optional) full pathname of the log file.
    /// @param    query      (optional) the query.
    ///
    /// @throws FileNotFoundException on incompatible machines
    XmlEventLog(char* logPath = "System", char* query = "Event/System");

    /// @brief    Destructor, frees the handle
    ~XmlEventLog();

    /// @brief    Reads the applicable events
    ///
    /// @return    The events.
    std::vector<std::unique_ptr<EventLogEntry>> ReadEvents();
};
}
}