#pragma once
#include <atlbase.h>
#include <iosfwd>
#include <vector>
#include <string>
#include <memory>

namespace Instalog { namespace SystemFacades {

	class SecurityProduct
	{
	public:
		enum UpdateStatusValues
		{
			OutOfDate = 0,
			UpToDate = 1,
			UpdateNotRequired = 2
		};
	private:
		std::wstring name_;
		std::wstring guid_;
		bool enabled_;
		UpdateStatusValues updateStatus_;
		const wchar_t * letterCode_;
	public:
		SecurityProduct(
			const std::wstring& name,
			const std::wstring& guid,
			bool enabled,
			UpdateStatusValues updateStatus,
			const wchar_t * letterCode)
			:	name_(name), 
				guid_(guid),
				enabled_(enabled),
				updateStatus_(updateStatus),
				letterCode_(letterCode)
		{}
		const wchar_t* GetTwoLetterPrefix() const
		{
			return letterCode_;
		}
		bool IsEnabled() const
		{
			return enabled_;
		}
		const std::wstring& GetInstanceGuid() const
		{
			return guid_;
		}
		const std::wstring& GetName() const
		{
			return name_;
		}
		UpdateStatusValues GetUpdateStatus() const
		{
			return updateStatus_;
		}
		void Delete();
	};

	inline bool operator==(const SecurityProduct& lhs, const SecurityProduct& rhs)
	{
		return lhs.GetInstanceGuid() == rhs.GetInstanceGuid();
	}
	inline bool operator<(const SecurityProduct& lhs, const SecurityProduct& rhs)
	{
		return lhs.GetInstanceGuid() < rhs.GetInstanceGuid();
	}

	std::vector<SecurityProduct> EnumerateSecurityProducts();

	CComPtr<IWbemServices> GetWbemServices();

}}

