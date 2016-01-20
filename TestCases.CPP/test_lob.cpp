#include "stdafx.h"

#include <atldbcli.h>
#include <atlconv.h>
#include <oledb.h>
#include <oledberr.h>
#include <atldbcli.h>
#include <atlcoll.h>
#include <atldbsch.h> //schema support
#include <atlcom.h>

#include "utils\dynamicaccessorex.h"
#include "utils\dynamiccommand.h"


#include "globals.h"
#include "helpers.h"
#include "utils\ISSHelper.h"
#include "test_lob.h"

class LOB
{
public:
	long m_Id;
	ISequentialStream*   pPicture;
	
	BEGIN_COLUMN_MAP(LOB)
		COLUMN_ENTRY(1, m_Id)
		BLOB_ENTRY(2, IID_ISequentialStream, STGM_READWRITE, pPicture)
	END_COLUMN_MAP()

	DEFINE_COMMAND_EX(LOB, _T(" SELECT `id`, `str` FROM `TestLOB`"))

	void ClearRecord()
	{
		memset(this, 0, sizeof(*this));
	}
};

class LobInsert
{
public:
	//Parameter variable
	long m_Id;
	ISequentialStream* pParamPicture;
	ISequentialStream* pPicture;

	BEGIN_COLUMN_MAP(LobInsert)
		COLUMN_ENTRY(1, m_Id)
		BLOB_ENTRY(2, IID_ISequentialStream, STGM_READWRITE, pPicture)
	END_COLUMN_MAP()
	
	//Parameter binding map
	BEGIN_PARAM_MAP(LobInsert)
		SET_PARAM_TYPE(DBPARAMIO_INPUT)
		BLOB_ENTRY(2, IID_ISequentialStream, STGM_WRITE, pParamPicture)
	END_PARAM_MAP()
};

///
/// Test BLOB data retrieve
///
bool Test_GetBLOB()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;
	HRESULT hr;
	TCHAR *sql_create_table_testlob = L"CREATE TABLE `TestLOB`(`id` INT PRIMARY KEY, `str` BLOB)";
	TCHAR *sql_drop_table_testbob = L"DROP TABLE IF EXISTS `TestLOB`";
	TCHAR *sql_insert_table_testbob = L"INSERT INTO `TestLOB` VALUES(1, 'abcdefghijklmnopqrstuvwxyz1234567890')";
	TCHAR *testBlob = L"abcdefghijklmnopqrstuvwxyz1234567890";

	//create the `TestBlob` table
	if(!ExecuteSQL(sql_drop_table_testbob))
		return false;
	if(!ExecuteSQL(sql_create_table_testlob))
		return false;
	if(!ExecuteSQL(sql_insert_table_testbob))
		return false;
	
	CCommand<CAccessor<LOB>> blob;
	ULONG cb;
	BYTE myBuffer[65536];
	char *sql = "SELECT * FROM TestLOB";

	hr = blob.Open(session, sql);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the <testLOB> table!");
		CloseConnection();
		return false;
	}	
	
	if(blob.MoveFirst() == S_OK)//retrieve just one row
	{
		do
		{
			hr = blob.pPicture->Read(myBuffer, 65536, &cb);	
			
			for(ULONG i = 0; i < cb; i++)
				ATLASSERT(myBuffer[i] == testBlob[i]);
		}
		while (hr != S_FALSE);
	}
	else
	{
		return false;
	}
	
	if(!DropTable(L"TestLOB"))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test CLOB data retrieve
///
bool Test_GetCLOB()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	HRESULT hr;
	TCHAR *sql_create_table_testlob = L"CREATE TABLE `TestLOB`(`id` INT PRIMARY KEY, `str` CLOB)";
	TCHAR *sql_drop_table_testbob = L"DROP TABLE IF EXISTS `TestLOB`";
	TCHAR *sql_insert_table_testbob = L"INSERT INTO `TestLOB` VALUES(1, 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')";
	TCHAR *testClob = L"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

	//create the `TestBlob` table
	if(!ExecuteSQL(sql_drop_table_testbob))
		return false;
	if(!ExecuteSQL(sql_create_table_testlob))
		return false;
	if(!ExecuteSQL(sql_insert_table_testbob))
		return false;

	CTable<CAccessor<LOB>> clob;
	ULONG cb;
	BYTE myBuffer[65536];

	clob.Open(session, "TestLOB");
	if(clob.MoveFirst() == S_OK)
	{
		do
		{
			hr = clob.pPicture->Read(myBuffer, 65536, &cb);
			for(ULONG i = 0; i < cb; i++)
				ATLASSERT(myBuffer[i] == testClob[i]);
		}
		while (hr != S_FALSE);

	}
	else
	{
		return false;
	}

	if(!DropTable(L"TestLOB"))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}


///
/// Test CLOB data retrieve with DynamicAccessor
///
bool Test_GetCLOB_DynamicAccessor()
{
	if(!OpenConnection()) //Open connection to the demodb database
		return false;

	TCHAR *sql_create_table_testlob = L"CREATE TABLE `TestLOB`(`id` INT PRIMARY KEY, `str` CLOB)";
	TCHAR *sql_drop_table_testbob = L"DROP TABLE IF EXISTS `TestLOB`";
	
	//create the `TestBlob` table
	if(!ExecuteSQL(sql_drop_table_testbob))
		return false;
	if(!ExecuteSQL(sql_create_table_testlob))
		return false;

	//First, we need to insert blank field for blob object
	if(!ExecuteSQL(_T("insert into TestLOB values(1, 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')")))
		return false;

	CloseConnection(); //Close connection to the demodb database

	CDBPropSet dbRowset(DBPROPSET_ROWSET); 
	dbRowset.AddProperty(DBPROP_ISequentialStream, true); 
	dbRowset.AddProperty(DBPROP_IRowsetChange, true); 
	dbRowset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT);

	if(!OpenConnection()) //Open connection to the demodb database
		return false;

	CCommand<CDynamicAccessor> cmd;
	TCHAR *sql = L"Select str from TestLOB where id = 1";

	HRESULT hr = cmd.Open(session, sql, &dbRowset);
	hr = cmd.MoveNext();

	if(FAILED(hr))
	{
		cmd.Close();
		CloseConnection();
		return false;
	}

	IUnknown* pUnknown = *(IUnknown**)cmd.GetValue( 1 );
	CComPtr<ISequentialStream> spSequentialStream;
	hr = pUnknown->QueryInterface( __uuidof(ISequentialStream), (void**)&spSequentialStream );
	if( SUCCEEDED(hr) && spSequentialStream )
	{
		CHAR buffer[101];
		ULONG cbRead = 0;
		do
		{
			hr = spSequentialStream->Read( (void*)buffer, 1024, &cbRead );

			for(ULONG i = 0; i < cbRead; i++)
				ATLASSERT(buffer[i] == 'a');
		}
		while (hr != S_FALSE  && cbRead > 0);
	}
	
	cmd.Close();

	if(!DropTable(L"TestLOB"))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test BLOB data retrieve with DynamicAccessor
///
bool Test_GetBLOB_DynamicAccessor()
{
	if(!OpenConnection()) //Open connection to the demodb database
		return false;

	TCHAR *sql_create_table_testlob = L"CREATE TABLE `TestLOB`(`id` INT PRIMARY KEY, `str` BLOB)";
	TCHAR *sql_drop_table_testbob = L"DROP TABLE IF EXISTS `TestLOB`";
	
	//create the `TestBlob` table
	if(!ExecuteSQL(sql_drop_table_testbob))
		return false;
	if(!ExecuteSQL(sql_create_table_testlob))
		return false;
	if(!ExecuteSQL(_T("insert into TestLOB values(1, 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')")))
		return false;

	CloseConnection(); //Close connection to the demodb database

	CDBPropSet dbRowset(DBPROPSET_ROWSET);
	dbRowset.AddProperty(DBPROP_ISequentialStream, true); 
	dbRowset.AddProperty(DBPROP_IRowsetChange, true); 
	dbRowset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT);

	if(!OpenConnection()) //Open connection to the demodb database
		return false;

	CCommand<CDynamicAccessor> cmd;
	TCHAR *sql = L"Select str from TestLOB where id = 1";
	
	HRESULT hr = cmd.Open(session, sql, &dbRowset);
	
	hr = cmd.MoveNext();

	if(FAILED(hr))
	{
		cmd.Close();
		CloseConnection();
		return false;
	}

	IUnknown* pUnknown = *(IUnknown**)cmd.GetValue( 1 );
	CComPtr<ISequentialStream> spSequentialStream;
	hr = pUnknown->QueryInterface( __uuidof(ISequentialStream), (void**)&spSequentialStream );
	if( SUCCEEDED(hr) && spSequentialStream )
	{
		CHAR buffer[101];
		ULONG cbRead = 0;
		do
		{
			hr = spSequentialStream->Read( (void*)buffer, 1024, &cbRead );

			for(ULONG i = 0; i < cbRead; i++)
				ATLASSERT(buffer[i] == 'a');
		}
		while (hr != S_FALSE  && cbRead > 0);
	}
	
	cmd.Close();

	if(!DropTable(L"TestLOB"))
		return false;

	TestCleanup(); //Close the connection
	return true; //test case passed
}


bool Test_InsertBlobFromAccessor()
{
	if(!OpenConnection()) //Open connection to the demodb database
		return false;

	TCHAR *sql_create_table_testlob = L"CREATE TABLE `TestLOB`(`id` INT PRIMARY KEY, `str` BLOB)";
	TCHAR *sql_drop_table_testbob = L"DROP TABLE IF EXISTS `TestLOB`";
	
	//create the `TestBlob` table
	if(!ExecuteSQL(sql_drop_table_testbob))
		return false;
	if(!ExecuteSQL(sql_create_table_testlob))
		return false;

	CloseConnection(); //Close connection to the demodb database
	
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CCommand <CAccessor<LobInsert>> blob_cmd;
	HRESULT hr;
	TCHAR *sql = (TCHAR *)calloc(1024, sizeof(TCHAR));

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_ISequentialStream, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	wcscpy(sql, L"SELECT * FROM TestLOB");
	hr = blob_cmd.Open(session, sql, &propset);
	
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not open TestLOB table!");
		CloseConnection();
		return false;
	}
	
	//Insert
	blob_cmd.m_Id = 1;
	CHAR buffer[1024];
	ULONG cbWritten = 0;
	strcpy_s(buffer,"abcdefghijklmnopqrstuvwxyz1234567890");
	ULONG toWrite = strlen(buffer);
	do
	{
		blob_cmd.pParamPicture->Write( (void*)buffer, toWrite, &cbWritten );
		toWrite -= cbWritten;
	}
	while(toWrite > 0);
	
	hr = blob_cmd.Insert();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not insert the blob!");
		blob_cmd.Close();
		CloseConnection();
		return false;
	}

	CCommand<CAccessor<LOB>> blob;
	ULONG cb;
	BYTE myBuffer[65536];
	char *sql2 = "SELECT * FROM TestLOB";

	hr = blob.Open(session, sql2);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the <testLOB> table!");
		CloseConnection();
		return false;
	}	
	
	if(blob.MoveFirst() == S_OK)//retrieve just one row
	{
		do
		{
			hr = blob.pPicture->Read(myBuffer, 65536, &cb);	
			
			for(ULONG i = 0; i < cb; i++)
				ATLASSERT(myBuffer[i] == buffer[i]);
		}
		while (hr != S_FALSE);
	}
	else
	{
		return false;
	}
	blob.Close();

	blob_cmd.Close();
	
	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_InsertClobFromAccessor()
{
	if(!OpenConnection()) //Open connection to the demodb database
		return false;
	
	TCHAR *sql_create_table_testlob = L"CREATE TABLE `TestLOB`(`id` INT PRIMARY KEY, `str` CLOB)";
	TCHAR *sql_drop_table_testbob = L"DROP TABLE IF EXISTS `TestLOB`";
	
	//create the `TestBlob` table
	if(!ExecuteSQL(sql_drop_table_testbob))
		return false;
	if(!ExecuteSQL(sql_create_table_testlob))
		return false;
	
	CloseConnection(); //Close connection to the demodb database
	
	if(!TestSetup()) //Open connection to the demodb database
		return false;
	
	CCommand <CAccessor<LobInsert>> clob_cmd;
	HRESULT hr;
	TCHAR *sql = (TCHAR *)calloc(1024, sizeof(TCHAR));

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_ISequentialStream, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);
	
	wcscpy(sql, L"SELECT * FROM TestLOB");
	hr = clob_cmd.Open(session, sql, &propset);
	
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not open TestLOB table!");
		CloseConnection();
		return false;
	}
	
	//Insert
	clob_cmd.m_Id = 1;
	CHAR buffer[1024];
	ULONG cbWritten = 0;
	strcpy_s(buffer,"abcdefghijklmnopqrstuvwxyz1234567890");
	ULONG toWrite = strlen(buffer);
	do
	{
		clob_cmd.pParamPicture->Write( (void*)buffer, toWrite, &cbWritten );
		toWrite -= cbWritten;
	}
	while(toWrite > 0);
	
	hr = clob_cmd.Insert();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not insert the clob!");
		clob_cmd.Close();
		CloseConnection();
		return false;
	}
	clob_cmd.Close();

	CCommand<CAccessor<LOB>> clob;
	ULONG cb;
	BYTE myBuffer[65536];
	char *sql2 = "SELECT * FROM TestLOB";
	
	hr = clob.Open(session, sql2);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the <testLOB> table!");
		CloseConnection();
		return false;
	}	
	
	if(clob.MoveFirst() == S_OK)//retrieve just one row
	{
		do
		{
			hr = clob.pPicture->Read(myBuffer, 65536, &cb);	
			
			for(ULONG i = 0; i < cb; i++)
				ATLASSERT(myBuffer[i] == buffer[i]);
		}
		while (hr != S_FALSE);
	}
	else
	{
		return false;
	}
	
	clob.Close();
	
	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_UpdateBlobFromAccessor()
{
	if(!OpenConnection()) //Open connection to the demodb database
		return false;
	
	TCHAR *sql_create_table_testlob = L"CREATE TABLE `TestLOB`(`id` INT PRIMARY KEY, `str` BLOB)";
	TCHAR *sql_drop_table_testbob = L"DROP TABLE IF EXISTS `TestLOB`";
	
	//create the `TestBlob` table
	if(!ExecuteSQL(sql_drop_table_testbob))
		return false;
	if(!ExecuteSQL(sql_create_table_testlob))
		return false;
	
	CloseConnection(); //Close connection to the demodb database

	if(!TestSetup()) //Open connection to the demodb database
		return false;
	
	if(!ExecuteSQL(_T("INSERT INTO TestLOB VALUES( 1, 'QWERTY')")))
		return false;

	session.StartTransaction();
	
	CCommand <CAccessor<LobInsert>> blob_cmd;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_ISequentialStream, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	HRESULT hr = blob_cmd.Open(session, L"select * from TestLOB", &propset);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = blob_cmd.MoveLast();
	if(FAILED(hr))
	{
		blob_cmd.Close();
		CloseConnection();
		return false;
	}

	ATLASSERT(blob_cmd.m_Id == 1);

	//Update
	blob_cmd.m_Id = 2;
	CHAR buffer[1024];
	ULONG cbWritten = 0;
	strcpy_s(buffer,"abcdefghijklmnopqrstuvwxyz1234567890");
	ULONG toWrite = strlen(buffer);
	do
	{
		blob_cmd.pParamPicture->Write( (void*)buffer, toWrite, &cbWritten );
		toWrite -= cbWritten;
	}
	while(toWrite > 0);
	
	hr = blob_cmd.SetData();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not update values!");
		ExecuteSQL(_T("DELETE FROM TestLOB"));
		blob_cmd.Close();
		CloseConnection();
		return false;
	}

	CCommand<CAccessor<LOB>> blob;
	ULONG cb;
	BYTE myBuffer[1024];
	char *sql2 = "SELECT * FROM TestLOB";

	hr = blob.Open(session, sql2);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the <testLOB> table!");
		CloseConnection();
		return false;
	}
	
	if(blob.MoveFirst() == S_OK)//retrieve just one row
	{
		ATLASSERT(blob_cmd.m_Id == 2);
		do
		{
			hr = blob.pPicture->Read(myBuffer, 1024, &cb);
			
			for(ULONG i = 0; i < cb; i++)
				ATLASSERT(myBuffer[i] == buffer[i]);
		}
		while (hr != S_FALSE);
	}
	else
	{
		return false;
	}
	blob.Close();

	blob_cmd.Close();

	session.Commit(); //Actually not needed - the CCommand close performs also the commit

	if(!ExecuteSQL(_T("DROP TABLE TestLOB")))
		return false;
	
	TestCleanup(); //Close the connection
	return true; //test case passed
}

bool Test_UpdateClobFromAccessor()
{
	if(!OpenConnection()) //Open connection to the demodb database
		return false;
	
	TCHAR *sql_create_table_testlob = L"CREATE TABLE `TestLOB`(`id` INT PRIMARY KEY, `str` CLOB)";
	TCHAR *sql_drop_table_testlob = L"DROP TABLE IF EXISTS `TestLOB`";
	
	//create the `TestBlob` table
	if(!ExecuteSQL(sql_drop_table_testlob))
		return false;
	if(!ExecuteSQL(sql_create_table_testlob))
		return false;
	
	CloseConnection(); //Close connection to the demodb database

	if(!TestSetup()) //Open connection to the demodb database
		return false;
	
	if(!ExecuteSQL(_T("INSERT INTO TestLOB VALUES( 1, 'QWERTY')")))
		return false;

	session.StartTransaction();
	
	CCommand <CAccessor<LobInsert>> clob_cmd;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_CANFETCHBACKWARDS, true);
	propset.AddProperty(DBPROP_ISequentialStream, true);
	propset.AddProperty(DBPROP_IRowsetScroll, true);
	propset.AddProperty(DBPROP_IRowsetChange, true);
	propset.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_CHANGE | DBPROPVAL_UP_INSERT | DBPROPVAL_UP_DELETE);

	HRESULT hr = clob_cmd.Open(session, L"select * from TestLOB", &propset);
	if(FAILED(hr))
	{
		CloseConnection();
		return false;
	}

	hr = clob_cmd.MoveLast();
	if(FAILED(hr))
	{
		clob_cmd.Close();
		CloseConnection();
		return false;
	}

	ATLASSERT(clob_cmd.m_Id == 1);

	//Update
	clob_cmd.m_Id = 2;
	CHAR buffer[1024];
	ULONG cbWritten = 0;
	strcpy_s(buffer,"abcdefghijklmnopqrstuvwxyz1234567890");
	ULONG toWrite = strlen(buffer);
	do
	{
		clob_cmd.pParamPicture->Write( (void*)buffer, toWrite, &cbWritten );
		toWrite -= cbWritten;
	}
	while(toWrite > 0);
	
	hr = clob_cmd.SetData();
	if(FAILED(hr))
	{
		OutputDebugString(L"Could not update values!");
		ExecuteSQL(_T("DELETE FROM TestLOB"));
		clob_cmd.Close();
		CloseConnection();
		return false;
	}
	
	CCommand<CAccessor<LOB>> clob;
	ULONG cb;
	BYTE myBuffer[1024];
	char *sql2 = "SELECT * FROM TestLOB";

	hr = clob.Open(session, sql2);
	if(FAILED(hr))
	{
		OutputDebugString(L"Couldn't open the <testLOB> table!");
		CloseConnection();
		return false;
	}
	
	if(clob.MoveFirst() == S_OK)//retrieve just one row
	{
		ATLASSERT(clob_cmd.m_Id == 2);
		do
		{
			hr = clob.pPicture->Read(myBuffer, 1024, &cb);
			
			for(ULONG i = 0; i < cb; i++)
				ATLASSERT(myBuffer[i] == buffer[i]);
		}
		while (hr != S_FALSE);
	}
	else
	{
		return false;
	}
	clob.Close();

	clob_cmd.Close();

	session.Commit(); //Actually not needed - the CCommand close performs also the commit

	if(!ExecuteSQL(_T("DROP TABLE TestLOB")))
		return false;
	
	TestCleanup(); //Close the connection
	return true; //test case passed
}