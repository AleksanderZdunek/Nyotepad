#pragma once
#include <Windows.h>

namespace Nyotepad
{ namespace Menu {

enum
{
	FILE_NEW,
	FILE_OPEN,
	FILE_SAVE,
	FILE_SAVEAS,
	FILE_QUIT,
	EDIT_COPY,
	EDIT_PASTE
};

void CreateMenu(HWND hWindow);
HACCEL ShortcutKeys();

}}//Menu //Nyotepad
