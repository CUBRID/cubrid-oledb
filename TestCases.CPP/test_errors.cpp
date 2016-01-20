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
#include "test_errors.h"

///
/// Test Provider errors
///
bool Test_Errors()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CCommand<CDynamicAccessor, CRowset> cmd;

	TCHAR *sql_err = L"select xyz from `code`";

	HRESULT hr = cmd.Open(session, sql_err);
	if (FAILED(hr))
	{
		ATLASSERT(hr == DB_E_ERRORSINCOMMAND);
	}
	else
	{
		CloseConnection();
		return false;
	}

	cmd.Close();
	cmd.ReleaseCommand();

	TCHAR *sql = L"select * from `code`";

	hr = cmd.Open(session, sql);
	if (FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = cmd.MoveNext();
	if (FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = cmd.MoveLast();
	if (FAILED(hr))
	{
		ATLASSERT(hr == DB_E_CANTSCROLLBACKWARDS);
	}
	else
	{
		CloseConnection();
		return false;
	}

	hr = cmd.Update();
	if (FAILED(hr))
	{
		ATLASSERT(hr == E_NOINTERFACE); //No support for UPDATE in the rowset
	}
	else
	{
		CloseConnection();
		return false;
	}

	hr = cmd.Insert();
	if (FAILED(hr))
	{
		ATLASSERT(hr == E_NOINTERFACE); //No support for UPDATE in the rowset
	}
	else
	{
		CloseConnection();
		return false;
	}

	hr = cmd.MoveNext();
	if (FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	//TODO Add more error scenarios

	TestCleanup(); //Close the connection
	return true; //test case passed
}
