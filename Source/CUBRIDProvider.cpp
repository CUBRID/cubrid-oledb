////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// CUBRIDProvider.cpp : Implementation of DLL Exports.

#include "stdafx.h"
#include "resource.h"

#define ENABLE_LOGGING
#define LOGGING_FILENAME		"c:\\Log_CUBRID.Provider.9.0.0.txt"
#define DEFAULT_TRACE_LEVEL	3

// The module attribute causes DllMain, DllRegisterServer and DllUnregisterServer to be automatically implemented for you

//http://msdn.microsoft.com/en-us/library/windows/desktop/ms221495%28v=vs.85%29.aspx
//http://msdn.microsoft.com/en-us/library/fs2z0f72%28v=vs.71%29.aspx
[
	module(
	dll,
	uuid = "{2B22247F-E9F7-47E8-A9B0-79E8039DCFC8}",
	name = "CUBRIDProvider", //name of the DLL
	version = "8.4", //version of the DLL (only major and minor accepted)
	helpstring = "CUBRID OLE DB Provider 8.4.1 Type Library",
	resource_name = "IDR_CUBRIDPROVIDER")
]

class CCUBRIDProviderModule
{
#ifndef _DEBUG
public:
	CCUBRIDProviderModule()
	{
		cci_init();
	}

	~CCUBRIDProviderModule()
	{
		//TODO Verify if we should call cci_end()
		//cci_end();
	}
#endif

#ifdef _DEBUG
private:
	HANDLE m_hLogFile;
	int m_fWarnMode, m_fErrorMode, m_fAssertMode;

public:
	CCUBRIDProviderModule()
	{
		CTrace::s_trace.SetLevel(DEFAULT_TRACE_LEVEL);
		cci_init();

#ifdef ENABLE_LOGGING
		m_fWarnMode = ::_CrtSetReportMode(_CRT_WARN, _CRTDBG_REPORT_MODE);
		m_fErrorMode = ::_CrtSetReportMode(_CRT_ERROR, _CRTDBG_REPORT_MODE);
		m_fAssertMode = ::_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_REPORT_MODE);
		m_hLogFile = ::CreateFile(LOGGING_FILENAME, GENERIC_WRITE, FILE_SHARE_READ,
			NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		::_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | m_fWarnMode);
		::_CrtSetReportFile(_CRT_WARN, m_hLogFile);
		::_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | m_fErrorMode);
		::_CrtSetReportFile(_CRT_ERROR, m_hLogFile);
		::_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | m_fAssertMode);
		::_CrtSetReportFile(_CRT_ASSERT, m_hLogFile);
#endif
	}

	~CCUBRIDProviderModule()
	{
#ifdef ENABLE_LOGGING
		::_CrtSetReportMode(_CRT_WARN, m_fWarnMode);
		::_CrtSetReportMode(_CRT_ERROR, m_fErrorMode);
		::_CrtSetReportMode(_CRT_ASSERT, m_fAssertMode);
		::CloseHandle(m_hLogFile);
#endif
	}
#endif
};
