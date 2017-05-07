#include <Windows.h>
#include "aizwinaux.h"
#include "aizd2daux.h"
#include <d2d1.h>
#include <dwrite.h>
#include <windowsx.h> //GET_X_LPARAM, GET_Y_LPARAM
#include <string>
#include "menu.h"
#include <fstream>
#include "charbuffer.h"
#include "resizable_array.h"

#include "resource.h"
#include "configconstants.h"
#include "ver.h"

namespace Nyotepad
{
//~~~~ Global variables ~~~~
HWND hWindow(nullptr);
const wchar_t* title = L"Nyotepad";
AIZ::DPIScale dpi;
HACCEL hAccelTable(nullptr);
aizstring theString(L"");
unsigned int mouseWheelScrollLines;
std::wstring activeFilepath;
std::wstring activeFileName;

	namespace D2D
	{
	//~~~~ Device Independent Resources ~~~~
	ID2D1Factory* pD2DFactory(nullptr);
	//~~~~ Device Dependent Resources ~~~~
	ID2D1HwndRenderTarget* pRenderTarget(nullptr);
	ID2D1SolidColorBrush* pBackgroundBrush(nullptr);
	ID2D1SolidColorBrush* pTextBrush(nullptr);
	ID2D1SolidColorBrush* pHighlightBrush(nullptr);

	//~~~~ Functions ~~~~
	void Init()
	{
		D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory) >> OnError(__FILE__, __LINE__);
	}

	void CreateDeviceResources()
	{
		if (!pRenderTarget)
		{
			RECT rc;
			GetClientRect(hWindow, &rc);
			D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

			pD2DFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hWindow, size), &pRenderTarget) >> OnError(__FILE__, __LINE__);
			pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBackgroundBrush) >> OnError(__FILE__, __LINE__);
			pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pTextBrush) >> OnError(__FILE__, __LINE__);
			pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSkyBlue), &pHighlightBrush) >> OnError(__FILE__, __LINE__);
		}
		return;
	}

	void DiscardDeviceResources()
	{
		SafeRelease(&pTextBrush);
		SafeRelease(&pBackgroundBrush);
		SafeRelease(&pHighlightBrush);
		SafeRelease(&pRenderTarget);
	}
	} //namespace D2D

	namespace Scrollbar
	{
	void SetScrollbar(float min, float max, float page)
	{
		SCROLLINFO s = { sizeof(SCROLLINFO) };
		s.fMask = SIF_PAGE | SIF_RANGE; //SIF_POS
		s.fMask |= SIF_DISABLENOSCROLL;
		s.nMin = static_cast<int>(min);
		s.nMax = static_cast<int>(max);
		s.nPage = static_cast<UINT>(page);
		//s.nPos = static_cast<int>(pos);

		::SetScrollInfo(hWindow, SB_VERT, &s, true);
	}

	void SetScrollbarPos(int pos)
	{
		SCROLLINFO s = { sizeof(SCROLLINFO) };
		s.fMask = SIF_POS;
		s.nPos = pos;
		::SetScrollInfo(hWindow, SB_VERT, &s, true);
	}

	int GetScrollOffset()
	{
		SCROLLINFO s{ sizeof(SCROLLINFO), SIF_RANGE | SIF_POS };
		::GetScrollInfo(hWindow, SB_VERT, &s);
		return s.nPos - s.nMin;
	}
	}//namespace Scrollbar

	namespace DWrite
	{
	using namespace D2D;

	bool drawCaret(true);
	float averageLineHeight;
	DWRITE_TEXT_METRICS textLayoutMetrics;

	//~~~~ Device Independent Resources ~~~~ ~~~~
	IDWriteFactory* pDWriteFactory(nullptr);
	IDWriteTextFormat* pTextFormat(nullptr);
	IDWriteTextLayout* pTextLayout(nullptr);

	//~~~~ Functions ~~~~
	void UpdateTextMetricsStructure()
	{
		pTextLayout->GetMetrics(&textLayoutMetrics) >> OnError(__FILE__, __LINE__);
	}

	float GetAverageLineHight()
	{
		//DWRITE_TEXT_METRICS m;
		//pTextLayout->GetMetrics(&m);

		return textLayoutMetrics.height / textLayoutMetrics.lineCount;
	}

	void CreateTextLayout(aizstring const &string)
	{
		SafeRelease(&pTextLayout);
		RECT rect;
		GetClientRect(hWindow, &rect);
		pDWriteFactory->CreateTextLayout(string.data(), string.length(), pTextFormat, dpi.PixelsToDipsX(rect.right), dpi.PixelsToDipsY(rect.bottom), &pTextLayout) >> OnError(__FILE__, __LINE__);

		UpdateTextMetricsStructure();
		Scrollbar::SetScrollbar(0.f, textLayoutMetrics.height, textLayoutMetrics.layoutHeight);
		averageLineHeight = GetAverageLineHight();
	}

	void Init()
	{
		DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory)) >> OnError(__FILE__, __LINE__);

		pDWriteFactory->CreateTextFormat(L"Segoe Script", nullptr, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-us", &pTextFormat) >> OnError(__FILE__, __LINE__);
		//L"Segoe Script"

		CreateTextLayout(theString);
	}

	size_t CursorPosAtPoint(FLOAT x, FLOAT y)
	{
		BOOL isTrailingHit, isInside;
		DWRITE_HIT_TEST_METRICS m;

		pTextLayout->HitTestPoint(x, y, &isTrailingHit, &isInside, &m) >> OnError(__FILE__, __LINE__);

		if( isTrailingHit ) return m.textPosition + 1;

		return m.textPosition;
	}

	void DrawCaret(float offsetX = 0.f, float offsetY = 0.f)
	{
		DWRITE_HIT_TEST_METRICS m;
		float pointX, pointY;
		pTextLayout->HitTestTextPosition(theString.GetCursorPos(), false, &pointX, &pointY, &m) >> OnError(__FILE__, __LINE__);
		pointX += offsetX;
		pointY += offsetY;

		D2D1_POINT_2F p0{ pointX+1, pointY };
		D2D1_POINT_2F p1{ pointX+1, pointY+m.height };
		pRenderTarget->DrawLine(p0, p1, pTextBrush);
	}

	void HighlightSelection(float originX, float originY)
	{
		size_t selectionLength(theString.GetSelectionLength());
		if( 0 < selectionLength ) //don't do selection highlighting if no selection
		{
			DWRITE_LINE_METRICS unused;
			UINT32 lineCount(0);
			pTextLayout->GetLineMetrics(&unused, 1, &lineCount);

			ResizableArray<DWRITE_HIT_TEST_METRICS> arr(lineCount);
			HRESULT result = pTextLayout->HitTestTextRange(theString.GetSelectionStartPosition(), selectionLength, originX, originY, arr.data, arr.capacity, &arr.size);// >> OnError(__FILE__, __LINE__);
			if( E_NOT_SUFFICIENT_BUFFER == result ) //resize output array capacity if needed
			{
				arr.SetCapacity(arr.size);
				pTextLayout->HitTestTextRange(theString.GetSelectionStartPosition(), selectionLength, originX, originY, arr.data, arr.capacity, &arr.size) >> OnError(__FILE__, __LINE__);

			}
			else result >> OnError(__FILE__, __LINE__);

			for( size_t i(0); i < arr.size; i++ )
			{
				DWRITE_HIT_TEST_METRICS &m = arr.data[i];
				D2D1_RECT_F rect{ m.left, m.top, m.left + m.width, m.top + m.height };
				pRenderTarget->FillRectangle(rect, pHighlightBrush);
			}

			//pTextLayout->SetDrawingEffect(pBackgroundBrush, selectionRange) >> OnError(__FILE__, __LINE__); //changes text color
		}
	}

	void RenderText()
	{
		RECT rc;
		GetClientRect(hWindow, &rc);

		//DWRITE_TEXT_METRICS tm;
		//pTextLayout->GetMetrics(&tm) >> OnError(__FILE__, __LINE__);
		float yOffset{-static_cast<float>(Scrollbar::GetScrollOffset())};

		D2D1_POINT_2F origin = D2D1::Point2F(dpi.PixelsToDipsX(rc.left), dpi.PixelsToDipsY(rc.top)+yOffset);

		HighlightSelection(0.f, yOffset);
		pRenderTarget->DrawTextLayout(origin, pTextLayout, pTextBrush);

		if(drawCaret) DrawCaret(0.f, yOffset);
	}
	}//namespace DWrite


//~~~~ Functions ~~~~

void SetWindowTitle(const wchar_t* title)
{
	SetWindowText(hWindow, title);
}

void ResetActiveFilepath()
{
	activeFilepath.clear();
	activeFileName.clear();
	SetWindowTitle(title);
}

void SetActiveFilepath(const wchar_t* path, const wchar_t* name = L"")
{
	activeFilepath = path;
	activeFileName = name;
	SetWindowTitle((std::wstring() + name + L" - " + Nyotepad::title).c_str());
}

HRESULT OnRender()
{
	D2D::CreateDeviceResources();

	D2D::pRenderTarget->BeginDraw();
	D2D::pRenderTarget->SetTransform(D2D1::IdentityMatrix());
	D2D::pRenderTarget->Clear(D2D::pBackgroundBrush->GetColor());

	DWrite::RenderText();

	HRESULT enddrawresult(D2D::pRenderTarget->EndDraw());
	if (D2DERR_RECREATE_TARGET == enddrawresult) D2D::DiscardDeviceResources();
	else enddrawresult >> OnError(__FILE__, __LINE__);
	return S_OK;
}

void OnResize(UINT width, UINT height)
{
	if(D2D::pRenderTarget)
	{
		D2D::pRenderTarget->Resize(D2D1::SizeU(width, height));

		DWrite::pTextLayout->SetMaxWidth(dpi.PixelsToDipsX(width));
		DWrite::pTextLayout->SetMaxHeight(dpi.PixelsToDipsY(height));
	}
}

//void OnClick(UINT x, UINT y)
//{
//	DWRITE_HIT_TEST_METRICS hitTestMetrics;
//	BOOL isTrailingHit;
//	BOOL isInside;
//
//	DWrite::pTextLayout->HitTestPoint(dpi.PixelsToDipsX(x), dpi.PixelsToDipsX(y), &isTrailingHit, &isInside, &hitTestMetrics);
//
//	if (isInside == TRUE)
//	{
//		BOOL underline;
//		DWrite::pTextLayout->GetUnderline(hitTestMetrics.textPosition, &underline);
//		DWRITE_TEXT_RANGE textRange = { hitTestMetrics.textPosition, 1 };
//		DWrite::pTextLayout->SetUnderline(!underline, textRange);
//
//		OnRender();
//	}
//}

void OnClick(UINT x, UINT y)
{
	float yOffset{ -static_cast<float>(Scrollbar::GetScrollOffset()) };
	theString.SetCursorPos( DWrite::CursorPosAtPoint(dpi.PixelsToDipsX(x), dpi.PixelsToDipsY(y)-yOffset) );
	theString.InitializeSelection();
	DWrite::drawCaret = true;
	InvalidateRect(hWindow, nullptr, false);
}

void OnMouseMove(UINT x, UINT y)
{
	float yOffset{ -static_cast<float>(Scrollbar::GetScrollOffset()) };
	theString.SetCursorPos(DWrite::CursorPosAtPoint(dpi.PixelsToDipsX(x), dpi.PixelsToDipsY(y)-yOffset));
	InvalidateRect(hWindow, nullptr, false);
}

void ScrollToCursor()
{
	RECT rc;
	GetClientRect(hWindow, &rc);

	float yOffset{ -static_cast<float>(Scrollbar::GetScrollOffset()) };

	//get cursor position -- dips
	DWRITE_HIT_TEST_METRICS m;
	float pointX, pointY; //cursor point
	DWrite::pTextLayout->HitTestTextPosition(theString.GetCursorPos(), false, &pointX, &pointY, &m) >> OnError(__FILE__, __LINE__);

	if( pointY+m.height+yOffset > dpi.PixelsToDipsY(rc.bottom) )
	{
		Scrollbar::SetScrollbarPos(static_cast<int>(pointY+m.height - DWrite::textLayoutMetrics.layoutHeight));
	}
	else if( pointY + yOffset < dpi.PixelsToDipsY(rc.top) )
	{
		Scrollbar::SetScrollbarPos(static_cast<int>(pointY));
	}
}

void CharInput(wchar_t c)
{
	DWrite::drawCaret = true;

	if( 0x8 == c ) //handle backspace
	{
		if( theString.GetSelectionLength() ) theString.EraseSelection();
		else theString.Erase();
	}
	else
	{
		if( theString.GetSelectionLength() ) theString.EraseSelection();
		theString.Insert(c);
	}

	DWrite::CreateTextLayout(theString);
	ScrollToCursor();
	OnRender();
}

void OnFileNew()
{
	ResetActiveFilepath();
	theString.clear();
	DWrite::CreateTextLayout(theString);

}

void OpenFile(wchar_t* path)
{
	ResetActiveFilepath();

	std::wifstream file(path, std::ios_base::binary);
	if( !file.good() )
	{
		MessageBox(hWindow, L"Error opening file stream", L"Open file error", MB_ICONWARNING);
		return;
	}

	theString.clear();
	
	WCHAR c;
	while( true )
	{
		c = file.get();
		if( file.good() ) theString.push_back(c);
		else break;
	}

	DWrite::CreateTextLayout(theString);
}

void OpenFileDialogBox()
{
	//https://msdn.microsoft.com/en-us/library/windows/desktop/ms646960(v=vs.85).aspx

	const DWORD MAXPATH = 32767; //https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#maxpath
	TCHAR filepath[MAXPATH] = {0}; 
	TCHAR filename[MAXPATH] = {0};

	OPENFILENAME s = {sizeof(OPENFILENAME), hWindow};
	s.lpstrFilter = nullptr;
	s.lpstrCustomFilter = nullptr;
	s.nFilterIndex = 1;
	s.lpstrFile = filepath;
	s.nMaxFile = MAXPATH;
	s.lpstrFileTitle = filename;
	s.nMaxFileTitle = MAXPATH;
	s.lpstrInitialDir = nullptr;
	s.lpstrTitle = nullptr;
	s.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

	if( !GetOpenFileName(&s) ) //Dialog box canceled or error
	{
		if( CommDlgExtendedError() ) MessageBox(hWindow, L"Open File Dialog Error", L"Dialog Error", 0); //Dialog box error

		return;
	}

	OpenFile(s.lpstrFile);
}

void OnFileOpen()
{
	OpenFileDialogBox();
}

void OnFileDrop(HDROP hDrop)
{
	const DWORD MAXPATH = 32767; //https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#maxpath
	TCHAR filepath[MAXPATH] = { 0 };

	UINT res = DragQueryFile(hDrop, 0, filepath, MAXPATH);
	if( 0 == res )MessageBox(hWindow, L"", L"File drop error", MB_ICONERROR);
	else OpenFile(filepath);

	DragFinish(hDrop);
}

void SaveToFile(const wchar_t* filename)
{
	std::wofstream file(filename, std::ios_base::binary);

	if( !file.good() )
	{
		MessageBox(hWindow, L"Error opening file stream", L"Save file error", MB_ICONWARNING);
		return;
	}

	file.write(theString.data(), theString.size());

	if( !file.good() ) MessageBox(hWindow, L"Error writing to file", L"Save file error", MB_ICONWARNING);

	file.close();
}

void SaveAsFileDialogBox()
{
	//https://msdn.microsoft.com/en-us/library/windows/desktop/ms646960(v=vs.85).aspx

	const DWORD MAXPATH = 32767; //https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx#maxpath
	TCHAR filepath[MAXPATH] = { 0 };
	TCHAR filename[MAXPATH] = { 0 };

	OPENFILENAME s = { sizeof(OPENFILENAME), hWindow };
	s.lpstrFilter = nullptr;
	s.lpstrCustomFilter = nullptr;
	s.nFilterIndex = 1;
	s.lpstrFile = filepath;
	s.nMaxFile = MAXPATH;
	s.lpstrFileTitle = filename;
	s.nMaxFileTitle = MAXPATH;
	s.lpstrInitialDir = nullptr;
	s.lpstrTitle = nullptr;
	s.Flags;

	if( !GetSaveFileName(&s) ) //Dialog box canceled or error
	{
		if( CommDlgExtendedError() ) MessageBox(hWindow, L"\"Save As...\"  Dialog Error", L"Dialog Error", 0); //Dialog box error

		return;
	}

	SaveToFile(s.lpstrFile);

	SetActiveFilepath(s.lpstrFile, s.lpstrFileTitle);
}

void OnFileSaveAs()
{
	SaveAsFileDialogBox();
}

void SaveToTestFile()
{
	SaveToFile(L"testsave.txt");
}

void SaveToActiveFilepath()
{
	if( activeFilepath.empty() ) OnFileSaveAs();
	else SaveToFile(activeFilepath.c_str());
}

void OnFileSave()
{
	//SaveToTestFile();
	SaveToActiveFilepath();
}

void OnScroll(WORD param)
{
	SCROLLINFO s{ sizeof(SCROLLINFO), SIF_ALL };
	GetScrollInfo(hWindow, SB_VERT, &s);
	s.fMask = SIF_POS;

	switch(param)
	{
	case SB_TOP:
		s.nPos = s.nMin;
		break;
	case SB_BOTTOM:
		s.nPos = s.nMax;
		break;
	case SB_LINEUP:
		s.nPos -= static_cast<int>(DWrite::averageLineHeight);
		break;
	case SB_LINEDOWN:
		s.nPos += static_cast<int>(DWrite::averageLineHeight);
		break;
	case SB_PAGEUP:
		s.nPos -= s.nPage;
		break;
	case SB_PAGEDOWN:
		s.nPos += s.nPage;
		break;
	case SB_THUMBTRACK:
		s.nPos = s.nTrackPos;
		break;
	default:
		return;
	}

	SetScrollInfo(hWindow, SB_VERT, &s, true);
}

int GetScrollWheelSpeed()
{
	return static_cast<int>(mouseWheelScrollLines * DWrite::averageLineHeight);
}

void OnMouseWheel(short zDelta)
{
	SCROLLINFO s{ sizeof(SCROLLBARINFO), SIF_POS };
	GetScrollInfo(hWindow, SB_VERT, &s);
	s.nPos -= GetScrollWheelSpeed() * zDelta / WHEEL_DELTA;
	SetScrollInfo(hWindow, SB_VERT, &s, true);
}

void CursorLineChange(int dir)
{
	DWRITE_HIT_TEST_METRICS m;
	float pointX, pointY;
	DWrite::pTextLayout->HitTestTextPosition(theString.GetCursorPos(), false, &pointX, &pointY, &m) >> OnError(__FILE__, __LINE__);

	theString.SetCursorPos(DWrite::CursorPosAtPoint(pointX, pointY+m.height*dir));
	DWrite::drawCaret = true;
	InvalidateRect(hWindow, nullptr, false);

	ScrollToCursor();
}

void CursorLineUp()
{
	CursorLineChange(-1);
};

void CursorLineDown()
{
	CursorLineChange(1);
};

void OnKeyDown(WPARAM wParam)
{
#ifdef NOSHIFT
#error Macro pollution __LINE__ __FILE__
#endif
#define NOSHIFT !(0xfff0 & GetKeyState(VK_SHIFT)) //0xfff0 checks high order bit for key down. 0x000f checks low order bit for key toggle

	switch( wParam )
	{
	case VK_LEFT:
		theString.DecrementCursor();
		if( NOSHIFT ) theString.InitializeSelection();
		DWrite::drawCaret = true;
		InvalidateRect(hWindow, nullptr, false);
		break;
	case VK_UP:
		CursorLineUp();
		if( NOSHIFT ) theString.InitializeSelection();
		DWrite::drawCaret = true;
		InvalidateRect(hWindow, nullptr, false);
		break;
	case VK_RIGHT:
		theString.IncrementCursor();
		if( NOSHIFT ) theString.InitializeSelection();
		DWrite::drawCaret = true;
		InvalidateRect(hWindow, nullptr, false);
		break;
	case VK_DOWN:
		CursorLineDown();
		if( NOSHIFT ) theString.InitializeSelection();
		DWrite::drawCaret = true;
		InvalidateRect(hWindow, nullptr, false);
		break;
	default:
		break;
	}
#undef NOSHIFT
}

void OnCopy()
{
	if( 0 == theString.GetSelectionLength() ) return; //Nothing to copy to clip board.

	const wchar_t* pCopyData{ theString.data() + theString.GetSelectionStartPosition() };
	const size_t copyDataSize_byte{ sizeof(wchar_t)*(theString.GetSelectionLength()+1) }; //+1 because clipboard expects null terminated string

	BOOL success = OpenClipboard(hWindow);
	if( !success ) { MessageBox(hWindow, L"Error Opening Clipboard", L"Edit Copy Error", MB_ICONWARNING); return; }

	success = EmptyClipboard();
	if( !success ) { MessageBox(hWindow, L"Error Emptying Clipboard", L"Edit Copy Error", MB_ICONWARNING); CloseClipboard(); return; }

	HGLOBAL hClipboardData{ GlobalAlloc(GMEM_MOVEABLE, copyDataSize_byte) };
	if( NULL == hClipboardData ) { MessageBox(hWindow, L"Error Global Alloc", L"Edit Copy Error", MB_ICONWARNING); CloseClipboard(); return; }

	wchar_t* pClipboardData{ reinterpret_cast<wchar_t*>(GlobalLock(hClipboardData)) };
	memcpy(pClipboardData, pCopyData, copyDataSize_byte);
	pClipboardData[theString.GetSelectionLength()] = 0; //clipboard expects null terminated string
	GlobalUnlock(hClipboardData);

	hClipboardData = SetClipboardData(CF_UNICODETEXT, hClipboardData);
	if( NULL == hClipboardData ) { MessageBox(hWindow, L"Error Set Clipboard Data", L"Edit Copy Error", MB_ICONWARNING); CloseClipboard(); return; }

	CloseClipboard();
	//ScrollToCursor();
}

bool OnPaste()
{
	bool retval(false); //Return true if changes were made.

	if( !IsClipboardFormatAvailable(CF_UNICODETEXT) ) return false;

	BOOL success = OpenClipboard(hWindow);
	if( !success ) { MessageBox(hWindow, L"Error Opening Clipboard", L"Edit Paste Error", MB_ICONWARNING); return false; }

	HGLOBAL hClipboardData{GetClipboardData(CF_UNICODETEXT)};
	if( NULL == hClipboardData ) { MessageBox(hWindow, L"Error Get Clipboard Data", L"Edit Paste Error", MB_ICONWARNING); CloseClipboard(); return false; }

	wchar_t* pCliboardData{reinterpret_cast<wchar_t*>(GlobalLock(hClipboardData))};
	if( NULL == hClipboardData ) { MessageBox(hWindow, L"Error Global Lock data pointer", L"Edit Paste Error", MB_ICONWARNING); CloseClipboard(); return false; }

	if( 0 != pCliboardData[0] )
	{
		retval = true;
		theString.EraseSelection(); //Only replace selection if clipboard contains real data.
		for( ; *pCliboardData != 0; pCliboardData++ )
		{
			theString.Insert(*pCliboardData);
		}
	}

	GlobalUnlock(hClipboardData);
	CloseClipboard();
	//ScrollToCursor();
	return retval;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LRESULT result(0);

	static bool mousedown(false); //ugly hack. see case WM_MOUSEMOVE:

	switch (message)
	{
	case WM_SIZE:
		OnResize(LOWORD(lParam), HIWORD(lParam)); //width, height
		result = 0;
	break;

	case WM_DISPLAYCHANGE:
		InvalidateRect(hWnd, nullptr, false);
		result = 0;
		break;

	case WM_PAINT:
		OnRender();
		ValidateRect(hWnd, nullptr);
		result = 0;
		break;

	case WM_LBUTTONDOWN:
		mousedown = true;
		OnClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_LBUTTONUP:
		mousedown = false;
		break;

	case WM_MOUSEMOVE:
		if( mousedown )
		{//this is an ugly hack to stop initial text selection when opening file by doubleclick in file open dialog.
			OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			mousedown = (MK_LBUTTON == wParam);
		}
		//if( MK_LBUTTON == wParam ) OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)); //this is the nicer looking solution
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		result = 1;
		break;
		
	case WM_CHAR:
		CharInput(static_cast<wchar_t>(wParam));
		result = 0;
		break;

	case WM_KEYDOWN:
		OnKeyDown(wParam);
		result = 0;
		break;

	case WM_COMMAND:
	{
		switch( LOWORD(wParam) )
		{
		case Menu::FILE_NEW:
			OnFileNew();
			InvalidateRect(hWnd, nullptr, false);
			break;
		case Menu::FILE_OPEN:
			OnFileOpen();
			InvalidateRect(hWnd, nullptr, false);
			break;
		case Menu::FILE_SAVE:
			OnFileSave();
			break;
		case Menu::FILE_SAVEAS:
			OnFileSaveAs();
			break;
		case Menu::FILE_QUIT:
			PostQuitMessage(0);
			break;
		case Menu::EDIT_COPY:
			OnCopy();
			break;
		case Menu::EDIT_PASTE:
			if( OnPaste() )
			{
				DWrite::CreateTextLayout(theString);
				ScrollToCursor();
				InvalidateRect(hWnd, nullptr, false);
			}
			break;
		}
	} break;

	case WM_TIMER:
		DWrite::drawCaret = !DWrite::drawCaret;
		InvalidateRect(hWindow, nullptr, false);
		break;

	case WM_VSCROLL:
		OnScroll(LOWORD(wParam));
		InvalidateRect(hWnd, nullptr, false);
		break;

	case WM_MOUSEWHEEL:
		OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
		InvalidateRect(hWindow, nullptr, false);
		break;

	case WM_DROPFILES:
		OnFileDrop((HDROP)wParam);
		break;

	case WM_SETTINGCHANGE:
		std::cout << "WM_SETTINGCHANGE\n"; //TODO: test this tomorrow
		//Fallthrough. no break;
	default:
		result = DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}

	return result;
}

void MakeWindow()
{

	const UINT width(CW_USEDEFAULT);
	const UINT height(CW_USEDEFAULT);

	//const wchar_t* title = ver::title;

	//Register the window class.
	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = GetModuleHandle(0);
	wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wcex.hCursor = LoadCursor(NULL, IDC_IBEAM);
	wcex.hbrBackground = nullptr;
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"Nyotepad";
	wcex.hIconSm = nullptr;

	RegisterClassEx(&wcex);

	hWindow = CreateWindowEx(WS_EX_ACCEPTFILES, wcex.lpszClassName, title, WS_OVERLAPPEDWINDOW | WS_VSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, width, height, nullptr, nullptr, GetModuleHandle(0), nullptr);
	(hWindow ? S_OK : E_FAIL) >> OnError(__FILE__, __LINE__);
}

void RunMessageLoop()
{
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		if( !TranslateAccelerator( hWindow, hAccelTable, &msg) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void UpdateMouseWheelSpeed()
{
	SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &mouseWheelScrollLines, 0);
}

void Init()
{
	MakeWindow();
	Menu::CreateMenu(hWindow);
	hAccelTable = Menu::ShortcutKeys();

	D2D::Init();
	dpi.Init(D2D::pD2DFactory);
	//D2D::CreateDeviceResources(); //This is done OnRender instead.
	DWrite::Init();

	ShowWindow(hWindow, SW_SHOWNORMAL);
	UpdateWindow(hWindow);

	SetTimer(hWindow, 0, 500, nullptr);

	UpdateMouseWheelSpeed();
}

void Run()
{
	RunMessageLoop();
}

}//namespace Nyotepad