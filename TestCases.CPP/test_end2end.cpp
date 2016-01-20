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
#include "test_end2end.h"

//Accessor class for the `t` table
//CREATE TABLE `t`(`id` INT PRIMARY KEY, `str` STRING)
class CTestAccessor
{
public:
	int m_id;
	TCHAR m_str[32];

	DBSTATUS m_dw_id_Status;
	DBSTATUS m_dw_str_Status;

	DBLENGTH m_dw_id_Length;
	DBLENGTH m_dw_str_Length;

	BEGIN_COLUMN_MAP(CTestAccessor)
		COLUMN_ENTRY_LENGTH_STATUS(1, m_id,  m_dw_id_Length,  m_dw_id_Status)
		COLUMN_ENTRY_LENGTH_STATUS(2, m_str, m_dw_str_Length, m_dw_str_Status)
	END_COLUMN_MAP()
};

///
/// End-to-End scenario, with INSERT, UPDATE, DELETE in Rowset
///
bool Test_End2End_Rowset()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	TCHAR *sql_create_table_t = L"CREATE TABLE `t`(`id` INT PRIMARY KEY, `str` STRING)";
	TCHAR *sql_insert_table_t = L"INSERT INTO `t` VALUES(1, 'Existing row')";

	//create the `t` table
	if(!DropTable(L"t"))
		return false;
	if(!ExecuteSQL(sql_create_table_t))
		return false;
	if(!ExecuteSQL(sql_insert_table_t))
		return false;

	session.StartTransaction();
	
	CTable<CAccessor<CTestAccessor>> cmd;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	HRESULT hr = cmd.Open(session, "t", &propset);  // ps is the property set
	if(FAILED(hr))
		return false;

	hr = cmd.MoveNext();
	if(FAILED(hr))
		return false;

	// Change the values of columns "id" and "str" in the current row
	cmd.m_id = 99;
	_tcscpy_s(cmd.m_str, _T("Updated value"));

	hr = cmd.SetData();
	if(FAILED(hr))
		return false;

	ATLASSERT(cmd.m_id == 99);
	ATLASSERT(EQ_STR(cmd.m_str, L"Updated value") == true);

	cmd.m_id = 101;
	_tcscpy_s(cmd.m_str, _T("Inserted value"));
	cmd.m_dw_str_Length = strlen("Inserted value");

	hr = cmd.Insert(0, true); //move from current row to the inserted row
	if(FAILED(hr))
		return false;

	ATLASSERT(cmd.m_id == 101);
	ATLASSERT(EQ_STR(cmd.m_str, L"Inserted value") == true);

	cmd.Close();

	session.Commit();

	// Cleanup
	if(!DropTable(L"t"))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Table and Accessor Declaration for the `code` table in the demodb database
///
class CCodeNoAttrAccessor
{
public:
	DWORD m_dw_sname_Status;
	DWORD m_dw_fname_Status;

	DWORD m_dw_sname_Length;
	DWORD m_dw_fname_Length;

	TCHAR m_sname[2];
	TCHAR m_fname[7];

	void GetRowsetProperties(CDBPropSet* pPropSet)
	{
		pPropSet->AddProperty(DBPROP_CANFETCHBACKWARDS, true);
		pPropSet->AddProperty(DBPROP_CANSCROLLBACKWARDS, true);
		pPropSet->AddProperty(DBPROP_IRowsetChange, true);
	}

	HRESULT OpenDataSource()
	{
		CDataSource _db;
		HRESULT hr;
		hr = _db.OpenFromInitializationString(connectString);
		if (FAILED(hr))
		{
			AtlTraceErrorRecords(hr);
			return hr;
		}

		return m_session.Open(_db);
	}

	void CloseDataSource()
	{
		m_session.Close();
	}

	operator const CSession&()
	{
		return m_session;
	}

	CSession m_session;

	BEGIN_COLUMN_MAP(CCodeNoAttrAccessor)
		COLUMN_ENTRY_LENGTH_STATUS(1, m_sname, m_dw_sname_Length, m_dw_sname_Status)
		COLUMN_ENTRY_LENGTH_STATUS(2, m_fname, m_dw_fname_Length, m_dw_fname_Status)
	END_COLUMN_MAP()
};

class CCodeNoAttr : public CTable<CAccessor<CCodeNoAttrAccessor>>
{
public:
	HRESULT OpenAll()
	{
		HRESULT hr;
		hr = OpenDataSource();
		if (FAILED(hr))
			return hr;

		__if_exists(GetRowsetProperties)
		{
			CDBPropSet propset(DBPROPSET_ROWSET);
			__if_exists(HasBookmark)
			{
				propset.AddProperty(DBPROP_IRowsetLocate, true);
			}
			GetRowsetProperties(&propset);

			return OpenRowset(&propset);
		}

		__if_not_exists(GetRowsetProperties)
		{
			__if_exists(HasBookmark)
			{
				CDBPropSet propset(DBPROPSET_ROWSET);
				propset.AddProperty(DBPROP_IRowsetLocate, true);

				return OpenRowset(&propset);
			}
		}

		return OpenRowset();
	}

	HRESULT OpenRowset(DBPROPSET *pPropSet = NULL)
	{
		HRESULT hr = Open(m_session, "code", pPropSet);
		if(FAILED(hr))
			AtlTraceErrorRecords(hr);

		return hr;
	}

	void CloseAll()
	{
		Close();
		CloseDataSource();
	}
};

///
/// End-to-End scenario, with Table and Accessor
///
bool Test_End2End_AccessorTable()
{
	CCodeNoAttr code;

	HRESULT hr = code.OpenAll();
	if(FAILED(hr))
		return false;

	hr = code.MoveNext();
	if(FAILED(hr))
		return false;

	ATLASSERT(EQ_STR(code.m_sname, L"X") == true);
	ATLASSERT(EQ_STR(code.m_fname, L"Mixed") == true);

	code.CloseAll();

	return true;
}

