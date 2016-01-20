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
#include "test_parameters.h"

///
/// Accessor class for the `nation` table in the demodb database
///
class CNationAccessor
{
public:
	TCHAR m_Code[4];
	TCHAR m_Name[41];
	TCHAR m_Continent[11];
	TCHAR m_Capital[31];

	BEGIN_COLUMN_MAP(CNationAccessor)
		COLUMN_ENTRY(1, m_Code)
		COLUMN_ENTRY(2, m_Name)
		COLUMN_ENTRY(3, m_Continent)
		COLUMN_ENTRY(4, m_Capital)
	END_COLUMN_MAP()

	BEGIN_PARAM_MAP(CNationAccessor)
		SET_PARAM_TYPE(DBPARAMIO_INPUT)
		COLUMN_ENTRY(1, m_Code)
	END_PARAM_MAP()
};

class CNationAccessorEx
{
public:
	TCHAR m_Code[4];
	TCHAR m_Name[41];
	TCHAR m_Continent[11];
	TCHAR m_Capital[31];

	BEGIN_COLUMN_MAP(CNationAccessor)
		COLUMN_ENTRY(1, m_Code)
		COLUMN_ENTRY(2, m_Name)
		COLUMN_ENTRY(3, m_Continent)
		COLUMN_ENTRY(4, m_Capital)
	END_COLUMN_MAP()

	BEGIN_PARAM_MAP(CNationAccessor)
		SET_PARAM_TYPE(DBPARAMIO_INPUT)
		COLUMN_ENTRY(1, m_Code)
	END_PARAM_MAP()

  DEFINE_COMMAND_EX(CNationAccessorEx, L"SELECT * FROM nation where code = ?")
};

///
/// Test Parameter Input for SELECT
///
bool Test_Simple_Parameter()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	HRESULT hr;

	CCommand <CAccessor<CNationAccessorEx>> nation;

	// Set the parameter for the query
	wcscpy(nation.m_Code, L"YEM");

	hr = nation.Open(session);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the <nation> table!");
		CloseConnection();
		return false;
	}

	ATLASSERT(nation.HasParameters() == true);

	hr = nation.MoveNext();
	if (FAILED(hr))
	{
		CloseConnection();
		nation.Close();
		return false;
	}

	ATLASSERT(EQ_STR(nation.m_Code, L"YEM") == true);

	nation.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test DynamicAccesssor with Parameters
///
bool Test_UseDynamicParameterAccessor()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("DROP TABLE IF EXISTS t")))
		return false;	
	if(!ExecuteSQL(_T("CREATE TABLE t(id INT)")))
		return false;
	if(!ExecuteSQL(_T("INSERT INTO t VALUES(1)")))
		return false;
	if(!ExecuteSQL(_T("INSERT INTO t VALUES(2)")))
		return false;
	if(!ExecuteSQL(_T("INSERT INTO t VALUES(10)")))
		return false;

	CCommand<CDynamicParameterAccessor, CRowset, CMultipleResults> rs;

	HRESULT hr;
	int p_id = 2;

	hr = rs.Create(session, L"SELECT * FROM t WHERE id=?");
	if (FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = rs.Prepare();
	if (FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	CSimpleValArray<DBPARAMBINDINFO> vParamInfo;
	CSimpleValArray<DB_UPARAMS> vParams;

	DBPARAMBINDINFO dbpi;
	DB_UPARAMS dbp;

	memset((void*)&dbpi, 0, sizeof(dbpi));
	//dbpi.dwFlags |= DBPARAMFLAGS_ISNULLABLE;
	dbpi.dwFlags = DBPARAMFLAGS_ISINPUT;
	dbpi.pwszName = NULL;
	dbpi.ulParamSize = sizeof(int);
	dbpi.pwszDataSourceType = L"DBTYPE_I4";
	dbpi.bPrecision = 0;
	dbpi.bScale = 0;

	vParamInfo.Add(dbpi);

	dbp=1;
	vParams.Add(dbp);

	hr = rs.SetParameterInfo(1, vParams.GetData(), vParamInfo.GetData()); 
	if (FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = rs.SetParam((ULONG)1, &p_id);
	if (FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = rs.Open(NULL, NULL, false);
	if (FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = rs.MoveNext();
	if (FAILED(hr) || hr == DB_S_ENDOFROWSET)
	{
		rs.Close();
		ExecuteSQL(_T("DROP TABLE IF EXISTS t"));
		CloseConnection();
		return false;
	}

	void *val = rs.GetValue(1);
	long *ptr_val = (long *)val;
	ATLASSERT(*ptr_val == 2);

	rs.Close();
	ExecuteSQL(_T("DROP TABLE IF EXISTS t"));

	TestCleanup(); //Close the connection
	return true; //test case passed
}
