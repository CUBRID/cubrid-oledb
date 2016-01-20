////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace ProvInfo
{
	extern int DEFAULT_FETCH_SIZE;
	extern bool DEFAULT_AUTOCOMMIT;
	extern int DEFAULT_LOGIN_TIMEOUT;
	extern int DEFAULT_QUERY_TIMEOUT;
	extern WCHAR *DEFAULT_CONN_CHARSET;

	// Buffer with all used strings.
	extern WCHAR wszAllStrings[];
	extern int size_wszAllStrings;

	// Relative strings
	extern LPOLESTR pwszPoint;
	extern LPOLESTR pwszPercent;
	extern LPOLESTR pwszUnderline;
	extern LPOLESTR pwszLeftSqBracket;
	extern LPOLESTR pwszRightSqBracket;
	extern LPOLESTR pwszQuote;
	extern LPOLESTR pwszInvalidChars;
	extern LPOLESTR pwszInvalidFirstChars;
	extern LPOLESTR pwszInvalidCharsShort;
	extern LPOLESTR pwszInvalidFirstCharsShort;

	extern DBLITERALINFO LiteralInfos[];
	extern int size_LiteralInfos;

	typedef struct
	{
		LPWSTR szName; //data type name
		T_CCI_U_TYPE nCCIUType; //data type CCI-type equivalent
		LPWSTR szCreateParams; //CREATE()-required parameters
	} _ptypes;

	extern _ptypes provider_types[];
	extern int size_provider_types;
}
