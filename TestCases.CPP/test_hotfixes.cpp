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
#include "test_datatypes.h"
#include "test_hotfixes.h"

///
/// Test SELECT of CUBRID data types with an Dynamic Accessor-type Command
///
/// http://jira.cubrid.org/secure/IssueNavigator.jspa?mode=hide
bool Test_Hotfixes_DataTypes()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	ExecuteSQL(_T("DROP TABLE `t`"));

	TCHAR *sql_create = L"CREATE TABLE `t`(`c1` char, `c2` datetime, `c3` double, `c4` float, `c5` bigint);";
	if(!ExecuteSQL(sql_create))
		return false;
	
	TCHAR *sql_insert = L"insert into `t` (`c1`, `c2`, `c3`, `c4`, `c5`) \
											 values ('a', DATETIME'2008-10-31 13:15:45.000', 12345.12345, 54321.54321, 123451234512345);";
	if(!ExecuteSQL(sql_insert))
		return false;
	
	CCommand<CDynamicAccessor, CRowset> cmd;
	char *sql = "SELECT * FROM `t`";
	DBTYPE pType;

	HRESULT hr = cmd.Open(session, sql);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the <t> table!");
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}
	
	if(!cmd.MoveNext() == S_OK)
	{
		ExecuteSQL(_T("DROP TABLE `t`"));
		CloseConnection();
		return false;
	}

	void *val = cmd.GetValue(1); //Char
	char *ptr_val = (char *)val;
	ATLASSERT(strcmp(ptr_val, "a") == 0);
	cmd.GetColumnType(1, &pType);
	ATLASSERT(pType == DBTYPE_STR);

	val = cmd.GetValue(2); //Datetime
	CUBRIDDateTime *dt_val = (CUBRIDDateTime *)val;
	ATLASSERT(dt_val->yr == 2008);
	ATLASSERT(dt_val->mon == 10);
	ATLASSERT(dt_val->day == 31);
	ATLASSERT(dt_val->hh == 13);
	ATLASSERT(dt_val->mm == 15);
	ATLASSERT(dt_val->ss == 45);
	cmd.GetColumnType(2, &pType);
	ATLASSERT(pType == DBTYPE_DBTIMESTAMP);

	val = cmd.GetValue(3); //Double
	double *dbl_val = (double *)val;
	ATLASSERT(*dbl_val == 12345.123449999999);
	cmd.GetColumnType(3, &pType);
	ATLASSERT(pType == DBTYPE_R8);

	val = cmd.GetValue(4);  //Float
	float *flt_val = (float *)val;
	ATLASSERT(*flt_val == 54321.543f);
	cmd.GetColumnType(4, &pType);
	ATLASSERT(pType == DBTYPE_R4);

	val = cmd.GetValue(5);  //Bigint
	_int64 *int64_val = (_int64 *)val;
	ATLASSERT(*int64_val == 123451234512345l);
	cmd.GetColumnType(5, &pType);
	ATLASSERT(pType == DBTYPE_I8);

	cmd.Close();

	if(!ExecuteSQL(_T("DROP TABLE `t`")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

