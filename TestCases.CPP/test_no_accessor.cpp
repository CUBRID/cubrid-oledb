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
#include "test_no_accessor.h"

///
/// Test No Accessor Command
///
bool Test_NoAccessor()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CCommand<CNoAccessor, CNoRowset> cmd;

	CDBPropSet ps(DBPROPSET_ROWSET);
	ps.AddProperty(DBPROP_IRowsetChange, true);
	ps.AddProperty(DBPROP_UPDATABILITY, DBPROPVAL_UP_DELETE);

	HRESULT hr = cmd.Open(session, "CREATE TABLE t(id INT)", &ps);
	if(FAILED(hr))
		return false;

	cmd.Close();

	hr = cmd.Open(session, "DROP TABLE IF EXISTS t", &ps);
	if(FAILED(hr))
		return false;

	cmd.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}
