#pragma once
#include <source_location>
#include "HighIncWindows.h"
#include <string>


	
	struct CheckerToken {};
	extern CheckerToken chk;
	std::wstring GetErrorDescription(HRESULT hr);
	std::string ToNarrow(const std::wstring& wide);
	struct HrGrabber {
		HrGrabber(unsigned int hr, std::source_location = std::source_location::current()) noexcept;
		unsigned int hr;
		std::source_location loc;
	};
	void operator>>(HrGrabber, CheckerToken);
