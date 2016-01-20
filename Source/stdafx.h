// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0400		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target Windows 2000 or later.
#endif

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off ATL's hiding of some common and often safely ignored warning messages
#define _ATL_ALL_WARNINGS

// Conformance Tests
// see atlbase.h(2989), etc.
#define _ATL_OLEDB_CONFORMANCE_TESTS

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

#ifndef DBPROP_CUBRIDPROVIDER_BROKER_PORT
#define DBPROP_CUBRIDPROVIDER_BROKER_PORT	0x200
#define CUBRIDPROVIDER_BROKER_PORT_Type VT_I4
#define CUBRIDPROVIDER_BROKER_PORT_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#endif

#ifndef DBPROP_CUBRIDPROVIDER_FETCH_SIZE
#define DBPROP_CUBRIDPROVIDER_FETCH_SIZE	0x201
#define CUBRIDPROVIDER_FETCH_SIZE_Type VT_I4
//#define CUBRIDPROVIDER_FETCH_SIZE_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ )
#define CUBRIDPROVIDER_FETCH_SIZE_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
#endif

#ifndef DBPROP_CUBRIDPROVIDER_AUTOCOMMIT
#define DBPROP_CUBRIDPROVIDER_AUTOCOMMIT	0x202
#define CUBRIDPROVIDER_AUTOCOMMIT_Type VT_I4
#define CUBRIDPROVIDER_AUTOCOMMIT_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
//#define CUBRIDPROVIDER_AUTOCOMMIT_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ )
#endif

#ifndef DBPROP_CUBRIDPROVIDER_LOGIN_TIMEOUT
#define DBPROP_CUBRIDPROVIDER_LOGIN_TIMEOUT	0x203
#define CUBRIDPROVIDER_LOGIN_TIMEOUT_Type VT_I4
#define CUBRIDPROVIDER_LOGIN_TIMEOUT_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
//#define CUBRIDPROVIDER_LOGIN_TIMEOUT_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ )
#endif

#ifndef DBPROP_CUBRIDPROVIDER_QUERY_TIMEOUT
#define DBPROP_CUBRIDPROVIDER_QUERY_TIMEOUT	0x204
#define CUBRIDPROVIDER_QUERY_TIMEOUT_Type VT_I4
#define CUBRIDPROVIDER_QUERY_TIMEOUT_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE )
//#define CUBRIDPROVIDER_QUERY_TIMEOUT_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ )
#endif

#ifndef DBPROP_CUBRIDPROVIDER_CHARSET
#define DBPROP_CUBRIDPROVIDER_CHARSET	0x205
#define CUBRIDPROVIDER_CHARSET_Type VT_BSTR
#define CUBRIDPROVIDER_CHARSET_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ | DBPROPFLAGS_WRITE  )
//#define CUBRIDPROVIDER_CHARSET_Flags ( DBPROPFLAGS_DBINIT | DBPROPFLAGS_READ  )
#endif

extern bool g_autocommit;
extern int  g_login_timeout;
extern int  g_query_timeout;
extern char g_charset[128];


#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <atltypes.h>
#include <atlctl.h>
#include <atlhost.h>
#include "atldb_cubrid.h"

#include "Localization.h"

using namespace ATL;

extern "C"
{
#include <cas_cci.h>
}
