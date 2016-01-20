#include "stdafx.h"

#include <atldbcli.h>
#include <atlconv.h>
#include <oledb.h>
#include <oledberr.h>
#include <atldbcli.h>
#include <atlcoll.h>
#include <atldbsch.h> //schema support

#include "globals.h"
#include "helpers.h"
#include "test_locale.h"

#define _CRT_SECURE_NO_WARNINGS

bool Test_Encoding_Varchar()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("CREATE TABLE `t`(`str` VARCHAR(128))")))
		return false;

	CCommand<CNoAccessor, CNoRowset> cmd;

	#pragma execution_character_set("utf-8") 

	TCHAR *sql_val = L"채식주의자 입니다"; //I'm a vegetarian

	TCHAR *sql = (TCHAR *)malloc(sizeof(char) * 128);
	memset(sql, '\0', sizeof(char) * 128);
	wcscat(sql, L"INSERT INTO `t` VALUES('");
	wcscat(sql, sql_val);
	wcscat(sql, L"')");

	//Get UTF-8 required length
	int utf8_length = WideCharToMultiByte(
  CP_UTF8,				// Convert to UTF-8
  0,							// No special character conversions required 
  sql,						// UTF-16 string to convert from
  -1,							// utf16 is NULL terminated
  NULL,						// Determining correct output buffer size
  0,							// Determining correct output buffer size
  NULL,						// Must be NULL for CP_UTF8
  NULL);					// Must be NULL for CP_UTF8

	if (utf8_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	//Allocate space for the UTF-8 string
	char* sql_utf8 = (char *)malloc(sizeof(char) * utf8_length);

	utf8_length = WideCharToMultiByte(
		CP_UTF8,			// Convert to UTF-8
		0,						// No special character conversions required 
		sql,					// UTF-16 string to convert from
		-1,						// utf16 is NULL terminated
		sql_utf8,			// UTF-8 output buffer
		utf8_length,	// UTF-8 output buffer size
		NULL,					// Must be NULL for CP_UTF8
		NULL);				// Must be NULL for CP_UTF8

	if (utf8_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	HRESULT hr = cmd.Open(session, sql_utf8);
	if(FAILED(hr))
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}
	
	cmd.Close();

	//Read data and verify

	CCommand<CDynamicAccessor, CRowset> cmd2;

	hr = cmd2.Open(session, L"SELECT * FROM `t`");
	if(FAILED(hr))
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	hr = cmd2.MoveNext();
	if(FAILED(hr))
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	//Verify column metadata
	ATLASSERT(cmd2.GetColumnCount() == 1);
	ATLASSERT(EQ_STR(cmd2.GetColumnName(1),  L"str") == true);
	DBTYPE dbType;
	cmd2.GetColumnType(1, &dbType);
	ATLASSERT(dbType == DBTYPE_STR);

	DBLENGTH len;
	cmd2.GetLength(1, &len);
	void *val = cmd2.GetValue(1);
	char *ptr_val = (char *)val;

	//Get wide char required length
	int wchar_length = MultiByteToWideChar(
    CP_UTF8,								// convert from UTF-8
    MB_ERR_INVALID_CHARS,		// error on invalid chars
    ptr_val,								// source UTF-8 string
    utf8_length,						// total length of source UTF-8 string, in CHAR's (= bytes), including end-of-string \0
    NULL,										// unused - no conversion done in this step
    0);											// request size of destination buffer, in WCHAR's

	if (wchar_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	//Allocate space for the wide char string
	TCHAR *my_ptr_val = (TCHAR *)malloc(wchar_length * sizeof(TCHAR));
	memset(my_ptr_val, '\0', wchar_length * sizeof(TCHAR));

	wchar_length = MultiByteToWideChar(
		CP_UTF8,												// convert from UTF-8
		MB_ERR_INVALID_CHARS,						// error on invalid chars
		ptr_val,												// source UTF-8 string
		utf8_length,										// total length of source UTF-8 string, in char's (= bytes), and including end-of-string \0
		my_ptr_val,											// destination buffer
		wchar_length * sizeof(TCHAR));	// size of destination buffer
	
	if (wchar_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	ATLASSERT(EQ_STR(my_ptr_val,  sql_val) == true);

	if(!ExecuteSQL(_T("DROP TABLE `t`")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_Encoding_Char()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("CREATE TABLE `t`(`str` CHAR(128))")))
		return false;

	CCommand<CNoAccessor, CNoRowset> cmd;

	#pragma execution_character_set("utf-8") 

	TCHAR *sql_val = L"채식주의자 입니다"; //I'm a vegetarian

	TCHAR *sql = (TCHAR *)malloc(sizeof(char) * 128);
	memset(sql, '\0', sizeof(char) * 128);
	wcscat(sql, L"INSERT INTO `t` VALUES('");
	wcscat(sql, sql_val);
	wcscat(sql, L"')");

	//Get UTF-8 required length
	int utf8_length = WideCharToMultiByte(
  CP_UTF8,				// Convert to UTF-8
  0,							// No special character conversions required 
  sql,						// UTF-16 string to convert from
  -1,							// utf16 is NULL terminated
  NULL,						// Determining correct output buffer size
  0,							// Determining correct output buffer size
  NULL,						// Must be NULL for CP_UTF8
  NULL);					// Must be NULL for CP_UTF8

	if (utf8_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	//Allocate space for the UTF-8 string
	char* sql_utf8 = (char *)malloc(sizeof(char) * utf8_length);

	utf8_length = WideCharToMultiByte(
		CP_UTF8,			// Convert to UTF-8
		0,						// No special character conversions required 
		sql,					// UTF-16 string to convert from
		-1,						// utf16 is NULL terminated
		sql_utf8,			// UTF-8 output buffer
		utf8_length,	// UTF-8 output buffer size
		NULL,					// Must be NULL for CP_UTF8
		NULL);				// Must be NULL for CP_UTF8

	if (utf8_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	HRESULT hr = cmd.Open(session, sql_utf8);
	if(FAILED(hr))
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}
	
	cmd.Close();

	//Read data and verify

	CCommand<CDynamicAccessor, CRowset> cmd2;

	hr = cmd2.Open(session, L"SELECT * FROM `t`");
	if(FAILED(hr))
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	hr = cmd2.MoveNext();
	if(FAILED(hr))
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	//Verify column metadata
	ATLASSERT(cmd2.GetColumnCount() == 1);
	ATLASSERT(EQ_STR(cmd2.GetColumnName(1),  L"str") == true);
	DBTYPE dbType;
	cmd2.GetColumnType(1, &dbType);
	ATLASSERT(dbType == DBTYPE_STR);

	DBLENGTH len;
	cmd2.GetLength(1, &len);
	void *val = cmd2.GetValue(1);
	char *ptr_val = (char *)val;

	//Get wide char required length
	int wchar_length = MultiByteToWideChar(
    CP_UTF8,								// convert from UTF-8
    MB_ERR_INVALID_CHARS,		// error on invalid chars
    ptr_val,								// source UTF-8 string
    utf8_length,						// total length of source UTF-8 string, in CHAR's (= bytes), including end-of-string \0
    NULL,										// unused - no conversion done in this step
    0);											// request size of destination buffer, in WCHAR's

	if (wchar_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	//Allocate space for the wide char string
	TCHAR *my_ptr_val = (TCHAR *)malloc(128 * sizeof(TCHAR));
	memset(my_ptr_val, '\0', 128 * sizeof(TCHAR));

	wchar_length = MultiByteToWideChar(
		CP_UTF8,												// convert from UTF-8
		MB_ERR_INVALID_CHARS,						// error on invalid chars
		ptr_val,												// source UTF-8 string
		utf8_length,										// total length of source UTF-8 string, in char's (= bytes), and including end-of-string \0
		my_ptr_val,											// destination buffer
		wchar_length * sizeof(TCHAR));	// size of destination buffer
	
	if (wchar_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	TCHAR *padded_value = (TCHAR *)malloc(128 * sizeof(TCHAR));
	memset(padded_value, '\0', 128 * sizeof(TCHAR));
	wcscat(padded_value, sql_val);
	wcscat(padded_value, L"                           ");
	ATLASSERT(EQ_STR(my_ptr_val,  padded_value) == true);

	if(!ExecuteSQL(_T("DROP TABLE `t`")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}


class CMyAccessor
{
public:
	char m_str[8912];

	BEGIN_COLUMN_MAP(CMyAccessor)
		COLUMN_ENTRY(1, m_str)
	END_COLUMN_MAP()
};


bool Test_Encoding_String()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("CREATE TABLE `t`(`str` STRING)")))
		return false;
	
	CCommand<CNoAccessor, CNoRowset> cmd;

	#pragma execution_character_set("utf-8") 

	TCHAR *sql_val = L"Одного языка никогда недостаточно"; //One language is never enough

	TCHAR *sql = (TCHAR *)malloc(sizeof(char) * 128);
	memset(sql, '\0', sizeof(char) * 128);
	wcscat(sql, L"INSERT INTO `t` VALUES('");
	wcscat(sql, sql_val);
	wcscat(sql, L"')");

	//Get UTF-8 required length
	int utf8_length = WideCharToMultiByte(
  CP_UTF8,				// Convert to UTF-8
  0,							// No special character conversions required 
  sql,						// UTF-16 string to convert from
  -1,							// utf16 is NULL terminated
  NULL,						// Determining correct output buffer size
  0,							// Determining correct output buffer size
  NULL,						// Must be NULL for CP_UTF8
  NULL);					// Must be NULL for CP_UTF8

	if (utf8_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	//Allocate space for the UTF-8 string
	char* sql_utf8 = (char *)malloc(sizeof(char) * utf8_length);

	utf8_length = WideCharToMultiByte(
		CP_UTF8,			// Convert to UTF-8
		0,						// No special character conversions required 
		sql,					// UTF-16 string to convert from
		-1,						// utf16 is NULL terminated
		sql_utf8,			// UTF-8 output buffer
		utf8_length,	// UTF-8 output buffer size
		NULL,					// Must be NULL for CP_UTF8
		NULL);				// Must be NULL for CP_UTF8

	if (utf8_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	HRESULT hr = cmd.Open(session, sql_utf8);
	if(FAILED(hr))
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}
	
	cmd.Close();

	//Read data and verify

	CCommand <CAccessor<CMyAccessor>> cmd2;

	hr = cmd2.Open(session, L"SELECT * FROM `t`");
	if(FAILED(hr))
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	hr = cmd2.MoveNext();
	if(FAILED(hr))
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	void *val = cmd2.m_str;
	char *ptr_val = (char *)val;

	//Get wide char required length
	int wchar_length = MultiByteToWideChar(
    CP_UTF8,								// convert from UTF-8
    MB_ERR_INVALID_CHARS,		// error on invalid chars
    ptr_val,								// source UTF-8 string
    utf8_length,						// total length of source UTF-8 string, in CHAR's (= bytes), including end-of-string \0
    NULL,										// unused - no conversion done in this step
    0);											// request size of destination buffer, in WCHAR's

	if (wchar_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	//Allocate space for the wide char string
	TCHAR *my_ptr_val = (TCHAR *)malloc(128 * sizeof(TCHAR));
	memset(my_ptr_val, '\0', 128 * sizeof(TCHAR));

	wchar_length = MultiByteToWideChar(
		CP_UTF8,												// convert from UTF-8
		MB_ERR_INVALID_CHARS,						// error on invalid chars
		ptr_val,												// source UTF-8 string
		utf8_length,										// total length of source UTF-8 string, in char's (= bytes), and including end-of-string \0
		my_ptr_val,											// destination buffer
		wchar_length * sizeof(TCHAR));	// size of destination buffer
	
	if (wchar_length == 0) 
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}
	
	ATLASSERT(EQ_STR(my_ptr_val,  sql_val) == true);

	cmd2.Close();

	if(!ExecuteSQL(_T("DROP TABLE `t`")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

