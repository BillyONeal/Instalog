// Copyright © 2012 Jacob Snyder, Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <string>
#include <memory>
#include <windows.h>
#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include "DdkStructures.h"

namespace Instalog { namespace SystemFacades {

    /// @brief    Exception for signaling invalid registry data type. This occurs
    ///         when a client asks for a data type that does not match the data
    ///         type in the registry itself.
    struct InvalidRegistryDataTypeException : public std::exception
    {
        virtual char const* what() const
        {
            return "Invalid Registry Value Type";
        }
    };

    /// @brief    Base registry value class. Implements functionality common to
    ///         all registry value implementations.
    class BasicRegistryValue
    {
    protected:
        BasicRegistryValue() {} // Not intended for client construction.
        ~BasicRegistryValue() {}
    public:
        /// @brief    Gets an iterator to the beginning of the data in this value.
        virtual unsigned char const* cbegin() const = 0;

        /// @brief    Gets an iterator to the range end of the data in this value.
        virtual unsigned char const* cend() const = 0;

        /// @brief    Same as cbegin, casted to wchar_t (instead of char)
        wchar_t const* wcbegin() const;

        /// @brief    Same as cend, casted to wchar_t (instead of char)
        wchar_t const* wcend() const;

        /// @brief    Gets the size of the data in the indicated registry value.
        virtual std::size_t size() const = 0;

        /// @brief    Checks whether or not this registry value contains no data.
        bool empty() const { return size() == 0; }

        /// @brief    Gets the type of this registry value.
        ///
        /// @remarks These value types are formally documented on MSDN at
        ///          http://msdn.microsoft.com/en-us/library/windows/desktop/ms724884.aspx
        ///
        /// @return    The type of registry value.
        virtual DWORD GetType() const = 0;

        /// @brief    Attempts to interpret the data in this value as a DWORD value.
        ///
        /// @remarks This function will attempt to convert the DWORD value from
        ///          a string, for instance a value of L"1" in the registry will
        ///          be returned as the integer 1. If this behavior is undesired,
        ///          use GetDWordStrict instead.
        ///          
        /// @throws InvalidRegistryDataTypeException
        ///         Thrown in the event the data cannot be interpreted as a DWORD.
        /// 
        /// @return    The value reinterpreted as a DWORD.
        DWORD GetDWord() const;

        /// @brief    Interprets the data in this value as a DWORD, without
        ///         conversions.
        ///         
        /// @remarks This function is the same as GetDWord, except operates only
        ///          on REG_DWORD and REG_DWORD_BIG_ENDIAN registry value types.
        ///
        /// @throws InvalidRegistryDataTypeException
        ///         Thrown in the event the data is not REG_DWORD or
        ///         REG_DWORD_BIG_ENDIAN.
        /// 
        /// @return    The the value reinterpreted as a DWORD. (Little Endian)
        DWORD GetDWordStrict() const;

        /// @brief    Attempts to interpret the data in this value as a QWORD value.
        ///
        /// @remarks This function will attempt to convert the QWORD value from
        ///          a string, for instance a value of L"1" in the registry will
        ///          be returned as the integer 1. If this behavior is undesired,
        ///          use GetDWordStrict instead.
        /// 
        /// @throws InvalidRegistryDataTypeException
        ///         Thrown in the event the data cannot be interpreted as a QWORD.
        /// 
        /// @return    The value reinterpreted as a QWORD.
        std::uint64_t GetQWord() const;

        /// @brief    Interprets the data in this value as a QWORD, without
        ///         conversions.
        ///         
        /// @remarks This function is the same as GetQWord, except operates only
        ///          on QWORD registry value types.
        ///
        /// @throws InvalidRegistryDataTypeException
        ///         Thrown in the event the data is not REG_QWORD
        /// 
        /// @return    The the value reinterpreted as a QWORD.
        std::uint64_t GetQWordStrict() const;

        /// @brief    Gets the data in this registry value, converted to a string
        ///         representation.
        ///
        /// @remarks This function will perform format conversions as necessary
        ///          to achieve a reasonable string representation. If this is
        ///          undesired behavior, use GetStringStrict instead, which
        ///          throws instead.
        /// 
        /// @return    The value data interpreted as a string.
        std::wstring GetString() const;

        /// @brief    Gets the data in this value as a string in a strict manner.
        /// 
        /// @remarks This function will not attempt to convert the data in
        ///          in the value. If you wish for conversions, use GetString
        ///          instead.
        /// 
        /// @throws InvalidRegistryDataTypeException
        ///         Thrown in the event the data is not of type REG_SZ or
        ///         REG_EXPAND_SZ.
        ///
        /// @return    The string strict.
        std::wstring GetStringStrict() const;

        /// @brief    Gets the data of this value as a vector of strings.
        ///
        /// @throws InvalidRegistryDataTypeException
        ///         Thrown in the event the value is not of type REG_MULTI_SZ. 
        ///
        /// @return    The multi string array.
        std::vector<std::wstring> GetMultiStringArray() const;

        /// @brief    Gets the data of this value as a vector of strings, assuming
        ///         the data is separated by commas.
        ///
        /// @throws InvalidRegistryDataTypeException
        ///         Thrown in the event the data type is not REG_SZ or
        ///         REG_MULTI_SZ
        /// 
        /// @return    The comma string array.
        std::vector<std::wstring> GetCommaStringArray() const;
    };

    /// @brief    Registry value. An implementation of BasicRegistryValue.
    /// 
    /// @remarks This class is used when registry values are asked for by name.
    class RegistryValue : public BasicRegistryValue
    {
        DWORD type_;
        std::vector<unsigned char> data_;
    public:
        /// @brief    Constructor. Constructs a registry value from a registry type
        ///         and data.
        ///
        /// @param    type            The type.
        /// @param [in,out]    data    The data.
        RegistryValue(DWORD type, std::vector<unsigned char> && data);

        /// @brief    Move constructor.
        ///
        /// @param [in,out]    other    The registry value being moved from.
        RegistryValue(RegistryValue && other);

        /// @brief    Gets the type of data in this registry value.
        ///
        /// @return    The type of data.
        virtual DWORD GetType() const;

        /// @brief    Gets the size of data in this registry value.
        ///
        /// @return    The data size.
        virtual std::size_t size() const;

        /// @brief    Gets an iterator to the beginning of the data in this value.
        virtual unsigned char const* cbegin() const;

        /// @brief    Gets an iterator to the range end of the data in this value.
        virtual unsigned char const* cend() const;
    };

    /// @brief    Registry value and data. An implementation of BasicRegistryValue
    ///         used when value enumeration is performed.
    class RegistryValueAndData : public BasicRegistryValue
    {
        std::vector<unsigned char> innerBuffer_;
        typedef KEY_VALUE_FULL_INFORMATION valueType;
        KEY_VALUE_FULL_INFORMATION const* Cast() const;
    public:
        /// @brief    Constructor. Constructs a RegistryValueAndData from a
        ///         KEY_VALUE_FULL_INFORMATION buffer.
        ///
        /// @param [in,out]    buff    The buffer to construct from.
        RegistryValueAndData(std::vector<unsigned char> && buff);

        /// @brief    Move constructor.
        ///
        /// @param [in,out]    other    The RegistryValueAndData being moved from.
        RegistryValueAndData(RegistryValueAndData && other);

        /// @brief    Assignment operator.
        ///
        /// @param    other    The RegistryValueAndData being assigned from.
        ///
        /// @return    A shallow copy of this instance.
        RegistryValueAndData& operator=(RegistryValueAndData other)
        {
            innerBuffer_ = std::move(other.innerBuffer_);
            return *this;
        }
        std::wstring GetName() const;

        /// @brief    Gets the type of data in this registry value.
        ///
        /// @return    The type of data.
        virtual DWORD GetType() const;

        /// @brief    Gets the size of data in this registry value.
        ///
        /// @return    The data size.
        virtual std::size_t size() const;

        /// @brief    Gets an iterator to the beginning of the data in this value.
        virtual unsigned char const* cbegin() const;

        /// @brief    Gets an iterator to the range end of the data in this value.
        virtual unsigned char const* cend() const;

        /// @brief    Less-than comparison operator. Compares the registry values
        ///         based on their names.
        ///
        /// @param    rhs    The right hand side registry value.
        ///
        /// @return    true if this instance's name is lexicographically less than
        ///         the right hand side instance's name.
        bool operator<(RegistryValueAndData const& rhs) const;
    };

    /// @brief    Information about the registry key size.
    class RegistryKeySizeInformation
    {
        std::uint64_t lastWriteTime_;
        std::uint32_t numberOfSubkeys_;
        std::uint32_t numberOfValues_;
    public:
        /// @brief    Constructor.
        ///
        /// @param    lastWriteTime      Time of the last write to the indicated registry key.
        /// @param    numberOfSubkeys    Number of sub keys of this registry key.
        /// @param    numberOfValues     Number of values in this registry key.
        RegistryKeySizeInformation(std::uint64_t lastWriteTime, std::uint32_t numberOfSubkeys, std::uint32_t numberOfValues);

        /// @brief    Gets the number of subkeys.
        ///
        /// @return    The number of subkeys.
        std::uint32_t GetNumberOfSubkeys() const;

        /// @brief    Gets the number of values.
        ///
        /// @return    The number of values.
        std::uint32_t GetNumberOfValues() const;

        /// @brief    Gets the last write time.
        ///
        /// @return    The last write time.
        std::uint64_t GetLastWriteTime() const;
    };

    /// @brief    Registry key.
    class RegistryKey : boost::noncopyable
    {
        HANDLE hKey_;
    public:
        /// @brief    Default constructor. Constructs an invalid registry key.
        RegistryKey();

        /// @brief    Constructor. Constructs a registry key instance around a
        ///         given handle.
        ///
        /// @param    hKey    Handle of the key.
        explicit RegistryKey(HANDLE hKey);

        /// @brief    Move constructor. Takes ownership of the other key's handle.
        ///
        /// @param [in,out]    other    The other registry key instance.
        RegistryKey(RegistryKey && other);

        /// @brief    Assignment operator.
        ///
        /// @param    other    The other registry key being assigned from.
        ///
        /// @return    A shallow copy of this instance.
        RegistryKey& operator=(RegistryKey other);

        /// @brief    Destructor. Closes the registry key handle contained here.
        ~RegistryKey();

        /// @brief    Gets the raw kernel handle to the registry key.
        ///
        /// @return    The raw key kernel handle.
        HANDLE GetHkey() const;

        /// @brief    Gets a registry value.
        ///
        /// @param    name    The name of the value to retrieve.
        ///
        /// @return    The value.
        RegistryValue const GetValue(std::wstring const& name) const;

        /// @brief    Array indexer operator. Forwards to GetValue()
        ///
        /// @param    name    The name of the value to retrieve.
        ///
        /// @return    The registry value retrieved from the given name.
        RegistryValue const operator[](std::wstring const& name) const;

        /// Sets a registry value.
        /// @param name             The name of the value.
        /// @param dataSize         Size of the data.
        /// @param [in,out] data If non-null, the data.
        /// @param type             The type to which the value shall be set.
        /// @throws Instalog::SystemFacades::Win32Exception on failure
        /// @throws std::out_of_range The parameter dataSize exceeds std::numeric_limits<uint32_t>::max().
        void SetValue(std::wstring const& name, std::size_t dataSize, void const* data, DWORD type);

        /// Sets a registry value.
        /// @param name The value name.
        /// @param data The value data.
        /// @param type The value type.
        /// @throws Instalog::SystemFacades::Win32Exception on failure
        template <typename Ty>
        void SetValue(std::wstring const& name, std::vector<Ty> const& data, DWORD type)
        {
            this->SetValue(name, data.size() * sizeof(Ty), static_cast<void const *>(data.data()), type);
        }

        /// Sets a registry value.
        /// @param name The value name.
        /// @param data The value data.
        /// @param type (optional) The type. If not supplied, defaults to REG_SZ.
        /// @throws Instalog::SystemFacades::Win32Exception on failure
        void SetValue(std::wstring const& name, std::wstring const& data, DWORD type = REG_SZ)
        {
            this->SetValue(name, data.size() * sizeof(wchar_t), static_cast<void const *>(data.data()), type);
        }

        /// @brief    Deletes this registry key. This method requires that the
        ///         registry key was opened with the DELETE access right.
        void Delete();

        /// @brief    Checks the validity of this instance.
        ///
        /// @return    true if this is a valid registry key, false otherwise
        bool Valid() const;

        /// @brief    Checks the inverse of validity of this instance.
        ///
        /// @remarks Equivilent to !Valid()
        /// 
        /// @return    true if this instance is invalid, false otherwise.
        bool Invalid() const;

        /// @brief    Gets the names of all values stored in this registry key.
        ///
        /// @return    A vector of value names.
        std::vector<std::wstring> EnumerateValueNames() const;

        /// @brief    Gets the registry values contained in this key.
        ///
        /// @return    A vector of registry values.
        std::vector<RegistryValueAndData> EnumerateValues() const;

        /// @brief    Gets the name of this registry key.
        ///
        /// @return    The name.
        std::wstring GetName() const;

        /// @brief    Gets the size information for this registry key.
        ///
        /// @return    The size information.
        RegistryKeySizeInformation GetSizeInformation() const;

        /// @brief    Gets the sub key names of this registry key.
        ///
        /// @return    A vector of sub key names.
        std::vector<std::wstring> EnumerateSubKeyNames() const;

        /// @brief    Enumerates sub keys.
        ///
        /// @param    samDesired    (optional) The access rights desired when opening
        ///                     child keys.
        ///
        /// @return    A vector of child registry keys. In the event an error occurs
        ///         in opening any of these keys, they will be invalid.
        std::vector<RegistryKey> EnumerateSubKeys(REGSAM samDesired = KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS) const;

        /// @brief    Opens a registry key.
        ///
        /// @param    key              The full native path to the key to open.
        /// @param    samDesired    (optional) The access rights desired when opening
        ///                     the key.
        ///
        /// @return    A RegistryKey instance. In the event an error occurs, this
        ///         instance will be invalid. Call GetLastError for extended
        ///         error information.
        static RegistryKey Open(std::wstring const& key, REGSAM samDesired = KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);

        /// @brief    Opens a registry key.
        ///
        /// @param    parent          The parent key, where the key to open is rooted.
        /// @param    key              The path of the sub key to open.
        /// @param    samDesired    (optional) The access rights desired when opening
        ///                     the key.
        ///
        /// @return    A RegistryKey instance. In the event an error occurs, this
        ///         instance will be invalid. Call GetLastError for extended
        ///         error information.
        static RegistryKey Open(RegistryKey const& parent, std::wstring const& key, REGSAM samDesired = KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);

        /// @brief    Opens a registry key.
        ///
        /// @param    parent          The parent key, where the key to open is rooted.
        /// @param    key              The path of the sub key to open.
        /// @param    samDesired    (optional) The access rights desired when opening
        ///                     the key.
        ///
        /// @return    A RegistryKey instance. In the event an error occurs, this
        ///         instance will be invalid. Call GetLastError for extended
        ///         error information.
        static RegistryKey Open(RegistryKey const& parent, UNICODE_STRING& key, REGSAM samDesired = KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS);

        /// @brief    Opens or creates a registry key.
        ///
        /// @param    key              The key path to create.
        /// @param    samDesired    (optional) The access rights desired when opening
        ///                     the key.
        /// @param    options       (optional) Flags passed to Windows regarding the
        ///                     creation of the indicated key.
        ///
        /// @return    A RegistryKey instance. In the event an error occurs, this
        ///         instance will be invalid. Call GetLastError for extended
        ///         error information.
        static RegistryKey Create(
            std::wstring const& key,
            REGSAM samDesired = KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
            DWORD options = REG_OPTION_NON_VOLATILE
        );

        /// @brief    Opens or creates a registry key.
        ///
        /// @param    parent          The parent key, from which the created key is
        ///                     rooted.
        /// @param    key              The key path to create.
        /// @param    samDesired    (optional) The access rights desired when opening
        ///                     the key.
        /// @param    options       (optional) Flags passed to Windows regarding the
        ///                     creation of the indicated key.
        ///
        /// @return    A RegistryKey instance. In the event an error occurs, this
        ///         instance will be invalid. Call GetLastError for extended
        ///         error information.
        static RegistryKey Create(
            RegistryKey const& parent,
            std::wstring const& key,
            REGSAM samDesired = KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
            DWORD options = REG_OPTION_NON_VOLATILE
        );

        /**
         * Checks this instance for validity and throws an exception if it is not.
         *
         * @remarks This function depends on the value of LastError set in Create or Open.
         * 
         * @throws Instalog::SystemFacades::Win32Exception The registry key is invalid.
         */
        void Check() const;
    };
}}
