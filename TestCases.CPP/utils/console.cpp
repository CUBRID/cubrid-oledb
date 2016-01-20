#include "stdafx.h" // precompiled header directive

#include <iostream>
#include <wincon.h>
#include <fcntl.h>
#include <io.h>

#include "console.h"

using std::ios;

extern "C" void ConsoleAlloc()
{
	Console::Alloc();
}

bool Console::Alloc()
{
	// If this is already a console app we don't need to call
	// RedirectToConsole().  AllocConsole() fails if this process
	// already has a console.
	return (AllocConsole() && RedirectIoToConsole());
}

bool Console::Free()
{
	return FreeConsole() != 0;
}

bool Console::SetTitle (LPCSTR sTitle)
{
	return SetConsoleTitle ((LPCWSTR)sTitle) != 0;
}

bool Console::RedirectIoToConsole ()
{
	HANDLE hStdHandle;
	int nConHandle;

	// redirect unbuffered STDOUT to the console
	hStdHandle = GetStdHandle (STD_OUTPUT_HANDLE);
	nConHandle = _open_osfhandle ((long)hStdHandle, _O_TEXT);
	*stdout = *_fdopen (nConHandle, "w");
	setvbuf (stdout, NULL, _IONBF, 0);

	// redirect unbuffered STDIN to the console
	hStdHandle = GetStdHandle (STD_INPUT_HANDLE);
	nConHandle = _open_osfhandle ((long)hStdHandle, _O_TEXT);
	*stdin = *_fdopen (nConHandle, "r");
	setvbuf (stdin, NULL, _IONBF, 0);

	// redirect unbuffered STDERR to the console
	hStdHandle = GetStdHandle (STD_ERROR_HANDLE);
	nConHandle = _open_osfhandle ((long)hStdHandle, _O_TEXT);
	*stderr = *_fdopen (nConHandle, "w");
	setvbuf (stderr, NULL, _IONBF, 0);

	// make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog
	// point to console as well
	ios::sync_with_stdio();

	return true;
}

bool Console::GetSize (int * pcolumns, int * plines)
{
	HANDLE hconsole = GetStdHandle (STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO coninfo;

	if (GetConsoleScreenBufferInfo (hconsole, &coninfo))
	{
		*pcolumns = coninfo.dwSize.X;
		*plines   = coninfo.dwSize.Y;

		return true;
	}
	else
	{
		return false;
	}
}

bool Console::SetSize (int columns, int lines)
{
	HANDLE hconsole = GetStdHandle (STD_OUTPUT_HANDLE);

	COORD size = { columns, lines };

	return SetConsoleScreenBufferSize (hconsole, size) !=0;
}

bool Console::SetTextColor (Color FgColor, Color BgColor /*= Black*/)
{
	HANDLE hconsole = GetStdHandle (STD_OUTPUT_HANDLE);

	return (SetConsoleTextAttribute (hconsole, FgColor | BgColor << 4) == TRUE);
}

bool Console::Clear ()
{
	HANDLE hconsole = GetStdHandle (STD_OUTPUT_HANDLE);

	// get the number of character cells in the current buffer
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo (hconsole, &csbi))
		return false;

	COORD coordScreen = { 0, 0 };    // here's where we'll home the cursor
	DWORD cCharsWritten;             // number of chars written by console output routines
	DWORD dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	// fill the entire screen with blanks
	return (FillConsoleOutputCharacter (hconsole, ' ', dwConSize, coordScreen, &cCharsWritten)              &&
		FillConsoleOutputAttribute (hconsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten) &&
		SetConsoleCursorPosition   (hconsole, coordScreen));
}

bool Console::SetCursorPos (int x, int y)
{
	HANDLE hconsole = GetStdHandle (STD_OUTPUT_HANDLE);

	COORD dwCursorPosition = { x, y };

	return (SetConsoleCursorPosition (hconsole, dwCursorPosition) == TRUE);
}


bool Console::GetCursorPos (int * px, int * py)
{
	HANDLE hconsole = GetStdHandle (STD_OUTPUT_HANDLE);

	CONSOLE_SCREEN_BUFFER_INFO coninfo;

	if (GetConsoleScreenBufferInfo (hconsole, &coninfo))
	{
		*px = coninfo.dwCursorPosition.X;
		*py = coninfo.dwCursorPosition.Y;

		return true;
	}
	else
	{
		return false;
	}
}

bool Console::SetCursorSize (DWORD dwPercentShown, bool bVisible /*=false*/)
{
	HANDLE hconsole = GetStdHandle (STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO CursorInfo = { dwPercentShown, bVisible };
	return (SetConsoleCursorInfo (hconsole, &CursorInfo) == TRUE);
}
