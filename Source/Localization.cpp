////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Localization.cpp : Localization support

#include "stdafx.h"

CLocale::localizedMessages CLocale::messages[] = 
{
	{ ID_START_MESSAGE,						L"",			L"" },
	{ ID_E_UNEXPECTED,						L"en_US", L"Catastrophic failure" },
	{ ID_E_FAIL,									L"en_US", L"A provider-specific error occurred" },
	{ ID_E_NOINTERFACE,						L"en_US", L"No such interface supported" },
	{ ID_E_INVALIDARG,						L"en_US", L"The parameter is incorrect." },
	{ ID_DB_E_BADACCESSORHANDLE,	L"en_US", L"Accessor is invalid." },
	{ ID_DB_E_ROWLIMITEXCEEDED,		L"en_US", L"Row could not be inserted into the rowset without exceeding provider's maximum number of active rows." },
	{ ID_DB_E_READONLYACCESSOR,		L"en_US", L"Accessor is read-only. Operation failed." },
	{ ID_DB_E_SCHEMAVIOLATION,		L"en_US", L"Given values violate the database schema." },
	{ ID_DB_E_BADROWHANDLE,				L"en_US", L"Row handle is invalid." },
	{ ID_DB_E_OBJECTOPEN,					L"en_US", L"Object was open." },
	{ ID_DB_E_BADCHAPTER,					L"en_US", L"Chapter is invalid." },
	{ ID_DB_E_CANTCONVERTVALUE,		L"en_US", L"Data or literal value could not be converted to the type of the column in the data source, and the provider was unable to determine which columns could not be converted.  Data overflow or sign mismatch was not the cause." },
	{ ID_DB_E_BADBINDINFO,				L"en_US", L"Binding information is invalid." },
	{ DB_SEC_E_PERMISSIONDENIED,	L"en_US", L"Permission denied." },
	{ ID_DB_E_NOTAREFERENCECOLUMN, L"en_US", L"Specified column does not contain bookmarks or chapters." },
	{ ID_DB_E_LIMITREJECTED,			L"en_US", L"Cost limits were rejected." },
	{ ID_DB_E_NOCOMMAND,					L"en_US", L"Command text was not set for the command object." },
	{ ID_DB_E_COSTLIMIT,					L"en_US", L"Query plan within the cost limit cannot be found." },
	{ ID_DB_E_BADBOOKMARK,				L"en_US", L"Bookmark is invalid." },
	{ ID_DB_E_BADLOCKMODE,				L"en_US", L"Lock mode is invalid." },
	{ ID_DB_E_PARAMNOTOPTIONAL,		L"en_US", L"No value given for one or more required parameters." },
	{ ID_DB_E_BADCOLUMNID,				L"en_US", L"Column ID is invalid." },
	{ ID_DB_E_BADRATIO,						L"en_US", L"Numerator was greater than denominator. Values must express ratio between zero and 1." },
	{ ID_DB_E_BADVALUES,					L"en_US", L"Value is invalid." },
	{ ID_DB_E_ERRORSINCOMMAND,		L"en_US", L"One or more errors occurred during processing of command." },
	{ ID_DB_E_CANTCANCEL,					L"en_US", L"Command cannot be canceled." },
	{ ID_DB_E_DIALECTNOTSUPPORTED, L"en_US", L"Command dialect is not supported by this provider." },
	{ ID_DB_E_DUPLICATEDATASOURCE, L"en_US", L"Data source object could not be created because the named data source already exists." },
	{ ID_DB_E_CANNOTRESTART,			L"en_US", L"The rowset was built over a live data feed and cannot be restarted." },
	{ ID_DB_E_NOTFOUND,						L"en_US", L"No key matching the described characteristics could be found within the current range." },
	{ ID_DB_E_NEWLYINSERTED,			L"en_US", L"Identity cannot be determined for newly inserted rows." },
	{ ID_DB_E_CANNOTFREE,					L"en_US", L"Provider has ownership of this tree." },
	{ ID_DB_E_GOALREJECTED,				L"en_US", L"Goal was rejected because no nonzero weights were specified for any goals supported. Current goal was not changed." },
	{ ID_DB_E_UNSUPPORTEDCONVERSION, L"en_US", L"Requested conversion is not supported." },
	{ ID_DB_E_BADSTARTPOSITION,		L"en_US", L"Goal was rejected because no nonzero weights were specified for any goals supported. Current goal was not changed." },
	{ ID_DB_E_NOQUERY,						L"en_US", L"Information was requested for a query, and the query was not set." },
	{ ID_DB_E_NOTREENTRANT,				L"en_US", L"Consumer's event handler called a non-reentrant method in the provider." },
	{ ID_DB_E_ERRORSOCCURRED,			L"en_US", L"Multiple-step OLE DB operation generated errors. Check each OLE DB status value, if available. No work was done." },
	{ ID_DB_E_NOAGGREGATION,			L"en_US", L"Non-NULL controlling IUnknown was specified, and either the requested interface was not IUnknown, or the provider does not support COM aggregation." },
	{ ID_DB_E_DELETEDROW,					L"en_US", L"Row handle referred to a deleted row or a row marked for deletion." },
	{ ID_DB_E_CANTFETCHBACKWARDS, L"en_US", L"The rowset does not support fetching backwards." },
	{ ID_DB_E_ROWSNOTRELEASED,		L"en_US", L"Row handles must all be released before new ones can be obtained." },
	{ ID_DB_E_BADSTORAGEFLAG,			L"en_US", L"One or more storage flags are not supported." },
	{ ID_DB_E_BADCOMPAREOP,				L"en_US", L"Comparison operator is invalid." },
	{ ID_DB_E_BADSTATUSVALUE,			L"en_US", L"The specified status flag was neither DBCOLUMNSTATUS_OK nor DBCOLUMNSTATUS_ISNULL." },
	{ ID_DB_E_CANTSCROLLBACKWARDS, L"en_US", L"Rowset does not support scrolling backward." },
	{ ID_DB_E_BADREGIONHANDLE,		L"en_US", L"Region handle is invalid." },
	{ ID_DB_E_NONCONTIGUOUSRANGE, L"en_US", L"Set of rows is not contiguous to, or does not overlap, the rows in the watch region." },
	{ ID_DB_E_INVALIDTRANSITION,	L"en_US", L"A transition from ALL* to MOVE* or EXTEND* was specified." },
	{ ID_DB_E_NOTASUBREGION,			L"en_US", L"The specified region is not a proper subregion of the region identified by the given watch region handle." },
	{ ID_DB_E_MULTIPLESTATEMENTS, L"en_US", L"Multiple-statement commands are not supported by this provider." },
	{ ID_DB_E_INTEGRITYVIOLATION, L"en_US", L"A specified value violated the integrity constraints for a column or table." },
	{ ID_DB_E_BADTYPENAME,				L"en_US", L"Type name is invalid." },
	{ ID_DB_E_ABORTLIMITREACHED,	L"en_US", L"Execution stopped because a resource limit was reached. No results were returned." },
	{ ID_DB_E_ROWSETINCOMMAND,		L"en_US", L"Command object whose command tree contains a rowset or rowsets cannot be cloned." },
	{ ID_DB_E_CANTTRANSLATE,			L"en_US", L"Current tree cannot be represented as text." },
	{ ID_DB_E_DUPLICATEINDEXID,		L"en_US", L"The specified index already exists." },
	{ ID_DB_E_NOINDEX,						L"en_US", L"The specified index does not exist." },
	{ ID_DB_E_INDEXINUSE,					L"en_US", L"Index is in use." },
	{ ID_DB_E_NOTABLE,						L"en_US", L"The specified table does not exist." },
	{ ID_DB_E_CONCURRENCYVIOLATION, L"en_US", L"The rowset was using optimistic concurrency and the value of a column has been changed since it was last read." },
	{ ID_DB_E_BADCOPY,						L"en_US", L"Errors were detected during the copy." },
	{ ID_DB_E_BADPRECISION,				L"en_US", L"Precision is invalid." },
	{ ID_DB_E_BADSCALE,						L"en_US", L"Scale is invalid." },
	{ ID_DB_E_BADTABLEID,					L"en_US", L"Table ID is invalid." },
	{ ID_DB_E_BADTYPE,						L"en_US", L"Type is invalid." },
	{ ID_DB_E_DUPLICATECOLUMNID,	L"en_US", L"Column ID already exists or occurred more than once in the array of columns." },
	{ ID_DB_E_DUPLICATETABLEID,		L"en_US", L"The specified table already exists." },
	{ ID_DB_E_TABLEINUSE,					L"en_US", L"Table is in use." },
	{ ID_DB_E_NOLOCALE,						L"en_US", L"The specified locale ID was not supported." },
	{ ID_DB_E_BADRECORDNUM,				L"en_US", L"Record number is invalid." },
	{ ID_DB_E_BOOKMARKSKIPPED,		L"en_US", L"Although the bookmark was validly formed, no row could be found to match it." },
	{ ID_DB_E_BADPROPERTYVALUE,		L"en_US", L"Property value is invalid." },
	{ ID_DB_E_INVALID,						L"en_US", L"The rowset was not chaptered." },
	{ ID_DB_E_BADACCESSORFLAGS,		L"en_US", L"One or more accessor flags were invalid." },
	{ ID_DB_E_BADSTORAGEFLAGS,		L"en_US", L"One or more storage flags are invalid." },
	{ ID_DB_E_BYREFACCESSORNOTSUPPORTED, L"en_US", L"Reference accessors are not supported by this provider." },
	{ ID_DB_E_NULLACCESSORNOTSUPPORTED, L"en_US", L"Null accessors are not supported by this provider." },
	{ ID_DB_E_NOTPREPARED,				L"en_US", L"The command was not prepared." },
	{ ID_DB_E_BADACCESSORTYPE,		L"en_US", L"The specified accessor was not a parameter accessor." },
	{ ID_DB_E_WRITEONLYACCESSOR,	L"en_US", L"The given accessor was write-only." },
	{ DB_SEC_E_AUTH_FAILED,				L"en_US", L"Authentication failed." },
	{ ID_DB_E_CANCELED,						L"en_US", L"Operation was canceled." },
	{ ID_DB_E_CHAPTERNOTRELEASED, L"en_US", L"The rowset was single-chaptered and the chapter was not released." },
	{ ID_DB_E_BADSOURCEHANDLE,		L"en_US", L"Source handle is invalid." },
	{ ID_DB_E_PARAMUNAVAILABLE,		L"en_US", L"Provider cannot derive parameter information and SetParameterInfo has not been called." },
	{ ID_DB_E_ALREADYINITIALIZED, L"en_US", L"The data source object is already initialized." },
	{ ID_DB_E_NOTSUPPORTED,				L"en_US", L"Method is not supported by this provider." },
	{ ID_DB_E_MAXPENDCHANGESEXCEEDED, L"en_US", L"Number of rows with pending changes exceeded the limit." },
	{ ID_DB_E_BADORDINAL,					L"en_US", L"The specified column did not exist." },
	{ ID_DB_E_PENDINGCHANGES,			L"en_US", L"There are pending changes on a row with a reference count of zero." },
	{ ID_DB_E_DATAOVERFLOW,				L"en_US", L"Literal value in the command exceeded the range of the type of the associated column." },
	{ ID_DB_E_BADHRESULT,					L"en_US", L"HRESULT is invalid." },
	{ ID_DB_E_BADLOOKUPID,				L"en_US", L"Lookup ID is invalid." },
	{ ID_DB_E_BADDYNAMICERRORID,	L"en_US", L"DynamicError ID is invalid." },
	{ ID_DB_E_PENDINGINSERT,			L"en_US", L"Most recent data for a newly inserted row could not be retrieved because the insert is pending." },
	{ ID_DB_E_BADCONVERTFLAG,			L"en_US", L"Conversion flag is invalid." },
	{ ID_DB_E_OBJECTMISMATCH,			L"en_US", L"Operation is not allowed to this column type" },
	{ ID_DB_E_ERROR_DESCRIPTION,	L"en_US", L"" },
	{ ID_DB_E_ERROR_GENERIC,			L"en_US", L"Error Description" },
	{ ID_OBJECT_ZOMBIE_STATE,			L"en_US", L"This object is in a zombie state" },
	{ ID_MAX_MESSAGE,							L"",			L""}
};


WCHAR *CLocale::Msg(int msgID)
{
	return CLocale::Msg(msgID, L"en_US");
}

WCHAR *CLocale::Msg(int msgID, WCHAR *locale)
{
	ATLTRACE(atlTraceDBProvider, 2, "CLocale::Msg\n");

	try
	{
		for(int i = ID_START_MESSAGE + 1; i <= ID_MAX_MESSAGE - 1; i++) //exclude first and last
		{
			if(!wcscmp(CLocale::messages[msgID].locale, locale) == true && CLocale::messages[msgID].msgId == msgID)
			{
				return CLocale::messages[msgID].msg;
			}
		}
	}
	catch(...)
	{
		ATLTRACE(atlTraceDBProvider, 0, "CLocale::Msg Exception!\n");
	}

	return L"Unknown error.";
}


