// Copyright © 2012 Jacob Snyder, Billy O'Neal III, and "sUBs"
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#pragma once
#include <vector>
#include <string>
#include <boost/iterator/iterator_facade.hpp>

namespace Instalog { namespace SystemFacades {

	/// @brief	Wrapper around a Win32 process
	class Process
	{
		std::size_t id_;
	public:
		/// @brief	Constructor.
		///
		/// @param	pid	The pid of the process of interest
		explicit Process(std::size_t pid);

		/// @brief	Gets the process identifier.
		///
		/// @return	The process identifier.
		std::size_t GetProcessId() const;

		/// @brief	Gets the command line of the process.
		///
		/// @return	The command line.
		std::wstring GetCmdLine() const;

		/// @brief	Gets the executable path of the process.
		///
		/// @return	The executable path.
		std::wstring GetExecutablePath() const;

		/// @brief	Terminates the process
		void Terminate();
	};

	/// @brief	Process iterator. An iterator which loops over processes in memory.
	class ProcessIterator
		: public boost::iterator_facade<ProcessIterator, Process, boost::forward_traversal_tag, Process>
	{
		friend class boost::iterator_core_access;
		std::vector<unsigned char>::const_iterator blockPtr, end_;
		void increment();
		bool equal(ProcessIterator const& other) const;
		Process dereference() const;
	public:
		ProcessIterator() {}
		ProcessIterator(
			std::vector<unsigned char>::const_iterator start,
			std::vector<unsigned char>::const_iterator end
			);
	};

	/// @brief	Process enumerator. Serves as a collection of processes in memory.
	class ProcessEnumerator
	{
		std::vector<unsigned char> informationBlock;
	public:
		typedef ProcessIterator iterator;

		ProcessEnumerator();
		iterator begin();
		iterator end();
	};
	inline bool operator==(
		Instalog::SystemFacades::Process const& lhs,
		Instalog::SystemFacades::Process const& rhs
		)
	{
		return lhs.GetProcessId() == rhs.GetProcessId();
	}

	inline bool operator==(
		Instalog::SystemFacades::Process const& lhs,
		const std::size_t rhs
		)
	{
		return lhs.GetProcessId() == rhs;
	}

	inline bool operator==(
		const std::size_t lhs,
		Instalog::SystemFacades::Process const& rhs
		)
	{
		return lhs == rhs.GetProcessId();
	}

}}
