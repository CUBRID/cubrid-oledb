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
#include "test_IUD.h"

#define _CRT_SECURE_NO_WARNINGS

//Accessor class for Insert/Update/Delete tests using CTable
class CNation
{
public:
	TCHAR m_Code[4];
	TCHAR m_Name[41];
	TCHAR m_Continent[11];
	TCHAR m_Capital[31];

	//Output binding map.
	BEGIN_COLUMN_MAP(CNation)
		COLUMN_ENTRY(1, m_Code)
		COLUMN_ENTRY(2, m_Name)
		COLUMN_ENTRY(3, m_Continent)
		COLUMN_ENTRY(4, m_Capital)
	END_COLUMN_MAP()
};

class CNationInsert
{
public:
	//Parameter variable
	TCHAR m_Code[4];
	TCHAR m_Name[41];
	TCHAR m_Continent[11];
	TCHAR m_Capital[31];

	//Parameter binding map
	BEGIN_PARAM_MAP(CNationInsert)
		SET_PARAM_TYPE(DBPARAMIO_INPUT)
		COLUMN_ENTRY(1, m_Code)
		COLUMN_ENTRY(2, m_Name)
		COLUMN_ENTRY(3, m_Continent)
		COLUMN_ENTRY(4, m_Capital)
	END_PARAM_MAP()
};

class CNationDelete
{
public:
	//parameter variable
	TCHAR m_Code[4];

	// Parameter binding map
	BEGIN_PARAM_MAP(CNationDelete)
		SET_PARAM_TYPE(DBPARAMIO_INPUT)
		COLUMN_ENTRY(1, m_Code)
	END_PARAM_MAP()
};

class CNationUpdate
{
public:
	//Parameter variable
	TCHAR m_Code[4];
	TCHAR m_Name[41];
	TCHAR m_Continent[11];
	TCHAR m_Capital[31];

	//Parameter binding map
	BEGIN_PARAM_MAP(CNationUpdate)
		SET_PARAM_TYPE(DBPARAMIO_INPUT)
		COLUMN_ENTRY(1, m_Code)
		COLUMN_ENTRY(2, m_Name)
		COLUMN_ENTRY(3, m_Continent)
		COLUMN_ENTRY(4, m_Capital)
	END_PARAM_MAP()
};

///http://www.codeproject.com/Articles/1559/OLE-DB-consumer-using-basic-C
bool Test_InsertFromAccessorUsingCTable()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CTable <CAccessor<CNation>> nation_cmd;
	HRESULT hr;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	hr = nation_cmd.Open(session, L"nation", &propset);
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not open nation table!");
		CloseConnection();
		return false;
	}

	nation_cmd.MoveNext();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not move to first record!");
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	//Insert
	wcscpy(nation_cmd.m_Code, L"ZZZ");
	wcscpy(nation_cmd.m_Name, L"ABCDEF");
	wcscpy(nation_cmd.m_Continent, L"MyXYZ");
	wcscpy(nation_cmd.m_Capital, L"QWERTY");

	hr = nation_cmd.Insert();

	if(FAILED(hr))
	{
		OutputDebugString(L"Could not insert row!");
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	nation_cmd.Close();

	if(!ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_DeleteFromAccessorUsingCTable()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("CREATE TABLE nation2(`code` CHAR(3), `name` VARCHAR(40), `continent` VARCHAR(10), `capital` VARCHAR(30))")))
		return false;
	if(!ExecuteSQL(_T("INSERT INTO nation2 VALUES('SRB', 'Serbia', 'Europe', 'Beograd')")))
		return false;

	CTable <CAccessor<CNation>> nation_cmd;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	HRESULT hr = nation_cmd.Open(session, L"nation2", &propset);
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not open nation2 table!");
		CloseConnection();
		return false;
	}

	hr = nation_cmd.MoveNext();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not move to first record!");
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	//Delete
	hr = nation_cmd.Delete();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not delete row!");
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	nation_cmd.Close();

	if(!ExecuteSQL(_T("DROP TABLE IF EXISTS nation2")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_UpdateFromAccessorUsingCTable()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("INSERT INTO nation VALUES('ZZZ', 'ABCDEF', 'MyXYZ', 'QWERTY')")))
		return false;

	session.StartTransaction();

	CTable <CAccessor<CNation>> nation;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	HRESULT hr = nation.Open(session, "nation", &propset);
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not find specific `code` in the table!");
		CloseConnection();
		return false;
	}

	hr = nation.MoveLast();
	if(FAILED(hr))
	{
		nation.Close();
		CloseConnection();
		return false;
	}

	ATLASSERT(EQ_STR(nation.m_Code, L"ZZZ") == true);

	//Update
	wcscpy(nation.m_Name, L"AAA");
	wcscpy(nation.m_Continent, L"BBB");
	wcscpy(nation.m_Capital, L"CCC");

	hr = nation.SetData();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not update values!");
		ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'"));
		nation.Close();
		CloseConnection();
		return false;
	}

	ATLASSERT(EQ_STR(nation.m_Name, L"AAA") == true);
	ATLASSERT(EQ_STR(nation.m_Continent, L"BBB") == true);
	ATLASSERT(EQ_STR(nation.m_Capital, L"CCC") == true);

	nation.Close();

	session.Commit();

	if(!ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_InsertFromAccessor()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CCommand <CAccessor<CNation>> nation_cmd;
	HRESULT hr;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	hr = nation_cmd.Open(session, L"SELECT * FROM nation", &propset);
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not open nation table!");
		CloseConnection();
		return false;
	}

	//Insert
	wcscpy(nation_cmd.m_Code, L"ZZZ");
	wcscpy(nation_cmd.m_Name, L"ABCDEF");
	wcscpy(nation_cmd.m_Continent, L"MyXYZ");
	wcscpy(nation_cmd.m_Capital, L"QWERTY");

	hr = nation_cmd.Insert();

	if(FAILED(hr))
	{
		OutputDebugString(L"Could not insert row!");
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	nation_cmd.Close();

	if(!ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_DeleteFromAccessor()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("INSERT INTO nation VALUES('ZZZ', 'ABCDEF', 'MyXYZ', 'QWERTY')")))
		return false;

	CCommand <CAccessor<CNation>> nation_cmd;
	HRESULT hr;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	hr = nation_cmd.Open(session, L"SELECT * FROM nation WHERE code = 'ZZZ'", &propset);
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not find specific code!");
		CloseConnection();
		return false;
	}

	hr = nation_cmd.MoveNext();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not move to specific row!");
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	//Delete
	hr = nation_cmd.Delete();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not delete row!");
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	nation_cmd.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_UpdateFromAccessorWithSetData()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("INSERT INTO nation VALUES('ZZZ', 'ABCDEF', 'MyXYZ', 'QWERTY')")))
		return false;

	session.StartTransaction();

	CCommand <CAccessor<CNation>> nation_cmd;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	HRESULT hr = nation_cmd.Open(session, L"select * from nation", &propset);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = nation_cmd.MoveLast();
	if(FAILED(hr))
	{
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	ATLASSERT(EQ_STR(nation_cmd.m_Code, L"ZZZ") == true);

	//Update
	wcscpy(nation_cmd.m_Name, L"AAA");
	wcscpy(nation_cmd.m_Continent, L"BBB");
	wcscpy(nation_cmd.m_Capital, L"CCC");

	hr = nation_cmd.SetData();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not update values!");
		ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'"));
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	ATLASSERT(EQ_STR(nation_cmd.m_Name, L"AAA") == true);
	ATLASSERT(EQ_STR(nation_cmd.m_Continent, L"BBB") == true);
	ATLASSERT(EQ_STR(nation_cmd.m_Capital, L"CCC") == true);

	nation_cmd.Close();

	session.Commit(); //Actually not needed - the CCommand close performs also the commit

	if(!ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_UpdateFromAccessorDefered()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("INSERT INTO nation VALUES('ZZZ', 'ABCDEF', 'MyXYZ', 'QWERTY')")))
		return false;

	session.StartTransaction();

	CCommand <CAccessor<CNation>> nation_cmd;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_IRowsetUpdate, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	HRESULT hr = nation_cmd.Open(session, L"select * from nation", &propset);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = nation_cmd.MoveLast();
	if(FAILED(hr))
	{
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	ATLASSERT(EQ_STR(nation_cmd.m_Code, L"ZZZ") == true);

	//Update
	wcscpy(nation_cmd.m_Name, L"AAA");
	wcscpy(nation_cmd.m_Continent, L"BBB");
	wcscpy(nation_cmd.m_Capital, L"CCC");

	hr = nation_cmd.SetData();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not update values!");
		ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'"));
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	hr = nation_cmd.Update();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not update values!");
		ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'"));
		nation_cmd.Close();
		CloseConnection();
		return false;
	}

	ATLASSERT(EQ_STR(nation_cmd.m_Name, L"AAA") == true);
	ATLASSERT(EQ_STR(nation_cmd.m_Continent, L"BBB") == true);
	ATLASSERT(EQ_STR(nation_cmd.m_Capital, L"CCC") == true);

	nation_cmd.Close();

	session.Commit(); //Actually not needed - the CCommand close performs also the commit

	if(!ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

//http://www.codeproject.com/Articles/6695/Using-the-ATL-OLE-DB-Consumer-Templates-on-the-Poc
bool Test_InsertImplicit()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CCommand <CNoAccessor, CNoRowset> nation_cmd;
	HRESULT hr;

	hr = nation_cmd.Open(session, L"INSERT INTO nation VALUES('ZZZ', 'ABCDEF', 'MyXYZ', 'QWERTY')");
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not insert row!");
		CloseConnection();
		return false;
	}

	nation_cmd.Close();

	if(!ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_DeleteImplicit()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("INSERT INTO nation VALUES('ZZZ', 'ABCDEF', 'MyXYZ', 'QWERTY')")))
		return false;

	CCommand <CNoAccessor, CNoRowset> nation_cmd;
	HRESULT hr;
	TCHAR *sql = (TCHAR *)calloc(1024, sizeof(TCHAR));

	//Delete
	wcscpy(sql, L"DELETE FROM nation WHERE `code` = 'ZZZ'");
	hr = nation_cmd.Open(session, sql);
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not delete row!");
		CloseConnection();
		return false;
	}

	nation_cmd.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_UpdateImplicit()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("INSERT INTO nation VALUES('ZZZ', 'ABCDEF', 'MyXYZ', 'QWERTY')")))
		return false;

	CCommand <CNoAccessor, CNoRowset> nation_cmd;
	HRESULT hr;

	hr = nation_cmd.Open(session, L"UPDATE nation SET `name` = 'ghijkl', `continent` = 'mY', `capital` = 'q' WHERE `code` = 'ZZZ'");
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not update values!");
		CloseConnection();
		return false;
	}

	nation_cmd.Close();

	if(!ExecuteSQL(_T("DELETE FROM nation WHERE `continent` = 'mY'")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_InsertExplicit()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CCommand <CAccessor<CNationInsert>> nation_cmd;
	HRESULT hr;

	//Insert
	wcscpy(nation_cmd.m_Code, L"ZZZ");
	wcscpy(nation_cmd.m_Name, L"ABCDEF");
	wcscpy(nation_cmd.m_Continent, L"MyXYZ");
	wcscpy(nation_cmd.m_Capital, L"QWERTY");

	hr = nation_cmd.Open(session, L"INSERT INTO nation VALUES(?, ?, ?, ?)");
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not insert row!");
		CloseConnection();
		return false;
	}

	nation_cmd.Close();

	if(!ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'ZZZ'")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_DeleteExplicit()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("INSERT INTO nation VALUES('ZZZ', 'ABCDEF', 'MyXYZ', 'QWERTY')")))
		return false;

	CCommand <CAccessor<CNationDelete>> nation_cmd;
	HRESULT hr;

	//Delete
	wcscpy(nation_cmd.m_Code, L"ZZZ");

	hr = nation_cmd.Open(session, L"DELETE FROM nation WHERE code = ?");
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not delete row!");
		CloseConnection();
		return false;
	}

	nation_cmd.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_UpdateExplicit()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	if(!ExecuteSQL(_T("INSERT INTO nation VALUES('ZZZ', 'ABCDEF', 'MyXYZ', 'QWERTY')")))
		return false;

	CCommand <CAccessor<CNationUpdate>> nation_cmd;
	HRESULT hr;

	//Update
	wcscpy(nation_cmd.m_Code, L"abc");
	wcscpy(nation_cmd.m_Name, L"abcabc");
	wcscpy(nation_cmd.m_Continent, L"abcab");
	wcscpy(nation_cmd.m_Capital, L"abcabcabc");

	hr = nation_cmd.Open(session, L"UPDATE nation SET `code` = ?, `name` = ?, continent = ?, capital = ?  WHERE `code` = 'ZZZ'");
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not update values!");
		ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'abc'"));
		nation_cmd.Close();		
		CloseConnection();
		return false;
	}

	ATLASSERT(EQ_STR(nation_cmd.m_Name, L"abcabc") == true);

	nation_cmd.Close();

	if(!ExecuteSQL(_T("DELETE FROM nation WHERE `code` = 'abc'")))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

