#include "stdafx.h"

#include <atldbcli.h>
#include <atlconv.h>
#include <atldbcli.h>
#include <oledb.h>
#include <oledberr.h>
#include <atldbcli.h>
#include <atlcoll.h>
#include <atldbsch.h> //schema support

#include "globals.h"
#include "helpers.h"
#include "test_datatypes.h"

class CData
{
public:
	TCHAR m_c1[10];
	BYTE m_c2;
	TCHAR m_c3[30];
	TCHAR m_c4[30];
	double m_c5;
	double m_c6;
	float m_c7;
	TCHAR m_c8[30];
	TCHAR m_c9[30];
	double m_c10;

	//Output binding map.
	BEGIN_COLUMN_MAP(CData)
		COLUMN_ENTRY(1, m_c1)
		COLUMN_ENTRY(2, m_c2)
		COLUMN_ENTRY(3, m_c3)
		COLUMN_ENTRY(4, m_c4)
		COLUMN_ENTRY(5, m_c5)
		COLUMN_ENTRY(6, m_c6)
		COLUMN_ENTRY(7, m_c7)
		COLUMN_ENTRY(8, m_c8)
		COLUMN_ENTRY(9, m_c9)
		COLUMN_ENTRY(10, m_c10)
	END_COLUMN_MAP()
};

///
/// Test SELECT of various CUBRID data types with an Dynamic Accessor-type Command
///
bool Test_Dynamic_DataTypes_SELECT()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	ExecuteSQL(_T("DROP TABLE `t`"));

	TCHAR *sql_create = L"CREATE TABLE `t`(`c1` char(10),`c2` bit(1),`c3` date,`c4` datetime,`c5` double,`c6` monetary, \
											`c7` float,`c8` time,`c9` timestamp,`c10` numeric(10,5));";
	if(!ExecuteSQL(sql_create))
		return false;
	
	TCHAR *sql_insert = L"insert into `t` (`c1`, `c2`, `c3`, `c4`, `c5`, `c6`, `c7`, `c8`, `c9`, `c10`) \
											 values ('abc', B'1', DATE'10/31/2008', DATETIME'2008-10-31 13:15:45.000', \
											 12345.12345, 12345, 12345.12345, TIME'13:15:45', TIMESTAMP'10/31/2008 13:15:45', 12345.12345);";
	if(!ExecuteSQL(sql_insert))
		return false;
	
	CCommand<CDynamicAccessor, CRowset> cmd;
	char *sql = "SELECT * FROM `t`";

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
	ATLASSERT(strcmp(ptr_val, "abc       ") == 0);

	val = cmd.GetValue(2); //Bit
	int *byte_val = (int *)val;
	ATLASSERT(*byte_val == 1);

	val = cmd.GetValue(3); //Date
	CUBRIDDate *d_val = (CUBRIDDate *)val;
	ATLASSERT(d_val->yr == 2008);
	ATLASSERT(d_val->mon == 10);
	ATLASSERT(d_val->day == 31);

	val = cmd.GetValue(4); //Datetime
	CUBRIDDateTime *dt_val = (CUBRIDDateTime *)val;
	ATLASSERT(dt_val->yr == 2008);
	ATLASSERT(dt_val->mon == 10);
	ATLASSERT(dt_val->day == 31);
	ATLASSERT(dt_val->hh == 13);
	ATLASSERT(dt_val->mm == 15);
	ATLASSERT(dt_val->ss == 45);

	val = cmd.GetValue(5);
	double *dbl_val = (double *)val;
	ATLASSERT(*dbl_val == 12345.123449999999);

	val = cmd.GetValue(6); //Monetary
	double *f_val = (double *)val;
	ATLASSERT(*f_val == 12345.00);

	val = cmd.GetValue(7);
	float *flt_val = (float *)val;
	ATLASSERT(*flt_val == 12345.123f);

	val = cmd.GetValue(8); //Time
	CUBRIDTime *t_val = (CUBRIDTime *)val;
	ATLASSERT(t_val->hh == 13);
	ATLASSERT(t_val->mm == 15);
	ATLASSERT(t_val->ss == 45);

	val = cmd.GetValue(9); //Timestamp
	CUBRIDTimestamp *ts_val = (CUBRIDTimestamp *)val;
	ATLASSERT(ts_val->yr == 2008);
	ATLASSERT(ts_val->mon == 10);
	ATLASSERT(ts_val->day == 31);
	ATLASSERT(ts_val->hh == 13);
	ATLASSERT(ts_val->mm == 15);
	ATLASSERT(ts_val->ss == 45);

	val = cmd.GetValue(10); //Numeric
	dbl_val = (double *)val;
	ATLASSERT(*dbl_val == 2.8461664895612178e-307);

	cmd.Close();

	if(!ExecuteSQL(_T("DROP TABLE `t`")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_DataTypes_INSERT()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	ExecuteSQL(_T("DROP TABLE IF EXISTS `t`"));

	TCHAR *sql_create = L"CREATE TABLE `t`(`c1` char(10),`c2` bit(6),`c3` date,`c4` datetime,`c5` double,`c6` monetary, \
											`c7` float,`c8` time,`c9` timestamp,`c10` numeric(10,5));";
	if(!ExecuteSQL(sql_create))
		return false;

	CCommand<CAccessor<CData>> types_cmd;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);
	
	HRESULT hr;
	char *sql = "SELECT * FROM t";

	hr = types_cmd.Open(session, sql, &propset);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the <t> table!");
		CloseConnection();
		return false;
	}
	
	wcscpy(types_cmd.m_c1, L"abc");
	types_cmd.m_c2 = 35;
	wcscpy(types_cmd.m_c3, L"10/31/2008");
	wcscpy(types_cmd.m_c4, L"2008-10-31 13:15:45.000");
	types_cmd.m_c5 = 12345.12345;
	types_cmd.m_c6 = 12345;
	types_cmd.m_c7 = 12345.12345f;
	wcscpy(types_cmd.m_c8, L"13:15:45");
	wcscpy(types_cmd.m_c9, L"10/31/2008 13:15:45");
	types_cmd.m_c10 = 12345.12345;
	
	hr = types_cmd.Insert();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not insert the data!");
		types_cmd.Close();
		CloseConnection();
		return false;
	}
	
	types_cmd.Close();

	CCommand<CDynamicAccessor, CRowset> cmd;
	char *sql2 = "SELECT * FROM `t`";

	hr = cmd.Open(session, sql2);
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
	ATLASSERT(strcmp(ptr_val, "abc       ") == 0);

	val = cmd.GetValue(2); //Bit
	int *byte_val = (int *)val;
	ATLASSERT(*byte_val == 35);

	val = cmd.GetValue(3); //Date
	CUBRIDDate *d_val = (CUBRIDDate *)val;
	ATLASSERT(d_val->yr == 2008);
	ATLASSERT(d_val->mon == 10);
	ATLASSERT(d_val->day == 31);

	val = cmd.GetValue(4); //Datetime
	CUBRIDDateTime *dt_val = (CUBRIDDateTime *)val;
	ATLASSERT(dt_val->yr == 2008);
	ATLASSERT(dt_val->mon == 10);
	ATLASSERT(dt_val->day == 31);
	ATLASSERT(dt_val->hh == 13);
	ATLASSERT(dt_val->mm == 15);
	ATLASSERT(dt_val->ss == 45);

	val = cmd.GetValue(5);
	double *dbl_val = (double *)val;
	ATLASSERT(*dbl_val == 12345.123449999999);

	val = cmd.GetValue(6); //Monetary
	double *f_val = (double *)val;
	ATLASSERT(*f_val == 12345.00);

	val = cmd.GetValue(7);
	float *flt_val = (float *)val;
	ATLASSERT(*flt_val == 12345.123f);

	val = cmd.GetValue(8); //Time
	CUBRIDTime *t_val = (CUBRIDTime *)val;
	ATLASSERT(t_val->hh == 13);
	ATLASSERT(t_val->mm == 15);
	ATLASSERT(t_val->ss == 45);

	val = cmd.GetValue(9); //Timestamp
	CUBRIDTimestamp *ts_val = (CUBRIDTimestamp *)val;
	ATLASSERT(ts_val->yr == 2008);
	ATLASSERT(ts_val->mon == 10);
	ATLASSERT(ts_val->day == 31);
	ATLASSERT(ts_val->hh == 13);
	ATLASSERT(ts_val->mm == 15);
	ATLASSERT(ts_val->ss == 45);

	val = cmd.GetValue(10); //Numeric
	dbl_val = (double *)val;
	ATLASSERT(*dbl_val == 2.8461664895612178e-307);

	cmd.Close();

	if(!ExecuteSQL(_T("DROP TABLE `t`")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}


bool Test_Dynamic_DataTypes_INSERT()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	ExecuteSQL(_T("DROP TABLE IF EXISTS t"));

	TCHAR *sql_create = L"CREATE TABLE `t`(`c1` char(10),`c2` bit(1),`c3` date,`c4` datetime,`c5` double,`c6` monetary, \
											`c7` float,`c8` time,`c9` timestamp,`c10` numeric(10,5));";
	if(!ExecuteSQL(sql_create))
		return false;

	CCommand<CDynamicAccessor, CRowset> types_cmd;
	char *sql = "SELECT * FROM t";

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	HRESULT hr = types_cmd.Open(session, sql, &propset);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the <t> table!");
		ExecuteSQL(_T("DROP TABLE t"));
		CloseConnection();
		return false;
	}
	
	char* c1="abc";
	BYTE c2 = 1;
	CUBRIDDate c3 = { 2008, 10, 31 };
	CUBRIDDateTime c4 = { 2008, 10, 31, 13, 15, 45, 000 };
	double c5 = 12345.12345;
	double c6 = 12345;
	float c7 = 12345.12345f;
	CUBRIDTime c8 = { 13, 15, 45 };
	CUBRIDTimestamp c9 = { 2008, 10, 31, 13, 15, 45 };
	CUBRIDNumeric c10 = { 4, 2, 1, (BYTE)1234 };
	
	hr = types_cmd.SetValue(1, c1);
	hr = types_cmd.SetValue(2,c2);
	hr = types_cmd.SetValue(3,c3);
	hr = types_cmd.SetValue(4,c4);
	hr = types_cmd.SetValue(5,c5);
	hr = types_cmd.SetValue(6,c6);
	hr = types_cmd.SetValue(7,c7);
	hr = types_cmd.SetValue(8,c8);
	hr = types_cmd.SetValue(9,c9);
	hr = types_cmd.SetValue(10,c10);

	if(FAILED(hr))
	{
		OutputDebugString(L"Could not set the data!");
		types_cmd.Close();
		CloseConnection();
		return false;
	}
	
	hr = types_cmd.Insert();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not insert the data!");
		types_cmd.Close();
		CloseConnection();
		return false;
	}
	types_cmd.Close();

	CCommand<CDynamicAccessor, CRowset> cmd;
	char *sql2 = "SELECT * FROM `t`";

	hr = cmd.Open(session, sql2);
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
	ATLASSERT(strcmp(ptr_val, "abc       ") == 0);

	val = cmd.GetValue(2); //Bit
	int *byte_val = (int *)val;
	ATLASSERT(*byte_val == 1);

	val = cmd.GetValue(3); //Date
	CUBRIDDate *d_val = (CUBRIDDate *)val;
	ATLASSERT(d_val->yr == 2008);
	ATLASSERT(d_val->mon == 10);
	ATLASSERT(d_val->day == 31);

	val = cmd.GetValue(4);//Datatime
	CUBRIDDateTime *dt_val = (CUBRIDDateTime *)val;
	ATLASSERT(dt_val->yr == 2008);
	ATLASSERT(dt_val->mon == 10);
	ATLASSERT(dt_val->day == 31);
	ATLASSERT(dt_val->hh == 13);
	ATLASSERT(dt_val->mm == 15);
	ATLASSERT(dt_val->ss == 45);

	val = cmd.GetValue(5);
	double *dbl_val = (double *)val;
	ATLASSERT(*dbl_val == 12345.123449999999);

	val = cmd.GetValue(6); //Monetary
	double *f_val = (double *)val;
	ATLASSERT(*f_val == 12345.00);

	val = cmd.GetValue(7);
	float *flt_val = (float *)val;
	ATLASSERT(*flt_val == 12345.123f);

	val = cmd.GetValue(8); //Time
	CUBRIDTime *t_val = (CUBRIDTime *)val;
	ATLASSERT(t_val->hh == 13);
	ATLASSERT(t_val->mm == 15);
	ATLASSERT(t_val->ss == 45);

	val = cmd.GetValue(9); //Timestamp
	CUBRIDTimestamp *ts_val = (CUBRIDTimestamp *)val;
	ATLASSERT(ts_val->yr == 2008);
	ATLASSERT(ts_val->mon == 10);
	ATLASSERT(ts_val->day == 31);
	ATLASSERT(ts_val->hh == 13);
	ATLASSERT(ts_val->mm == 15);
	ATLASSERT(ts_val->ss == 45);

	val = cmd.GetValue(10); //Numeric
	dbl_val = (double *)val;
	ATLASSERT(*dbl_val == 1.740699705293e-311);

	cmd.Close();

	if(!ExecuteSQL(_T("DROP TABLE t")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}