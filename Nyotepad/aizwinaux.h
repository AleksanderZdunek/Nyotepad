#pragma once
#include <Windows.h>
#include <functional>
#include <stdexcept>
#include <string>

#include <comdef.h> //_com_error

#include <iostream> //CreateConsole

template<typename Interface>
inline void SafeRelease(Interface** ppInterfaceToRelease)
{
	if (nullptr != *ppInterfaceToRelease)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = nullptr;
	}
}

inline bool CreateConsole()
{
	bool r(AllocConsole() != 0);
	FILE* unused;
	freopen_s(&unused, "CONIN$", "r", stdin);
	freopen_s(&unused, "CONOUT$", "w", stdout);
	freopen_s(&unused, "CONOUT$", "w", stderr);
	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();
	return r;
}

//~~~~~~~~~~ Error code handling ~~~~~~~~~~~~~~~~~~
struct onError_struct
{
	//char const * message;
	std::string message;
	std::function<void()> cleanUpFunction;
};

inline onError_struct OnError(char const * s, std::function<void()> cleanUpFunction = []() {})
{
	return {s, cleanUpFunction};
}

inline onError_struct OnError(char const * file, int line, std::function<void()> cleanUpFunction = []() {})
{
	return	{"File: "+(file+(" Line: "+std::to_string(line))), cleanUpFunction };
}

namespace aiz{
inline std::string wtomb(const wchar_t* orig)
{
	std::string r;
	
	const size_t newsize = wcslen(orig) * 2 + 2;
	r.reserve(newsize);

	size_t convertedChars(0);
	wcstombs_s(&convertedChars, const_cast<char*>(r.data()), newsize, orig, _TRUNCATE);

	return r;
}}

inline HRESULT operator >> (HRESULT hr, onError_struct s)
{
	if (FAILED(hr))
	{
		s.cleanUpFunction();
		_com_error err(hr);
		throw std::runtime_error(std::string(s.message) + "\nErrorcode: " + aiz::wtomb(err.ErrorMessage()));
		//throw std::runtime_error(std::string(s.message) + "\nErrorcode: " + err.ErrorMessage() );
	}
	return hr;
}