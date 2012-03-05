#pragma once
#include <vector>
#include <string>
#include <boost/iterator/iterator_facade.hpp>

namespace Instalog { namespace SystemFacades {
	class Process
	{
		std::size_t id_;
	public:
		explicit Process(std::size_t pid);
		std::size_t GetProcessId() const;
		std::wstring GetCommandLine() const;
		std::wstring GetExecutablePath() const;
		void Terminate();
	};

	class ProcessIterator
		: public boost::iterator_facade<ProcessIterator, Process, boost::forward_traversal_tag, Process>
	{
		friend class boost::iterator_core_access;
		std::vector<unsigned char>::const_iterator blockPtr, end_;
	public:
		void increment();
		bool equal(ProcessIterator const& other) const;
		Process dereference() const;

		ProcessIterator(
			std::vector<unsigned char>::const_iterator start,
			std::vector<unsigned char>::const_iterator end
			);
	};

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
