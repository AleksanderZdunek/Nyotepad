#include "menu.h"
#include "aizwinaux.h"
#include "ver.h"

void Nyotepad::Menu::CreateMenu(HWND hWindow)
{
	HMENU hMenu = ::CreateMenu();
	(hMenu ? S_OK : HRESULT_FROM_WIN32(GetLastError())) >> OnError(__FILE__, __LINE__);

	//~~~ File menu ~~~
	{
		HMENU hFileMenu = CreatePopupMenu();
		(hFileMenu ? S_OK : HRESULT_FROM_WIN32(GetLastError())) >> OnError(__FILE__, __LINE__);

		MENUITEMINFO m = { sizeof(MENUITEMINFO), MIIM_SUBMENU | MIIM_STRING, MFT_STRING };
		m.hSubMenu = hFileMenu;
		m.dwTypeData = L"File";
		InsertMenuItem(hMenu, 0, MF_BYPOSITION, &m);

		m.fMask = MIIM_ID | MIIM_STRING;
		m.wID = FILE_NEW;
		m.dwTypeData = L"New";
		InsertMenuItem(hFileMenu, 0, MF_BYPOSITION, &m);

		m.wID = FILE_OPEN;
		m.dwTypeData = L"Open";
		InsertMenuItem(hFileMenu, 1, MF_BYPOSITION, &m);

		m.wID = FILE_SAVE;
		m.dwTypeData = L"Save\tCtrl+S";
		InsertMenuItem(hFileMenu, 2, MF_BYPOSITION, &m);

		m.wID = FILE_SAVEAS;
		m.dwTypeData = L"Save As...";
		InsertMenuItem(hFileMenu, 3, MF_BYPOSITION, &m);

		m.wID = FILE_QUIT;
		m.dwTypeData = L"Quit";
		InsertMenuItem(hFileMenu, 4, MF_BYPOSITION, &m);
	}

	//~~~ Edit menu ~~~
	{
		HMENU hEditMenu = CreatePopupMenu();
		(hEditMenu ? S_OK : HRESULT_FROM_WIN32(GetLastError())) >> OnError(__FILE__, __LINE__);

		MENUITEMINFO m = { sizeof(MENUITEMINFO), MIIM_SUBMENU | MIIM_STRING, MFT_STRING };
		m.hSubMenu = hEditMenu;
		m.dwTypeData = L"Edit";
		InsertMenuItem(hMenu, 1, MF_BYPOSITION, &m);

		m.fMask = MIIM_ID | MIIM_STRING;
		m.wID = EDIT_COPY;
		m.dwTypeData = L"Copy\tCtrl+C";
		InsertMenuItem(hEditMenu, 0, MF_BYPOSITION, &m);

		m.wID = EDIT_PASTE;
		m.dwTypeData = L"Paste\tCtrl+V";
		InsertMenuItem(hEditMenu, 1, MF_BYPOSITION, &m);
	}
	
	//~~~ Font ~~~
	{
		MENUITEMINFO m{ sizeof(MENUITEMINFO) };
		m.fMask = MIIM_ID | MIIM_STRING;
		m.wID = OPTION_FONT;
		m.dwTypeData = L"Font";
		InsertMenuItem(hMenu, 2, MF_BYPOSITION, &m);
	}

	//~~~ ver menu ~~~
	{
		HMENU hVerMenu = CreatePopupMenu();
		(hVerMenu ? S_OK : HRESULT_FROM_WIN32(GetLastError())) >> OnError(__FILE__, __LINE__);

		MENUITEMINFO m = { sizeof(MENUITEMINFO) };
		m.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_FTYPE;
		m.fType = MFT_RIGHTJUSTIFY;
		m.hSubMenu = hVerMenu;
		m.dwTypeData = L"ver";
		InsertMenuItem(hMenu, 3, MF_BYPOSITION, &m);

		m.fMask = MIIM_STRING | MIIM_STATE;
		m.fState = MFS_DISABLED;
		m.dwTypeData = ver::title;
		InsertMenuItem(hVerMenu, 0, MF_BYPOSITION, &m);

		m.dwTypeData = ver::author;
		InsertMenuItem(hVerMenu, 1, MF_BYPOSITION, &m);

		m.dwTypeData = ver::build;
		InsertMenuItem(hVerMenu, 2, MF_BYPOSITION, &m);

		m.dwTypeData = ver::compile;
		InsertMenuItem(hVerMenu, 3, MF_BYPOSITION, &m);
	}

	//~~~
	SetMenu(hWindow, hMenu);
}

HACCEL Nyotepad::Menu::ShortcutKeys()
{
	const int nShortcuts(3);

	ACCEL shortcuts[nShortcuts]
	{
		{ FCONTROL | FVIRTKEY, 'S', FILE_SAVE },
		{ FCONTROL | FVIRTKEY, 'C', EDIT_COPY },
		{ FCONTROL | FVIRTKEY, 'V', EDIT_PASTE },
	};

	HACCEL hAccelTable( CreateAcceleratorTable(shortcuts, nShortcuts) );
	(hAccelTable ? S_OK : HRESULT_FROM_WIN32(GetLastError())) >> OnError(__FILE__, __LINE__);

	return hAccelTable;
}
