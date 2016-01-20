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
#include "test_schema.h"

//http://msdn.microsoft.com/en-us/library/ms711276%28v=vs.85%29.aspx

///
/// Test ColumnPrivileges schema
///
bool Test_Schema_ColumnPrivileges()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CColumnPrivileges ColumnPrivilegesRowset;
	//Restriction columns: TABLE_CATALOG, TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, GRANTOR, GRANTEE
	TCHAR *table_name = L"nation";
	TCHAR *column_name = L"capital";
	HRESULT hr = ColumnPrivilegesRowset.Open(session, NULL, NULL, table_name, column_name);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	int i = 0;
	hr = ColumnPrivilegesRowset.MoveFirst();

	while (hr == S_OK)
	{
		i++;

		if(i == 1)
		{
			ATLASSERT(EQ_STR(ColumnPrivilegesRowset.m_szGrantee, L"public") == true);
			ATLASSERT(EQ_STR(ColumnPrivilegesRowset.m_szGrantor, L"DBA") == true);
			ATLASSERT(EQ_STR(ColumnPrivilegesRowset.m_szPrivilegeType, L"DELETE") == true);
			ATLASSERT(ColumnPrivilegesRowset.m_bIsGrantable == -1);
		}

		hr = ColumnPrivilegesRowset.MoveNext();
	}

	ATLASSERT(i == 4);

	ColumnPrivilegesRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test TablePrivileges schema
///
bool Test_Schema_TablePrivileges()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CTablePrivileges TablePrivilegesRowset;
	//Restriction columns: TABLE_CATALOG, TABLE_SCHEMA, TABLE_NAME, GRANTOR, GRANTEE
	TCHAR *table_name = L"nation";
	HRESULT hr = TablePrivilegesRowset.Open(session, NULL, NULL, table_name);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	int i = 0;
	hr = TablePrivilegesRowset.MoveFirst();
	while (hr == S_OK)
	{
		i++;

		if(i == 1)
		{
			ATLASSERT(EQ_STR(TablePrivilegesRowset.m_szGrantee, L"public") == true);
			ATLASSERT(EQ_STR(TablePrivilegesRowset.m_szGrantor, L"DBA") == true);
			ATLASSERT(EQ_STR(TablePrivilegesRowset.m_szName, L"nation") == true);
			ATLASSERT(EQ_STR(TablePrivilegesRowset.m_szType, L"DELETE") == true);
		}

		hr = TablePrivilegesRowset.MoveNext();
	}

	ATLASSERT(i == 4);

	TablePrivilegesRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test TableConstraints schema
///
bool Test_Schema_TableConstraints()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	//Restriction columns: CONSTRAINT_CATALOG, CONSTRAINT_SCHEMA, CONSTRAINT_NAME, TABLE_CATALOG, TABLE_SCHEMA, TABLE_NAME, CONSTRAINT_TYPE
	CTableConstraints TableConstraintsRowset;
	HRESULT hr = TableConstraintsRowset.Open(session, NULL, NULL, NULL, NULL, NULL, NULL);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	int i = 0;
	hr = TableConstraintsRowset.MoveFirst();

	while (hr == S_OK)
	{
		i++;

		if(i == 1)
		{
			ATLASSERT(EQ_STR(TableConstraintsRowset.m_szName, L"u_db_ha_apply_info_db_name_copied_log_path") == true);
			ATLASSERT(EQ_STR(TableConstraintsRowset.m_szDescription, L"") == true);
			ATLASSERT(TableConstraintsRowset.m_bIsDeferrable == 0);
		}

		hr = TableConstraintsRowset.MoveNext();
	}

	ATLASSERT(i == 2);

	TableConstraintsRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Statistics schema
///
bool Test_Schema_Statistics()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	//Restriction columns: TABLE_CATALOG, TABLE_SCHEMA, TABLE_NAME
	CStatistics StatisticsRowset;
	TCHAR *table_name = L"nation";
	HRESULT hr = StatisticsRowset.Open(session, NULL, NULL, table_name);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	int i = 0;
	hr = StatisticsRowset.MoveFirst();

	while (hr == S_OK)
	{
		i++;

		if(i == 1)
		{
			ATLASSERT(EQ_STR(StatisticsRowset.m_szTableSchema, L"") == true);
			ATLASSERT(EQ_STR(StatisticsRowset.m_szTableCatalog, L"") == true);
			ATLASSERT(EQ_STR(StatisticsRowset.m_szTableName, L"nation") == true);
			ATLASSERT(StatisticsRowset.m_nCardinality == 215);
		}

		hr = StatisticsRowset.MoveNext();
	}

	ATLASSERT(i == 1);

	StatisticsRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test ViewColumnUsage schema
///
bool Test_Schema_ViewColumnUsage()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	//Restriction columns: VIEW_CATALOG, VIEW_SCHEMA, VIEW_NAME
	CViewColumnUsage ViewColumnUsageRowset;
	HRESULT hr = ViewColumnUsageRowset.Open(session, NULL, NULL, NULL);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	int i = 0;
	hr = ViewColumnUsageRowset.MoveFirst();

	while (hr == S_OK)
	{
		i++;

		if(i == 1)
		{
			ATLASSERT(EQ_STR(ViewColumnUsageRowset.m_szColumnName, L"attr_name") == true);
			ATLASSERT(EQ_STR(ViewColumnUsageRowset.m_szName, L"db_attr_setdomain_elm") == true);
			ATLASSERT(EQ_STR(ViewColumnUsageRowset.m_szTableName, L"db_attr_setdomain_elm") == true);
		}

		hr = ViewColumnUsageRowset.MoveNext();
	}

	ATLASSERT(i == 100);

	ViewColumnUsageRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test ProviderTypes schema
///
bool Test_Schema_ProviderTypes()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CProviderTypes ProviderTypesRowset;
	HRESULT hr = ProviderTypesRowset.Open(session, NULL, NULL, NULL);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	int i = 0;
	hr = ProviderTypesRowset.MoveFirst();
	while (hr == S_OK)
	{
		i++;

		if(i == 1)
		{
			ATLASSERT(EQ_STR(ProviderTypesRowset.m_szTypeName, L"SMALLINT") == true);
			ATLASSERT(ProviderTypesRowset.m_nDataType == 2);
			ATLASSERT(ProviderTypesRowset.m_bCaseSensitive == 0);
		}

		if(i == 10)
		{
			ATLASSERT(EQ_STR(ProviderTypesRowset.m_szTypeName, L"BIT") == true);
			ATLASSERT(ProviderTypesRowset.m_nDataType == 128);
			ATLASSERT(ProviderTypesRowset.m_bCaseSensitive == 0);
		}

		hr = ProviderTypesRowset.MoveNext();
	}

	ATLASSERT(i == 27);

	ProviderTypesRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Columns schema
///
bool Test_Schema_Columns()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CColumns ColumnSchemaRowset;
	// TABLE_NAME is the third restriction column
	TCHAR *table_name = L"nation";
	HRESULT hr = ColumnSchemaRowset.Open(session, NULL, NULL, (LPCTSTR)table_name);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	int i = 0;
	hr = ColumnSchemaRowset.MoveFirst();
	while (hr == S_OK)
	{
		i++;
		hr = ColumnSchemaRowset.MoveNext();

		if(i == 1)
		{
			ATLASSERT(EQ_STR(ColumnSchemaRowset.m_szColumnName, L"name") == true);
			ATLASSERT(ColumnSchemaRowset.m_nDataType == 129);
			ATLASSERT(ColumnSchemaRowset.m_nOrdinalPosition == 2);
		}
	}

	ATLASSERT(i == 4); //there are 4 columns in the `nation` table

	ColumnSchemaRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Tables schema
///
bool Test_Schema_Tables()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CTables TablesSchemaRowset;
	//Restriction columns: TABLE_CATALOG, TABLE_SCHEMA, TABLE_NAME, TABLE_TYPE
	TCHAR *table_name = L"nation";
	HRESULT hr = TablesSchemaRowset.Open(session, NULL, NULL, (LPCTSTR)table_name);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	hr = TablesSchemaRowset.MoveFirst();
	if (hr == S_OK)
	{
		ATLASSERT(EQ_STR(TablesSchemaRowset.m_szCatalog , L"") == true);
		ATLASSERT(EQ_STR(TablesSchemaRowset.m_szDescription , L"Class") == true);
		ATLASSERT(EQ_STR(TablesSchemaRowset.m_szName , L"nation") == true);
		ATLASSERT(EQ_STR(TablesSchemaRowset.m_szType , L"TABLE") == true);
	}
	else
	{
		CloseConnection();
		return false;
	}

	TablesSchemaRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Tables schema
///
bool Test_Schema_Tables_All()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CTables TablesSchemaRowset;
	//Restriction columns: TABLE_CATALOG, TABLE_SCHEMA, TABLE_NAME, TABLE_TYPE
	HRESULT hr = TablesSchemaRowset.Open(session, NULL, NULL, NULL);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	hr = TablesSchemaRowset.MoveFirst();
	if (hr == S_OK)
	{
		ATLASSERT(EQ_STR(TablesSchemaRowset.m_szCatalog , L"") == true);
		ATLASSERT(EQ_STR(TablesSchemaRowset.m_szDescription , L"System Class") == true);
		ATLASSERT(EQ_STR(TablesSchemaRowset.m_szName , L"db_attr_setdomain_elm") == true);
		ATLASSERT(EQ_STR(TablesSchemaRowset.m_szType , L"SYSTEM TABLE") == true);
	}
	else
	{
		CloseConnection();
		return false;
	}

	TablesSchemaRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Primary Keys schema
///
bool Test_Schema_PrimaryKeys()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CPrimaryKeys PrimaryKeysSchemaRowset;
	//Restriction columns: TABLE_CATALOG, TABLE_SCHEMA, TABLE_NAME
	TCHAR *table_name = L"nation";
	HRESULT hr = PrimaryKeysSchemaRowset.Open(session, NULL, NULL, (LPCTSTR)table_name);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	hr = PrimaryKeysSchemaRowset.MoveFirst();
	if (hr == S_OK)
	{
		ATLASSERT(EQ_STR(PrimaryKeysSchemaRowset.m_szColumnName , L"code")==true);
		ATLASSERT(PrimaryKeysSchemaRowset.m_nOrdinal == 1);
		ATLASSERT(EQ_STR(PrimaryKeysSchemaRowset.m_szTableName , L"nation")==true);
	}
	else
	{
		CloseConnection();
		return false;
	}

	PrimaryKeysSchemaRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Views schema
///
bool Test_Schema_Views()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CViews ViewsSchemaRowset;
	//Restriction columns: TABLE_CATALOG, TABLE_SCHEMA, TABLE_NAME
	TCHAR *table_name = L"db_class";
	HRESULT hr = ViewsSchemaRowset.Open(session, NULL, NULL, (LPCTSTR)table_name);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	hr = ViewsSchemaRowset.MoveFirst();
	if (hr == S_OK)
	{
		//TODO Add verification of the columns properties
		ATLASSERT(EQ_STR(ViewsSchemaRowset.m_szDescription , L"")==true);
		ATLASSERT(EQ_STR(ViewsSchemaRowset.m_szTableSchema , L"")==true);
		ATLASSERT(EQ_STR(ViewsSchemaRowset.m_szTableCatalog , L"")==true);
		ATLASSERT(EQ_STR(ViewsSchemaRowset.m_szTableName , L"db_class")==true);
		ATLASSERT(ViewsSchemaRowset.m_bIsUpdatable == 0);
		ATLASSERT(EQ_STR(ViewsSchemaRowset.m_szDefinition, L"SELECT [c].[class_name], CAST([c].[owner].[name] AS VARCHAR(255)), CASE [c].[class_type] WHEN 0 THEN 'CLASS' WHEN 1 THEN 'VCLASS") == true);
	}
	else
	{
		CloseConnection();
		return false;
	}

	ViewsSchemaRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Foreign Keys schema
///
bool Test_Schema_ForeignKeys()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CForeignKeys ForeignKeysSchemaRowset;
	//Restriction columns: PK_TABLE_CATALOG, PK_TABLE_SCHEMA, PK_TABLE_NAME, FK_TABLE_CATALOG, FK_TABLE_SCHEMA, FK_TABLE_NAME
	TCHAR *table_name = L"game";
	HRESULT hr = ForeignKeysSchemaRowset.Open(session, NULL, NULL, NULL, NULL, NULL, (LPCTSTR)table_name);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	hr = ForeignKeysSchemaRowset.MoveFirst();
	if (hr == S_OK)
	{
		ATLASSERT(EQ_STR(ForeignKeysSchemaRowset.m_szFKColumnName, L"athlete_code")==true);
		ATLASSERT(EQ_STR(ForeignKeysSchemaRowset.m_szFKTableName , L"game")==true);
		ATLASSERT(EQ_STR(ForeignKeysSchemaRowset.m_szPKColumnName, L"code")==true);
		ATLASSERT(EQ_STR(ForeignKeysSchemaRowset.m_szPKTableName , L"athlete")==true);
		ATLASSERT(EQ_STR(ForeignKeysSchemaRowset.m_szDeleteRule , L"restrict")==true);
		ATLASSERT(EQ_STR(ForeignKeysSchemaRowset.m_szUpdateRule , L"restrict")==true);
	}
	else
	{
		ForeignKeysSchemaRowset.Close();
		CloseConnection();
		return false;
	}

	ForeignKeysSchemaRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Indexes schema
///
bool Test_Schema_Indexes()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CIndexes IndexesSchemaRowset;
	//Restriction columns: TABLE_CATALOG, TABLE_SCHEMA, INDEX_NAME, TYPE, TABLE_NAME
	HRESULT hr = IndexesSchemaRowset.Open(session, NULL, NULL, NULL, NULL, NULL);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	hr = IndexesSchemaRowset.MoveFirst();
	if (hr == S_OK)
	{
		ATLASSERT(EQ_STR(IndexesSchemaRowset.m_szColumnName , L"name")==true);
		ATLASSERT(EQ_STR(IndexesSchemaRowset.m_szTableName , L"db_user")==true);
		ATLASSERT(EQ_STR(IndexesSchemaRowset.m_szIndexName , L"i_db_user_name")==true);
	}
	else
	{
		CloseConnection();
		return false;
	}

	IndexesSchemaRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Constraints schema
///
bool Test_Schema_Contraints()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CTableConstraints TableContraintsSchemaRowset;
	//Restriction columns: CONSTRAINT_CATALOG, CONSTRAINT_SCHEMA, CONSTRAINT_NAME, TABLE_CATALOG, TABLE_SCHEMA, TABLE_NAME, CONSTRAINT_TYPE
	TCHAR *table_name = L"game";
	HRESULT hr = TableContraintsSchemaRowset.Open(session, NULL, NULL, NULL, NULL, NULL, table_name);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	hr = TableContraintsSchemaRowset.MoveFirst();
	if (hr == S_OK)
	{
		ATLASSERT(EQ_STR(TableContraintsSchemaRowset.m_szName , L"code")==true);
		ATLASSERT(EQ_STR(TableContraintsSchemaRowset.m_szTableName , L"athlete")==true);
	}
	else
	{
		CloseConnection();
		return false;
	}

	TableContraintsSchemaRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Procedures schema
///
bool Test_Schema_Procedures()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CProcedures ProceduresSchemaRowset;
	//Restriction columns: PROCEDURE_CATALOG, PROCEDURE_SCHEMA, PROCEDURE_NAME, PROCEDURE_TYPE
	HRESULT hr = ProceduresSchemaRowset.Open(session, NULL, NULL, NULL, NULL);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	hr = ProceduresSchemaRowset.MoveFirst();
	if (hr == S_OK)
	{
		//TODO Add verification of the columns properties
		ATLASSERT(EQ_STR(ProceduresSchemaRowset.m_szName , L"")==true);
		ATLASSERT(EQ_STR(ProceduresSchemaRowset.m_szDescription , L"")==true);
	}
	else
	{
		CloseConnection();
		return false;
	}

	ProceduresSchemaRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Catalogs schema
///
bool Test_Schema_Catalogs()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CCatalogs CatalogsSchemaRowset;
	//Restriction columns: CATALOG_NAME
	HRESULT hr = CatalogsSchemaRowset.Open(session, NULL);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}	

	hr = CatalogsSchemaRowset.MoveFirst();
	if (hr == S_OK)
	{
		ATLASSERT(EQ_STR(CatalogsSchemaRowset.m_szName , L"demodb") == true);
		ATLASSERT(EQ_STR(CatalogsSchemaRowset.m_szDescription , L"demodb") == true);
	}
	else
	{
		CloseConnection();
		return false;
	}

	CatalogsSchemaRowset.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

