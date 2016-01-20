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
#include "test_commands.h"

///
/// A basic test case, with some commands to be executed, with no Rowset access
///
bool Test_Simple_Commands()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	TCHAR *sql_create_table = L"CREATE TABLE `t`(`id` INT)";
	TCHAR *sql_insert_in_table = L"INSERT INTO `t` VALUES(1)";
	TCHAR *sql_update_table = L"UPDATE `t` SET `id` = 2";
	TCHAR *sql_delete_in_table = L"DELETE FROM `t` WHERE `id` = 2";
	TCHAR *sql_drop_table = L"DROP TABLE `t`";

	CCommand<CNoAccessor, CNoRowset> cmd;

	HRESULT hr = cmd.Open(session, sql_create_table);
	if(FAILED(hr))
		return false;
	cmd.Close();

	hr = cmd.Open(session, sql_insert_in_table);
	if(FAILED(hr))
		return false;
	cmd.Close();

	hr = cmd.Open(session, sql_update_table);
	if(FAILED(hr))
		return false;
	cmd.Close();

	hr = cmd.Open(session, sql_delete_in_table);
	if(FAILED(hr))
		return false;
	cmd.Close();

	hr = cmd.Open(session, sql_drop_table);
	if(FAILED(hr))
		return false;
	cmd.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

