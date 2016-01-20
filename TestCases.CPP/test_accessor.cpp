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
#include "test_accessor.h"

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

	DEFINE_COMMAND_EX(CNationAccessor, _T("SELECT * FROM `nation`"))

	// You may wish to call this function if you are inserting a record and wish to
	// initialize all the fields, if you are not going to explicitly set all of them.
	void ClearRecord()
	{
		memset(this, 0, sizeof(*this));
	}
};

///
/// Test SELECT with an Accessor-type Command
///
bool Test_Accessor_Simple_SELECT()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CCommand <CAccessor<CNationAccessor>> nation;
	char *sql = "SELECT * FROM nation";

	HRESULT hr = nation.Open(session, sql);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the <nation> table!");
		CloseConnection();
		return false;
	}

	if(nation.MoveNext() == S_OK)
	{
		ATLASSERT(EQ_STR(nation.m_Code, L"SRB") == true);
		ATLASSERT(EQ_STR(nation.m_Name, L"Serbia") == true);
		ATLASSERT(EQ_STR(nation.m_Continent, L"Europe") == true);
		ATLASSERT(EQ_STR(nation.m_Capital, L"Beograd") == true);
	}
	else
	{
		CloseConnection();
		return false;
	}

	nation.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test CDBPropSet and default SELECT with Accessor-type Command
///
bool Test_Accessor_Command_CDBPropSet()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CDBPropSet propset(DBPROPSET_ROWSET); 
	propset.AddProperty(DBPROP_IRowsetChange, true);		
	propset.AddProperty(DBPROP_UPDATABILITY, 
		DBPROPVAL_UP_CHANGE | 
		DBPROPVAL_UP_INSERT | 
		DBPROPVAL_UP_DELETE);

	CCommand <CAccessor<CNationAccessor>> cmd;

	HRESULT hr = cmd.Open(session, NULL, &propset);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the `nation` table!");
		CloseConnection();
		return false;
	}

	cmd.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

//Accessor class for the `t` table
//CREATE TABLE `t`(`id` INT PRIMARY KEY, `str` STRING)
class CTAccessor
{
public:
	// data elements
	int m_id;
	TCHAR m_str[32];

	BEGIN_ACCESSOR_MAP(CTAccessor, 4)
		BEGIN_ACCESSOR(0, true)
			COLUMN_ENTRY_TYPE(1, DBTYPE_I4, m_id)
			COLUMN_ENTRY_TYPE(2, DBTYPE_STR, m_str)
		END_ACCESSOR()
		BEGIN_ACCESSOR(1, false)  // Not an auto accessor
			COLUMN_ENTRY_TYPE(2, DBTYPE_STR, m_str)
		END_ACCESSOR()
		BEGIN_ACCESSOR(2, false)  // Not an auto accessor
			COLUMN_ENTRY_TYPE(1, DBTYPE_I4, m_id)
		END_ACCESSOR()
		BEGIN_ACCESSOR(3, false)  // Not an auto accessor
			COLUMN_ENTRY_TYPE(1, DBTYPE_I4, m_id)
			COLUMN_ENTRY_TYPE(2, DBTYPE_STR, m_str)
		END_ACCESSOR()
	END_ACCESSOR_MAP()
};


///
/// Test INSERT, UPDATE, DELETE with an Accessor-type Command
/// Table used: `t`
///
bool Test_Accessor_UPDATE()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	TCHAR *sql_select = L"SELECT * FROM t";
	TCHAR *sql_create_table_t = L"CREATE TABLE `t`(`id` INT PRIMARY KEY, `str` STRING)";
	TCHAR *sql_insert_table_t = L"INSERT INTO `t` VALUES(1, 'One')";
	TCHAR *sql_drop_table_t = L"DROP TABLE IF EXISTS `t`";

	//create the `t` table
	if(!ExecuteSQL(sql_drop_table_t))
		return false;
	if(!ExecuteSQL(sql_create_table_t))
		return false;

	//insert some test data into the `t` table
	if(!ExecuteSQL(sql_insert_table_t))
		return false;

	session.StartTransaction();

	CCommand <CAccessor<CTAccessor>> t;

	CDBPropSet propset(DBPROPSET_ROWSET);
  propset.AddProperty(DBPROP_CANFETCHBACKWARDS,   true);
  propset.AddProperty(DBPROP_IRowsetScroll, true);
  propset.AddProperty(DBPROP_IRowsetChange, true);
  propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE );

	//select data from the `t` table
	HRESULT hr = t.Open(session, sql_select, &propset);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the `t` table!");
		ExecuteSQL(sql_drop_table_t);
		CloseConnection();
		return false;
	}

	if(t.MoveNext() == S_OK)
	{
		ATLASSERT(t.m_id == 1);
		ATLASSERT(EQ_STR(t.m_str, L"One") == 0);
	}
	else
	{
		OutputDebugString(L"Couldn't open the `t` table!");
		ExecuteSQL(sql_drop_table_t);
		CloseConnection();
		return false;
	}

	//update data in the `t` table
	t.m_id = 1000;
	//strncpy_s(t.m_str, "Two", strlen("Two"));
	wcscpy(t.m_str, L"Two");

	hr = t.SetData(3);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't update the `t` table!");
		ExecuteSQL(sql_drop_table_t);
		CloseConnection();
		return false;
	}

	t.Close();
	t.ReleaseCommand();

	session.Commit();

	//select data from the `t` table
	hr = t.Open(session, sql_select);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the `t` table!");
		ExecuteSQL(sql_drop_table_t);
		CloseConnection();
		return false;
	}

	if(t.MoveNext() == S_OK)
	{
		ATLASSERT(t.m_id == 1000);
		ATLASSERT(EQ_STR(t.m_str, L"Two") == 0);
	}
	else
	{
		OutputDebugString(L"Couldn't open the `t` table!");
		ExecuteSQL(sql_drop_table_t);
		CloseConnection();
		return false;
	}

	t.Close();

	//drop the `t` table
	ExecuteSQL(sql_drop_table_t);

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Define accessor for the `code` table
///
class CCodeAccessor
{
public:
	char m_s_name[1];
	char m_f_name[6];

	DBSTATUS m_s_nameStatus;
	DBSTATUS m_f_nameStatus;

	BEGIN_COLUMN_MAP(CCodeAccessor)
		COLUMN_ENTRY_STATUS(1, m_s_name, m_s_nameStatus)
		COLUMN_ENTRY_STATUS(2, m_f_name, m_f_nameStatus)
	END_COLUMN_MAP()

	DEFINE_COMMAND_EX(CCodeAccessor, _T("SELECT s_name, f_name FROM `code` order by s_name ASC"))

		// You may wish to call this function if you are inserting a record and wish to
		// initialize all the fields, if you are not going to explicitly set all of them.
		void ClearRecord()
	{
		memset(this, 0, sizeof(*this));
	}
};

///
/// Define clase based on the 'code' accessor (CCodeAccessor)
///
class CCodeAccessorTable : public CCommand<CAccessor<CCodeAccessor> >
{
public:
	HRESULT Open()
	{
		HRESULT		hr;
		hr = OpenDataSource();
		if (FAILED(hr))
			return hr;

		return OpenRowset();
	}

	CSession	m_session;

private:
	HRESULT OpenDataSource()
	{
		CDataSource db;

		HRESULT		hr;
		hr = db.OpenFromInitializationString(connectString);
		if (FAILED(hr))
			return hr;

		return m_session.Open(db);
	}

	HRESULT OpenRowset()
	{
		// Set properties for Open
		CDBPropSet	propset(DBPROPSET_ROWSET);
		propset.AddProperty(DBPROP_IRowsetChange, true);
		propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

		return CCommand<CAccessor<CCodeAccessor> >::Open(m_session, NULL, &propset);
	}
};

///
/// Test UPDATE into the `code` table, by calling CRowset::SetData() on the CCommand object 
///
bool Test_Accessor_UPDATE_SetData()
{
	CCodeAccessorTable rs;

	HRESULT hr = rs.Open();
	if (FAILED(hr))
		return false;

	hr = rs.MoveLast();
	if (FAILED(hr))
		return false;

	//TODO
	ATLASSERT(rs.m_s_name == "X");
	ATLASSERT(rs.m_f_name == "Mixed");
	ATLASSERT(rs.m_s_nameStatus == DBSTATUS_S_OK);
	ATLASSERT(rs.m_s_nameStatus == DBSTATUS_S_OK);

	// Insert a new record
	rs.ClearRecord();
	strcpy_s(rs.m_s_name, 1, "Y");
	rs.m_f_nameStatus = DBSTATUS_S_IGNORE;
	hr=rs.Insert();
	if (FAILED(hr))
		return false;

	//TODO
	ATLASSERT(rs.m_s_name == "Z");
	ATLASSERT(rs.m_f_name == NULL);
	ATLASSERT(rs.m_s_nameStatus == DBSTATUS_S_OK);
	ATLASSERT(rs.m_s_nameStatus == DBSTATUS_S_ISNULL);

	// Update the test field
	strcpy_s(rs.m_s_name, 1, "Z");
	rs.m_f_nameStatus = DBSTATUS_S_IGNORE;
	hr=rs.SetData();
	if (FAILED(hr))
		return false;

	ATLASSERT(rs.m_s_name == "Z");
	ATLASSERT(rs.m_f_name == NULL);
	ATLASSERT(rs.m_s_nameStatus == DBSTATUS_S_OK);
	ATLASSERT(rs.m_s_nameStatus == DBSTATUS_S_ISNULL);

	//Delete the inserted record
	CCommand<CNoAccessor> cmd;
	hr=cmd.Open(rs.m_session, "DELETE FROM `code` WHERE s_name='Z'");
	if (FAILED(hr))
		return false;
	cmd.Close();

	rs.Close();
	rs.m_session.Close();

	return true;
};


