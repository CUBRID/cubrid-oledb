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

#include "testcase_template.h"

///
/// Write here a description of the test case purpose
///
bool Test_XXXXX()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	//Do your stuff here
	//...

	//HRESULT hr = ...
	//if(FAILED(hr))
	//{
	//	OutputDebugString(L"...<write an error messagge here>...!");
	//	CloseConnection();
	//	return false;
	//}

	TestCleanup(); //Close the connection
	return true; //test case passed
}


