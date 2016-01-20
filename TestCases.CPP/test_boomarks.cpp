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
#include "test_bookmarks.h"

///
/// Accessor class for the `code` table in the demodb database
///
class CCode
{
public:
	// Data Elements
	CHAR m_sz_s_name[1];
	CHAR m_sz_f_name[6];

	CBookmark<4> bookmark;

	// Column binding map
	BEGIN_COLUMN_MAP(CCode)
		COLUMN_ENTRY(1, m_sz_s_name)
		COLUMN_ENTRY(2, m_sz_f_name)
	END_COLUMN_MAP()
};

///
/// Test Accessor Rowset Bookmark
///
bool Test_Bookmark()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CDBPropSet propset(DBPROPSET_ROWSET);
	propset.AddProperty(DBPROP_BOOKMARKS, true);

	CTable<CAccessor<CCode>> code;
	code.Open(session, "`code`", &propset);

	code.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

///
/// Test Bookmark navigation
///
bool Test_Bookmark_Navigate()
{
	if(!TestSetup()) //Open connection to the demodb database
		return false;

	CCommand<CDynamicAccessor> rs;

	CDBPropSet pset( DBPROPSET_ROWSET );
	pset.AddProperty( DBPROP_ISequentialStream, true, DBPROPOPTIONS_OPTIONAL );
	pset.AddProperty( DBPROP_IStream, true, DBPROPOPTIONS_OPTIONAL );
	pset.AddProperty( DBPROP_CANFETCHBACKWARDS, true, DBPROPOPTIONS_OPTIONAL );
	pset.AddProperty( DBPROP_CANSCROLLBACKWARDS, true, DBPROPOPTIONS_OPTIONAL );
	bool b=pset.AddProperty(DBPROP_CLIENTCURSOR,true,DBPROPOPTIONS_REQUIRED);

	rs.SetBlobHandling( DBBLOBHANDLING_SKIP );
	HRESULT hr=rs.Open(session, L"select * from nation", &pset );
	DBORDINAL count=rs.GetColumnCount();

	ATLASSERT(count == 4);

	CBookmark<> firstmark;
	rs.MoveNext();
	char* pV=static_cast<char*>(rs.GetValue(1));
	hr=rs.GetBookmark(&firstmark);

	while(S_OK==rs.MoveNext())
	{
		CBookmark<> bookmark;
		hr=rs.GetBookmark(&bookmark);
		BYTE* p=bookmark.GetBuffer();
	}

	pV=static_cast<char*>(rs.GetValue(1));
	ATLASSERT(strcmp(pV, "AFG") == 0);

	hr=rs.MoveToBookmark(firstmark);

	pV=static_cast<char*>(rs.GetValue(1));
	ATLASSERT(strcmp(pV, "AFG") == 0);

	rs.Close();

	TestCleanup(); //Close the connection
	return true; //test case passed
}

