#include "GraphicsError.h"
#include "EntryBuilder.h"
#include <ranges>
#include <format>

#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define chilog EntryBuilder{ __FILEW__, __FUNCTIONW__, __LINE__ }

namespace rn = std::ranges;
namespace vi = rn::views;



	std::wstring GetErrorDescription(HRESULT hr)
{
	wchar_t* descriptionWinalloc = nullptr;
	const auto result = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&descriptionWinalloc), 0, nullptr
	);

	std::wstring description;
	if (!result) {
		chilog.warn(L"Failed formatting windows error");
	}
	else {
		description = descriptionWinalloc;
		if (LocalFree(descriptionWinalloc)) {
			chilog.warn(L"Failed freeing memory for windows error formatting");
		}
		if (description.ends_with(L"\r\n")) {
			description.resize(description.size() - 2);
		}
	}
	return description;
}

	std::string ToNarrow(const std::wstring& wide)
	{
		std::string narrow;
		narrow.resize(wide.size() * 2);
		size_t actual;
		wcstombs_s(&actual, narrow.data(), narrow.size(), wide.c_str(), _TRUNCATE);
		narrow.resize(actual - 1);
		return narrow;
	}
	
	CheckerToken chk;

	HrGrabber::HrGrabber(unsigned int hr, std::source_location loc) noexcept
		:
		hr(hr),
		loc(loc)
	{}

	void operator>>(HrGrabber g, CheckerToken)
	{
		if (FAILED(g.hr)) {
			// get error description as narrow string with crlf removed
			auto errorString = ToNarrow(GetErrorDescription(g.hr)) |
				vi::transform([](char c) {return c == '\n' ? ' ' : c; }) |
				vi::filter([](char c) {return c != '\r'; }) /* |
				rn::to<std::basic_string>()*/;
			throw std::runtime_error{
				std::format("Graphics Error: {0}, {1}, {2}", ToNarrow(GetErrorDescription(g.hr)) , g.loc.file_name(), g.loc.line())
			};
		}
	}
