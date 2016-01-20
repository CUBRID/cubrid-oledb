#ifndef ECONSOLE_H
#define ECONSOLE_H

#include <windows.h>

//==============================================================================
// class Console - Eric Tetz 10/5/99
//
// Encapsulates the Windows console API (some of it).
//
// Each process can have one and only one console.  For this reason, all Console
// members are static.  If your program is already a console app, you can freely
// call any method of this class.  If your app is NOT a console app, you must
// first call Alloc() to create a console for your process and redirect IO to
// it. You may call Free() to detach your process from it's console.
//
//==============================================================================

class Console
{
public:
	enum Color
	{
		Black       = 0,
		b        = FOREGROUND_INTENSITY,
		LightGrey   = FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE,
		White       = FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
		Blue        = FOREGROUND_BLUE,
		Green       = FOREGROUND_GREEN,
		Cyan        = FOREGROUND_GREEN | FOREGROUND_BLUE,
		Red         = FOREGROUND_RED,
		Purple      = FOREGROUND_RED   | FOREGROUND_BLUE,
		LightBlue   = FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
		LightGreen  = FOREGROUND_GREEN | FOREGROUND_INTENSITY,
		LightCyan   = FOREGROUND_GREEN | FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
		LightRed    = FOREGROUND_RED   | FOREGROUND_INTENSITY,
		LightPurple = FOREGROUND_RED   | FOREGROUND_BLUE  | FOREGROUND_INTENSITY,
		Orange      = FOREGROUND_RED   | FOREGROUND_GREEN,
		Yellow      = FOREGROUND_RED   | FOREGROUND_GREEN | FOREGROUND_INTENSITY,
	};

public:
	static bool Alloc();
	static bool Free();
	static bool SetTitle (LPCSTR sTitle);
	static bool SetSize (int columns, int lines);
	static bool GetSize (int * pcolumns, int * plines);
	static bool SetCursorPos (int x, int y);
	static bool GetCursorPos (int * px, int * py);
	static bool SetCursorSize (DWORD dwPercentShown, bool bVisible = false);
	static bool SetTextColor (Color FgColor, Color BgColor = Black);
	static bool Clear();

protected:
	static bool RedirectIoToConsole ();
};

#endif //ECONSOLE_H
