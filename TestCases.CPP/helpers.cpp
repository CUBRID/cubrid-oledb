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

bool OpenConnection()
{
	HRESULT hr = ds.OpenFromInitializationString(connectString, false);
	if(SUCCEEDED(hr))
	{
		hr = session.Open(ds);
		if(!SUCCEEDED(hr))
		{
			ds.Close();
			return false;
		}
		else
		{
			return true;
		}
	}

	OutputDebugString(L"Couldn't open the connection!");
	return false;
}

bool OpenConnection(DBPROPSET *ps)
{
	HRESULT hr = ds.OpenFromInitializationString(connectString, false);
	if(SUCCEEDED(hr))
	{
		hr = session.Open(ds, ps);
		if(!SUCCEEDED(hr))
		{
			ds.Close();
			return false;
		}
		else
		{
			return true;
		}
	}

	OutputDebugString(L"Couldn't open the connection!");
	return false;
}

void CloseConnection()
{
	session.Close();
	ds.Close();
}

bool TestSetup()
{
	if(!OpenConnection())
		return false;

	//Perform other initialization procedures here, if needed
	//...
	ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'"));
	ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'YYY'"));
	ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'abc'"));

	ExecuteSQL(_T("DROP TABLE IF EXISTS `t`"));

	return true;
}

bool TestSetup(DBPROPSET *ps)
{
	if(!OpenConnection(ps))
		return false;

	//Perform other initialization procedures here, if needed
	//...

	return true;
}

void TestCleanup()
{
	try
	{
		CloseConnection();
	}
	catch(...)
	{ }
	//Perform other cleanup procedures here, if needed
	//...
}

///
/// Returns the count of records in a table
///
long TableRowsCount(TCHAR *tableName)
{
	CString select;
	select.Format(_T("SELECT COUNT(*) FROM %s"), tableName);
	long res = -1;

	CCommand<CDynamicAccessor, CRowset> cmd;
	HRESULT hr = cmd.Open(session, select);
	if(FAILED(hr))
	{
		OUTPUT_DEBUG_STRING_ARG(_T("Couldn't open the table: %s"), tableName);
		return -1;
	}

	if(cmd.MoveNext() == S_OK)
	{
		res = *(long*)cmd.GetValue(1);
		cmd.Close();
		return res;
	}

	return -1;
}

///
/// Execute a SQL statement
///
bool ExecuteSQL(TCHAR *sql)
{
	CCommand<CNoAccessor, CNoRowset> cmd;

	HRESULT hr = cmd.Open(session, sql);
	if (FAILED(hr))
	{
		OUTPUT_DEBUG_STRING_ARG(_T("Couldn't execute command: %s"), sql);
		return false;
	}
	cmd.Close();

	return true;
}

///
/// Drops the specified table
///
bool DropTable(TCHAR *tableName)
{
	bool ret = false;
	CString str;
	str.Format(_T("drop table if exists %s"), tableName);

	ret = ExecuteSQL(str.GetBuffer());
	str.ReleaseBuffer();

	return ret;
}

void SetUpdateableRowsetProperties(CDBPropSet* pPropSet)
{
  pPropSet->AddProperty(DBPROP_CANFETCHBACKWARDS, true, DBPROPOPTIONS_OPTIONAL);
  pPropSet->AddProperty(DBPROP_CANSCROLLBACKWARDS, true, DBPROPOPTIONS_OPTIONAL);
  pPropSet->AddProperty(DBPROP_IRowsetUpdate,true,DBPROPOPTIONS_OPTIONAL);
  pPropSet->AddProperty(DBPROP_IRowsetChange, true, DBPROPOPTIONS_OPTIONAL);
  pPropSet->AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE);
}