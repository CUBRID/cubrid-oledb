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
#include "test_transaction.h"

///
/// Test Session Rollback
///
bool Test_Rollback()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("DROP TABLE IF EXISTS test1")))
	{
		CloseConnection();
		OutputDebugString(L"Couldn't drop table!");
		return false;
	}

	session.StartTransaction();

	long demodb_tables_count = TableRowsCount(L"db_class");
	if(demodb_tables_count < 0)
	{
		CloseConnection();
		OutputDebugString(L"Couldn't execute count()!");
		return false;
	}

	if(!ExecuteSQL(_T("CREATE TABLE test1(id integer)")))
	{
		CloseConnection();
		OutputDebugString(L"Couldn't create table!");
		return false;
	}

	ATLASSERT(TableRowsCount(L"db_class") == (demodb_tables_count + 1));

	session.Abort();
	ATLASSERT(TableRowsCount(L"db_class") == demodb_tables_count);

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Session Commit
///
bool Test_Commit()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("DROP TABLE IF EXISTS test1")))
	{
		CloseConnection();
		OutputDebugString(L"Couldn't drop table!");
		return false;
	}

	session.StartTransaction();

	long demodb_tables_count = TableRowsCount(L"db_class");
	if(demodb_tables_count < 0)
	{
		CloseConnection();
		OutputDebugString(L"Couldn't execute count()!");
		return false;
	}

	if(!ExecuteSQL(_T("CREATE TABLE test1(id integer)")))
	{
		CloseConnection();
		OutputDebugString(L"Couldn't create table!");
		return false;
	}

	ATLASSERT(TableRowsCount(L"db_class") == (demodb_tables_count + 1));

	session.Commit();

	ATLASSERT(TableRowsCount(L"db_class") == (demodb_tables_count + 1));

	if(!ExecuteSQL(_T("DROP TABLE IF EXISTS test1")))
	{
		CloseConnection();
		OutputDebugString(L"Couldn't drop table!");
		return false;
	}

	TestCleanup(); //Close the connection
	return true; //test case passed
}
