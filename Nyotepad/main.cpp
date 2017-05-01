//Nyotepad

#include "aizwinaux.h"
#include "nyotepad.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	//CreateConsole();
	//CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED) >> OnError("CoInitializeEx");
	Nyotepad::Init();
	Nyotepad::Run();
	//CoUninitialize();
	return 0;
}

int main(int argc, char* argv[])
{
	std::cout << "Nyotepad 0.0.1x\n";
	
	wWinMain(GetModuleHandle(0), nullptr, nullptr, 1);

	system("pause");
	return 0;
}

