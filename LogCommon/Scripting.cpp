#include "pch.hpp"
#include "Scripting.hpp"

namespace Instalog
{
	void Log::Notify() const
	{
		for (std::size_t idx = 0; idx < listeners.size(); ++idx)
		{
			listeners[idx](this); //Call the listening thing
		}
	}


	void Log::AddListener( std::function<void(Log const*)> listener )
	{
		listeners.push_back(listener);
	}


	void Log::AddSection( LogSection const& section )
	{
		sections.push_back(section);
		Notify();
	}


}
