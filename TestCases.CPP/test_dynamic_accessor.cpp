#include "stdafx.h"

#include <atldbcli.h>
#include <atlconv.h>
#include <oledb.h>
#include <oledberr.h>
#include <atldbcli.h>
#include <atlcoll.h>
#include <atldbsch.h> //schema support

#include "utils\dynamicaccessorex.h"
#include "utils\dynamiccommand.h"

#include "globals.h"
#include "helpers.h"
#include "test_dynamic_accessor.h"

bool Test_DynamicAccessor_Rowset_Select_FixedValues()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	//TCHAR *sql = L"select 'Beograd', 3 from db_root";
	TCHAR *sql = L"select cast('Beograd' as varchar(32)), 3 from db_root";

	CCommand<CDynamicAccessor, CRowset> cmd;

	HRESULT hr = cmd.Open(session, sql);
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

	ATLASSERT(cmd.GetColumnCount() == 2);
	ATLASSERT(EQ_STR(cmd.GetColumnName(1), L"cast('Beograd' as varchar(32))") == true);
	ATLASSERT(EQ_STR(cmd.GetColumnName(2), L"3") == true);

	void *val = cmd.GetValue(1);
	CHAR *ptr_val = (CHAR *)val;
	ATLASSERT(!strcmp(ptr_val,  "Beograd") == true);

	val = cmd.GetValue(2);
	int *ival = (int *)val;
	int num = *ival;

	ATLASSERT(num == 3);

	cmd.Close();   

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_DynamicAccessorEx_Select()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	TCHAR *sql = L"select * from nation";
	oledb::CDynamicCommand<oledb::CDynamicAccessorEx> cmd;
	cmd.SetCommandTemplate(sql);

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_CANSCROLLBACKWARDS, true);
	//propset.AddProperty( DBPROP_ISequentialStream, true, DBPROPOPTIONS_OPTIONAL );
	//propset.AddProperty( DBPROP_IStream, true, DBPROPOPTIONS_OPTIONAL );
	propset.AddProperty(DBPROP_IRowsetScroll,true,DBPROPOPTIONS_OPTIONAL);
	//propset.AddProperty(DBPROP_IRowsetUpdate,true,DBPROPOPTIONS_OPTIONAL);
	//propset.AddProperty(DBPROP_IRowsetChange, true, DBPROPOPTIONS_OPTIONAL);
	//propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE);
	//propset.AddProperty(DBPROP_CLIENTCURSOR,true,DBPROPOPTIONS_REQUIRED);

	HRESULT hr = cmd.Open(session, &propset);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = cmd.MoveFirst(); 
	if(FAILED(hr)) 
	{
		cmd.Close();
		CloseConnection();
		return false;
	}

	CString str;
	cmd.GetValue(1, &str);
	ATLASSERT(EQ_STR(str,  L"SRB") == true);

	hr = cmd.MoveNext();
	if(FAILED(hr)) 
	{
		cmd.Close();
		CloseConnection();
		return false;
	}
	cmd.GetValue(1, &str);
	ATLASSERT(EQ_STR(str,  L"KIR") == true);

	hr = cmd.MoveLast(); 
	if(FAILED(hr)) 
	{
		cmd.Close();
		CloseConnection();
		return false;
	}
	cmd.GetValue(1, &str);
	ATLASSERT(EQ_STR(str,  L"AFG") == true);

	hr = cmd.MovePrev(); 
	if(FAILED(hr)) 
	{
		cmd.Close();
		CloseConnection();
		return false;
	}
	cmd.GetValue(1, &str);
	ATLASSERT(EQ_STR(str,  L"AHO") == true);

	cmd.Close();   

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_DynamicAccessor_Select_Table()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	TCHAR *sql = L"select capital, ROWNUM from nation where ROWNUM = 3";
	CCommand<CDynamicAccessor, CRowset> cmd;

	HRESULT hr = cmd.Open(session, sql);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = cmd.MoveNext();
	if(FAILED(hr))
	{
		cmd.Close();
		CloseConnection();
		return false;
	}

	void *val = cmd.GetValue(1);
	ATLASSERT(strcmp((char*)val,  "Beograd") == 0);

	val = cmd.GetValue(2);
	int *ival = (int *)val;
	int num = *ival;

	ATLASSERT(num == 3);

	cmd.Close();   

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_DynamicAccessor_Select_FixedValues()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	TCHAR *sql = L"select 'Beograd', 3 from db_root";
	CCommand<CDynamicAccessor, CRowset> cmd;

	HRESULT hr = cmd.Open(session, sql);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	ATLASSERT(cmd.GetColumnCount() == 2);
	ATLASSERT(EQ_STR(cmd.GetColumnName(1), L"'Beograd'") == true);
	ATLASSERT(EQ_STR(cmd.GetColumnName(2), L"3") == true);
	ATLASSERT(cmd.HasOutputColumns() == true);
	ATLASSERT(cmd.m_hRow == 0);

	cmd.Close();   

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_DynamicAccessor_SelectCOUNT()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	TCHAR *sql = L"select count(*) from nation";
	CCommand<CDynamicAccessor, CRowset> cmd;

	HRESULT hr = cmd.Open(session, sql);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = cmd.MoveNext();
	if(FAILED(hr))
	{
		cmd.Close();
		CloseConnection();
		return false;
	}

	void *val = cmd.GetValue(1);
	int *ival = (int *)val;
	int count = *ival;

	ATLASSERT(count == 215);

	cmd.Close();   

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_DynamicAccessor()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CTable<CDynamicAccessor> rs;

	HRESULT hr = rs.Open(session, "nation");
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = rs.MoveNext();
	if(FAILED(hr))
		return false;

	int row_pos = 0;
	while(SUCCEEDED(hr) && hr != DB_S_ENDOFROWSET)
	{
		row_pos++;
		if(row_pos > 2)
			break;

		for(size_t i = 1; i <= rs.GetColumnCount(); i++)
		{
			DBTYPE type;
			rs.GetColumnType(i, &type);
			switch(i)
			{
			case 1:
				ATLASSERT(EQ_STR(rs.GetColumnName(i), L"code") == true);
				ATLASSERT(type == DBTYPE_STR);
				break;
			case 2:
				ATLASSERT(EQ_STR(rs.GetColumnName(i), L"name") == true);
				ATLASSERT(type == DBTYPE_STR);
				break;
			case 3:
				ATLASSERT(EQ_STR(rs.GetColumnName(i), L"continent") == true);
				ATLASSERT(type == DBTYPE_STR);
				break;
			case 4:
				ATLASSERT(EQ_STR(rs.GetColumnName(i), L"capital") == true);
				ATLASSERT(type == DBTYPE_STR);
				break;
			default:
				break;
			}

			CString val;
			void *value;
			char *ptr_val;

			switch(type)
			{
			case DBTYPE_STR:
				value = rs.GetValue(i);
				ptr_val = (char *)value;
				val.Format(_T("%s"), CA2W(ptr_val));
				break;
			default:
				val.Format(_T("%l"), (long*)rs.GetValue(i));
			}

			switch(row_pos)
			{
			case 1:
				if(i==1)
				{
					ATLASSERT(val == "SRB");
				}
				if(i==2)
				{
					ATLASSERT(val == "Serbia");
				}
				break;
			case 2:
				if(i==1)
				{
					ATLASSERT(val == "KIR");
				}
				if(i==3)
				{
					ATLASSERT(val == "Oceania");
				}
				break;
			}
		}

		hr = rs.MoveNext();
		if(FAILED(hr))
		{
			CloseConnection();
			return false;
		}
	}

	rs.Close();   

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test dynamic string accessor - CDynamicStringAccessor
///
bool Test_DynamicStringAccessor()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CTable<CDynamicStringAccessor> rs;

	HRESULT hr = rs.Open(session, "nation");
	if(FAILED(hr))
		return false;

	hr = rs.MoveNext();
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	if(SUCCEEDED(hr) && hr != DB_S_ENDOFROWSET)
	{
		ATLASSERT(EQ_STR(rs.GetString(1), L"SRB") == true);
		ATLASSERT(EQ_STR(rs.GetString(2), L"Serbia") == true);
		ATLASSERT(EQ_STR(rs.GetString(3), L"Europe") == true);
		ATLASSERT(EQ_STR(rs.GetString(4), L"Beograd") == true);
	}

	rs.Close();   

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test dynamic accessor override
/// http://msdn.microsoft.com/en-us/library/63e67b95%28v=vs.71%29.aspx
///
bool Test_Override_DynamicAccessor()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CCommand<CDynamicAccessor> nation;
	// Open the table, passing false to prevent automatic binding 
	nation.Open(session, _T("SELECT `code`, `name` FROM `nation`"), NULL, NULL, DBGUID_DEFAULT, false);

	DBORDINAL nColumns;
	DBCOLUMNINFO* pColumnInfo;
	OLECHAR* ppStrings;

	// Get the column information from the opened rowset.
	// TODO Document that 3rd parameter can't be NULL, despite the official MSDN documentation :(
	HRESULT hr = nation.GetColumnInfo(&nColumns, &pColumnInfo, &ppStrings);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	ATLASSERT(nColumns == 2);

	// Add Bind entries as they are
	nation.AddBindEntry(pColumnInfo[0]);
	nation.AddBindEntry(pColumnInfo[1]);

	nation.Bind();

	// Free the memory for the column information that was retrieved in previous call to GetColumnInfo.
	//CoTaskMemFree(pColumnInfo);

	char* pszNationCode;
	char* pszNationName;
	if (nation.MoveNext() == S_OK)
	{
		 pszNationCode = (char*)nation.GetValue(1);
		 pszNationName = (char*)nation.GetValue(2);

		 ATLASSERT(strcmp(pszNationCode, "SRB") == 0);
		 ATLASSERT(strcmp(pszNationName, "Serbia") == 0);
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
/// Test columns properties
///
bool Test_DynamicStringAccessor_ColumnsStatus()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CTable<CDynamicStringAccessor> rs;

	HRESULT hr = rs.Open(session, "nation");
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = rs.MoveNext();
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	if(SUCCEEDED(hr) && hr != DB_S_ENDOFROWSET)
	{
		DBSTATUS status;

		bool ret = rs.GetStatus(1, &status);
		if(ret)
			ATLASSERT(status == DBSTATUS_S_OK);
		else
		{
			rs.Close();
			return false;
		}
		
		ret = rs.GetStatus("name", &status);
		if(ret)
			ATLASSERT(status == DBSTATUS_S_OK);
		else
		{
			rs.Close();
			return false;
		}
		
		DBLENGTH length;
		ret = rs.GetLength(1, &length);
		if(ret)
			ATLASSERT(length == 6); 
		else
		{
			rs.Close();
			return false;
		}
	}
		
	rs.Close();   

	TestCleanup(); //Close the connection
	return true; //test case passed
}

