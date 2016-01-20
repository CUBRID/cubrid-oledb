////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ProviderInfo.h"

namespace ProvInfo
{
	int DEFAULT_FETCH_SIZE = 100;
	bool DEFAULT_AUTOCOMMIT = true;
	int DEFAULT_LOGIN_TIMEOUT = 10000;
	int DEFAULT_QUERY_TIMEOUT = 10000;
	WCHAR *DEFAULT_CONN_CHARSET = L"UTF-8";

	WCHAR wszAllStrings[] =
		L".\0" // pwszPoint
		L"%\0" // pwszPercent
		L"_\0" // pwszUnderline
		L"[\0" // pwszLeftSqBracket
		L"]\0" // pwszRightSqBracket
		L"\"\0" // pwszQuote
		L"!\"%&'()*+,-./:;<=>?@[\\]^{|} ~\0" // pwszInvalidChars
		L"0123456789!\"%&'()*+,-./:;<=>?@[\\]^{|}~ \0" // pwszInvalidFirstChars
		L"\",.;*<>?|\0" // pwszInvalidCharsShort
		L"0123456789\",.;*<>?|\0"; // pwszInvalidFirstCharsShort

	int size_wszAllStrings = sizeof(wszAllStrings);

	// Relative strings
	LPOLESTR pwszPoint = wszAllStrings;
	LPOLESTR pwszPercent = pwszPoint + wcslen( pwszPoint ) + 1;
	LPOLESTR pwszUnderline = pwszPercent + wcslen( pwszPercent ) + 1;
	LPOLESTR pwszLeftSqBracket = pwszUnderline + wcslen( pwszUnderline ) + 1;
	LPOLESTR pwszRightSqBracket = pwszLeftSqBracket + wcslen( pwszLeftSqBracket ) + 1;
	LPOLESTR pwszQuote = pwszRightSqBracket + wcslen( pwszRightSqBracket ) + 1;
	LPOLESTR pwszInvalidChars = pwszQuote + wcslen( pwszQuote ) + 1;
	LPOLESTR pwszInvalidFirstChars = pwszInvalidChars + wcslen( pwszInvalidChars ) + 1;
	LPOLESTR pwszInvalidCharsShort = pwszInvalidFirstChars + wcslen( pwszInvalidFirstChars ) + 1;
	LPOLESTR pwszInvalidFirstCharsShort = pwszInvalidCharsShort + wcslen( pwszInvalidCharsShort ) + 1;

	DBLITERALINFO LiteralInfos[] = {
		{ NULL, NULL, NULL, DBLITERAL_BINARY_LITERAL, TRUE, 16384 },
		//{ NULL, NULL, NULL, DBLITERAL_CATALOG_NAME, FALSE, 0 },
		//{ NULL, NULL, NULL, DBLITERAL_CATALOG_SEPARATOR, FALSE, 0 },
		{ NULL, NULL, NULL, DBLITERAL_CHAR_LITERAL, TRUE, 16384 },
		//{ NULL, NULL, NULL, DBLITERAL_COLUMN_ALIAS, FALSE, 0 },
		{ NULL, pwszInvalidChars, pwszInvalidFirstChars, DBLITERAL_COLUMN_NAME, TRUE, 127 },
		//{ NULL, NULL, NULL, DBLITERAL_COLUMN_ALIAS, DBLITERAL_CONSTRAINT_NAME , 0 },
		{ NULL, pwszInvalidChars, pwszInvalidFirstChars, DBLITERAL_CORRELATION_NAME, TRUE, 255 },
		{ NULL, pwszInvalidChars, pwszInvalidFirstChars, DBLITERAL_CURSOR_NAME, TRUE, 255 },
		//{ NULL, NULL, NULL, DBLITERAL_ESCAPE_PERCENT_PREFIX, FALSE, 0 },
		//{ NULL, NULL, NULL, DBLITERAL_ESCAPE_PERCENT_SUFFIX, FALSE, 0 },
		//{ NULL, NULL, NULL, DBLITERAL_ESCAPE_UNDERSCORE_PREFIX, FALSE, 0 },
		//{ NULL, NULL, NULL, DBLITERAL_ESCAPE_UNDERSCORE_SUFFIX, FALSE, 0 },
		{ NULL, pwszInvalidChars, pwszInvalidFirstChars, DBLITERAL_INDEX_NAME, TRUE, 255 },
		{ pwszPercent, NULL, NULL, DBLITERAL_LIKE_PERCENT, TRUE, 1 },
		{ pwszUnderline, NULL, NULL, DBLITERAL_LIKE_UNDERSCORE, TRUE, 1 },
		//{ NULL, NULL, NULL, DBLITERAL_PROCEDURE_NAME, FALSE, 0 },
		//{ NULL, NULL, NULL, DBLITERAL_SCHEMA_NAME, FALSE, 0 },
		//{ NULL, NULL, NULL, DBLITERAL_SCHEMA_SEPARATOR, FALSE, 0 },
		{ NULL, pwszInvalidChars, pwszInvalidFirstChars, DBLITERAL_TABLE_NAME, TRUE, 127 },
		{ NULL, NULL, NULL, DBLITERAL_TEXT_COMMAND, TRUE, ~0 },
		{ NULL, pwszInvalidChars, pwszInvalidFirstChars, DBLITERAL_USER_NAME, TRUE, 255 },
		{ NULL, pwszInvalidChars, pwszInvalidFirstChars, DBLITERAL_VIEW_NAME, TRUE, 255 },
		{ pwszQuote, NULL, NULL, DBLITERAL_QUOTE_PREFIX, TRUE, 1 },
		{ pwszQuote, NULL, NULL, DBLITERAL_QUOTE_SUFFIX, TRUE, 1 },
	};

	int size_LiteralInfos = sizeof(LiteralInfos) / sizeof(*LiteralInfos);

	_ptypes provider_types[] = {
		{ L"CHAR",      CCI_U_TYPE_CHAR,      L"length" },
		{ L"VARCHAR",   CCI_U_TYPE_STRING,    L"max length" },
		{ L"NCHAR",			CCI_U_TYPE_NCHAR,			L"length" },
		{ L"VARNCHAR",	CCI_U_TYPE_VARNCHAR,	L"max length" },
		{ L"STRING",		CCI_U_TYPE_STRING,		L"" },
		{ L"BIT",				CCI_U_TYPE_BIT,				L"length" },
		{ L"VARBIT",		CCI_U_TYPE_VARBIT,		L"max length" },
		{ L"BIT VARYING", CCI_U_TYPE_VARBIT,  L"max length" },
		{ L"SMALLINT",	CCI_U_TYPE_SHORT,			L"" },
		{ L"INT",				CCI_U_TYPE_INT,				L"" },
		{ L"SHORT",			CCI_U_TYPE_SHORT,			L"" },
		{ L"BIGINT",		CCI_U_TYPE_BIGINT,		L"" },
		{ L"MONETARY",	CCI_U_TYPE_MONETARY,	L"" },
		{ L"NUMERIC",		CCI_U_TYPE_NUMERIC,		L"precision,scale" },
		{ L"FLOAT",			CCI_U_TYPE_FLOAT,			L"precision" },
		{ L"DOUBLE",		CCI_U_TYPE_DOUBLE,		L"" },
		{ L"DATE",			CCI_U_TYPE_DATE,			L"" },
		{ L"DATETIME",	CCI_U_TYPE_DATETIME,	L"" },
		{ L"TIME",			CCI_U_TYPE_TIME,			L"" },
		{ L"TIMESTAMP",	CCI_U_TYPE_TIMESTAMP,	L"" },
		{ L"SET",				CCI_U_TYPE_SET,				L"" },
		{ L"MULTISET",	CCI_U_TYPE_MULTISET,	L"" },
		{ L"SEQUENCE",	CCI_U_TYPE_SEQUENCE,	L"" },
		{ L"OBJECT",		CCI_U_TYPE_OBJECT,		L"" },
		{ L"RESULTSET",	CCI_U_TYPE_RESULTSET,	L"" },
		{ L"BLOB",			CCI_U_TYPE_BLOB,			L"" },
		{ L"CLOB",			CCI_U_TYPE_CLOB,			L"" },
	};

	int size_provider_types = sizeof(provider_types) / sizeof(*provider_types);
}
