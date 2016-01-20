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
#include "test_other_accessors.h"

///
/// Test XMLAccessor 
///
bool Test_XMLAccessor()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CTable<CXMLAccessor> rs;
	HRESULT hr = rs.Open(session, L"nation");
	if(FAILED(hr))
		return false;

	CStringW strColumnInfo;
	rs.GetXMLColumnData(strColumnInfo);
	CString str;
	str.Format(_T("%s\n"), strColumnInfo);
	ATLASSERT(str.GetLength() == 180);

	hr = rs.MoveNext();
	if(SUCCEEDED(hr) && hr != DB_S_ENDOFROWSET)
	{
		CStringW strRowData;
		rs.GetXMLRowData(strRowData);
		CString str;
		str.Format(_T("%s\n"), strRowData);
		ATLASSERT(str.GetLength() == 102);
	}

	rs.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
};


class CUserTable
{
public:
	char m_szName[10];
	int m_nID;

	BEGIN_COLUMN_MAP(CUserTable)
		COLUMN_ENTRY(1, m_szName)
		COLUMN_ENTRY(2, m_nID)
	END_COLUMN_MAP()
};

bool Test_CArrayRowset()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	TCHAR *sql = L"select 'Beograd', 3 from db_root union select 'Bucharest', 2 from db_root";
	CCommand< CAccessor<CUserTable>, CArrayRowset > cmd;

	HRESULT hr = cmd.Open(session, sql);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	ATLASSERT(strcmp(cmd[0].m_szName, "Beograd   ") == 0);
	ATLASSERT(cmd[0].m_nID == 3);

	ATLASSERT(strcmp(cmd[1].m_szName, "Bucharest ") == 0);
	ATLASSERT(cmd[1].m_nID == 2);

	cmd.Close();   

	TestCleanup(); //Close the connection
	return true; //test case passed
}

class CBulkUserTable
{
public:
	char m_szCode[4];

	BEGIN_COLUMN_MAP(CBulkUserTable)
		COLUMN_ENTRY(1, m_szCode)
	END_COLUMN_MAP()
};

bool Test_CBulkRowset()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	TCHAR *sql = L"select `code` from nation";
	CCommand< CAccessor<CBulkUserTable>, CBulkRowset > cmd;

	cmd.SetRows(20);

	HRESULT hr = cmd.Open(session, sql);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	cmd.MoveFirst();
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = cmd.MoveNext();
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	ATLASSERT(strcmp(cmd.m_szCode, "KIR") == 0);

	hr = cmd.MoveNext();
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	ATLASSERT(strcmp(cmd.m_szCode, "SCG") == 0);

	hr = cmd.MoveFirst();
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	ATLASSERT(strcmp(cmd.m_szCode, "SRB") == 0);

	cmd.Close();   

	TestCleanup(); //Close the connection
	return true; //test case passed
}
