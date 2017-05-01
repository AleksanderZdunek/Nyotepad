#pragma once

namespace ver
{

wchar_t* const title = L"Nyotepad " "0.0.1x";
wchar_t* const build = L"Build " __DATE__ " @ " __TIME__;
wchar_t* const compile = L"" 
#ifdef _MSC_VER
	#ifdef _DEBUG
	"Debug "
	#else 
	"Release "
	#endif
	#ifdef _WIN64
	"x64"
	#else
		#ifdef _WIN32
		"x86"
		#endif
	#endif
#endif
;
}