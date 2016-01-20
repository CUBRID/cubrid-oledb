#include "stdafx.h"

#include <atldbcli.h>
#include <atlconv.h>
#include <oledb.h>
#include <oledberr.h>
#include <atldbcli.h>
#include <atlcoll.h>
#include <atldbsch.h> //schema support

#include "utils\console.h"
#include "helpers.h"

#include "testcase_template.h"
#include "test_connect.h"
#include "test_schema.h"
#include "test_lob.h"
#include "test_provider.h"
#include "test_transaction.h"
#include "test_parameters.h"
#include "test_bookmarks.h"
#include "test_end2end.h"
#include "test_commands.h"
#include "test_other_accessors.h"
#include "test_accessor.h"
#include "test_no_accessor.h"
#include "test_dynamic_accessor.h"
#include "test_IUD.h"
#include "test_errors.h"
#include "test_locale.h"
#include "test_datatypes.h"
#include "test_hotfixes.h"

#include <iostream>
using namespace std;

int passed = 0; //no. of test cases that pass
int failed = 0; //no. of test cases that fail

void ECHO_OK()
{
	Console::SetTextColor(Console::LightGreen); 
	std::cout << "OK.";     
	Console::SetTextColor(Console::White);
	std::cout << endl;
	passed++;
}

void ECHO_FAILED()
{
	Console::SetTextColor(Console::LightRed); 
	std::cout << "Failed!";     
	Console::SetTextColor(Console::White);
	std::cout << endl;
	failed++;
}

#define RUN_TEST_CASE(test_case); cout << "Executing " << #test_case << "... ";test_case() ? (ECHO_OK()) : (ECHO_FAILED());
#define RUN_EXPECT_FAILURE_TEST_CASE(test_case); cout << "Executing " << #test_case << "... ";test_case() ? (ECHO_FAILED()) : (ECHO_OK());
#define RUN_FAKE_FAILURE_TEST_CASE(test_case); RUN_TEST_CASE(!test_case);

//Global task memory allocator
//http://msdn.microsoft.com/en-us/library/windows/desktop/ms678418%28v=vs.85%29.aspx
IMalloc* g_pIMalloc = NULL;
//Connection string to the standard 'demodb' database
LPCOLESTR connectString = L"Provider=CUBRID.Provider;Location=localhost;Data Source=demodb;User Id=public;Port=33000";
CDataSource ds;
CSession session;

//Setup&Cleanup procedures for the test cases suite
//They are executed only once, no matter how many test cases will be run in the test suite
bool TestSuiteSetup();
void TestSuiteCleanup();

///
/// This is the main() function, which executes all the test cases in the suite, in the defined sequential order
///
void _tmain(int argc, _TCHAR* argv[])
{
	Console::SetTextColor( Console::Yellow);
	cout << "Test cases execution started..." << endl;
	Console::SetTextColor( Console::White);

	//Initialize test cases suite data
	if(!TestSuiteSetup())
		return;

	//////////////////////////////////////////////////////////////
	// Test cases suite - Start section                         //
	//////////////////////////////////////////////////////////////
	
	//Dymmy test cases set suite
	//Expected results: 1 passed, 2 failed
	RUN_TEST_CASE(Test_XXXXX);
	RUN_EXPECT_FAILURE_TEST_CASE(Test_XXXXX);
	RUN_FAKE_FAILURE_TEST_CASE(Test_XXXXX);

	//Connect
	RUN_TEST_CASE(Test_Connect_Basic);
	//RUN_TEST_CASE(Test_Connect_CDBPropSet);
	//RUN_TEST_CASE(Test_Connect_CDBPropSet_ProviderString);
	RUN_TEST_CASE(Test_Connect_Extended);
	RUN_TEST_CASE(Test_Connect_Extended_2);
	RUN_TEST_CASE(Test_Connect_Timeout);

	//Provider properties
	RUN_TEST_CASE(Test_Providers_List);
	RUN_TEST_CASE(Test_Provider_CLSID);

	//TODO Investigate heap corruption in the next text case, in cci_connect(...) call (util.cpp)
	RUN_TEST_CASE(Test_Connect_WrongParams_CDBPropSet);

	//Commands
	RUN_TEST_CASE(Test_Simple_Commands);

	//Schema
	RUN_TEST_CASE(Test_Schema_Tables);
	RUN_TEST_CASE(Test_Schema_Columns);
	RUN_TEST_CASE(Test_Schema_ProviderTypes);
	RUN_TEST_CASE(Test_Schema_TablePrivileges);
	RUN_TEST_CASE(Test_Schema_ColumnPrivileges);
	RUN_TEST_CASE(Test_Schema_TableConstraints);
	RUN_TEST_CASE(Test_Schema_Statistics);
	RUN_TEST_CASE(Test_Schema_Indexes);
	RUN_TEST_CASE(Test_Schema_ViewColumnUsage);
	RUN_TEST_CASE(Test_Schema_Views);
	RUN_TEST_CASE(Test_Schema_PrimaryKeys);
	RUN_TEST_CASE(Test_Schema_ForeignKeys);

	//Not implemented yet
	RUN_FAKE_FAILURE_TEST_CASE(Test_Schema_Procedures); //Not implemented yet
	RUN_FAKE_FAILURE_TEST_CASE(Test_Schema_Contraints); //Not implemented yet

	//Basic SELECTs
	RUN_TEST_CASE(Test_DynamicAccessorEx_Select);
	RUN_TEST_CASE(Test_DynamicAccessor_SelectCOUNT);
	RUN_TEST_CASE(Test_DynamicAccessor_Select_Table);
	RUN_TEST_CASE(Test_DynamicAccessor_Select_FixedValues);
	RUN_TEST_CASE(Test_DynamicAccessor_Rowset_Select_FixedValues);

	//Transactions
	RUN_TEST_CASE(Test_Rollback);
	RUN_TEST_CASE(Test_Commit);

	//Other Rowset types
	RUN_TEST_CASE(Test_CArrayRowset);
	RUN_TEST_CASE(Test_CBulkRowset);

	//End-2-End scenarios
	RUN_TEST_CASE(Test_End2End_AccessorTable);
	RUN_TEST_CASE(Test_End2End_Rowset);

	//NoAccessor
	RUN_TEST_CASE(Test_NoAccessor);

	//Other Accessors
	RUN_TEST_CASE(Test_XMLAccessor);

	//Accessor
	RUN_TEST_CASE(Test_Accessor_Simple_SELECT);
	RUN_TEST_CASE(Test_Accessor_Command_CDBPropSet);

	//DynamicAccessor
	RUN_TEST_CASE(Test_DynamicAccessor);
	RUN_TEST_CASE(Test_DynamicStringAccessor);
	RUN_TEST_CASE(Test_Override_DynamicAccessor);
	RUN_TEST_CASE(Test_DynamicStringAccessor_ColumnsStatus);

	//Bookmarks
	RUN_TEST_CASE(Test_Bookmark);
	RUN_TEST_CASE(Test_Bookmark_Navigate);

	//Parameters
	RUN_TEST_CASE(Test_Simple_Parameter);
	RUN_EXPECT_FAILURE_TEST_CASE(Test_UseDynamicParameterAccessor); //Not supported
	
	//Test I(nsert), U(pdate), D(elete) operations, Implicit & Explicit, using Accessor, CTable
	RUN_TEST_CASE(Test_InsertImplicit);
	RUN_TEST_CASE(Test_InsertExplicit);
	RUN_TEST_CASE(Test_InsertFromAccessor);
	RUN_TEST_CASE(Test_InsertFromAccessorUsingCTable);
	
	RUN_TEST_CASE(Test_UpdateImplicit);
	RUN_TEST_CASE(Test_UpdateExplicit);
	RUN_TEST_CASE(Test_UpdateFromAccessorDefered);
	RUN_TEST_CASE(Test_UpdateFromAccessorWithSetData);
	RUN_TEST_CASE(Test_UpdateFromAccessorUsingCTable);
	RUN_TEST_CASE(Test_Accessor_UPDATE);
	
	RUN_TEST_CASE(Test_DeleteImplicit);
	RUN_TEST_CASE(Test_DeleteExplicit);
	RUN_TEST_CASE(Test_DeleteFromAccessor);
	RUN_TEST_CASE(Test_DeleteFromAccessorUsingCTable);

	//Data types
	RUN_TEST_CASE(Test_Dynamic_DataTypes_SELECT);
	RUN_TEST_CASE(Test_Dynamic_DataTypes_INSERT);
	RUN_TEST_CASE(Test_DataTypes_INSERT);

	//Encoding
	RUN_TEST_CASE(Test_Encoding_Varchar);
	RUN_TEST_CASE(Test_Encoding_Char);
	RUN_TEST_CASE(Test_Encoding_String);

	//Errors
	RUN_TEST_CASE(Test_Errors);

	//BLOB/CLOB
	RUN_TEST_CASE(Test_GetBLOB);
	RUN_TEST_CASE(Test_GetCLOB);
	RUN_TEST_CASE(Test_GetBLOB_DynamicAccessor);
	RUN_TEST_CASE(Test_GetCLOB_DynamicAccessor);
	RUN_TEST_CASE(Test_InsertBlobFromAccessor);
	RUN_TEST_CASE(Test_InsertClobFromAccessor);
	RUN_TEST_CASE(Test_UpdateBlobFromAccessor);
	RUN_TEST_CASE(Test_UpdateClobFromAccessor);

	////Hotfixes
	//RUN_TEST_CASE(Test_Hotfixes_DataTypes);


	//////////////////////////////////////////////////////////////
	// Test cases suite - End section                           //
	//////////////////////////////////////////////////////////////

	//Cleanup test cases suite data
	TestSuiteCleanup();

	Console::SetTextColor(Console::White);
	cout << endl << "*** Test cases results ***" << endl;
	Console::SetTextColor(Console::LightGreen);
	cout << passed << " test case(s) Passed." << endl;
	if(failed > 0)
	{
		Console::SetTextColor(Console::LightRed);
		cout << failed << " test case(s) Failed." << endl;
		Console::SetTextColor(Console::White);
	}
	cout << "Press any key to continue..." << endl;
	std::getchar();
}

///
/// This methods contains initialization/setup code for the test cases suite
/// It is execute only once, before any test cases
///
bool TestSuiteSetup()
{
	HRESULT hr = OleInitialize(NULL);
	if (FAILED(hr))
	{
		OutputDebugString(L"Couldn't initialize OLE!");
		return false;
	}
	//Get the task memory allocator
	if (FAILED(CoGetMalloc(MEMCTX_TASK, &g_pIMalloc)))
	{
		OutputDebugString(L"Couldn't get the task memory allocator!");
		return false;
	}

	return true;
}

///
/// This methods contains cleanup code for the test cases suite
/// It is execute only once, after all test cases in the suite are executed
///
void TestSuiteCleanup()
{
	//Release the task memory allocator
	g_pIMalloc->Release();
	//Unitialize OLE
	OleUninitialize();
}

