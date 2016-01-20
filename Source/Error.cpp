////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Error.h"
#include "DataSource.h"
#include "CUBRIDStream.h"

void ClearError()
{
	ATLTRACE2(atlTraceDBProvider, 2, "Error::ClearError\n");

	SetErrorInfo(0, NULL);
}

HRESULT RaiseError(ERRORINFO &info, CComVariant &var, BSTR bstrSQLState)
{
	ATLTRACE2(atlTraceDBProvider, 2, "Error::RaiseError\n");

	CComPtr<IErrorInfo> spErrorInfo;
	GetErrorInfo(0, &spErrorInfo);

	if(spErrorInfo == NULL)
		spErrorInfo.CoCreateInstance(CLSID_EXTENDEDERRORINFO, NULL, CLSCTX_INPROC_SERVER);

	DISPPARAMS dispparams = { NULL, NULL, 0, 0 };
	if(V_VT(&var) != VT_EMPTY)
	{
		dispparams.rgvarg = &var;
		dispparams.cArgs = 1;
	}

	CComPtr<IUnknown> spSQLInfo;
	if(bstrSQLState)
	{
		CComPolyObject<CCUBRIDErrorInfo> *pObj;
		CComPolyObject<CCUBRIDErrorInfo>::CreateInstance(NULL, &pObj);

		CCUBRIDErrorInfo *pSQLInfo = &(pObj->m_contained);
		pSQLInfo->m_bstrSQLState = bstrSQLState;
		pSQLInfo->m_lNativeError = info.dwMinor;

		pSQLInfo->QueryInterface(__uuidof(IUnknown), (void **)&spSQLInfo);
	}

	CComPtr<IErrorRecords> spErrorRecords;
	spErrorInfo->QueryInterface(__uuidof(IErrorRecords), (void **)&spErrorRecords);
	spErrorRecords->AddErrorRecord(&info, info.dwMinor, &dispparams, spSQLInfo, 0);

	SetErrorInfo(0, spErrorInfo);

	return info.hrError;
}

HRESULT RaiseError(HRESULT hrError, DWORD dwMinor, IID iid, CComVariant &var, BSTR bstrSQLState)
{
	ATLTRACE2(atlTraceDBProvider, 2, "Error::RaiseError\n");

	ERRORINFO info;
	info.hrError = hrError;
	info.dwMinor = dwMinor;
	info.clsid = CCUBRIDDataSource::GetObjectCLSID();
	info.iid = iid;
	info.dispid = 0;

	return RaiseError(info, var, bstrSQLState);
}

HRESULT RaiseError(HRESULT hrError, DWORD dwMinor, IID iid, LPCSTR pszText, BSTR bstrSQLState)
{
	ATLTRACE2(atlTraceDBProvider, 2, "Error::RaiseError\n");

	CComVariant var;
	if(pszText)
		var = pszText;

	return RaiseError(hrError, dwMinor, iid, var, bstrSQLState);
}

HRESULT RaiseError(HRESULT hrError, DWORD dwMinor, IID iid, LPWSTR pwszText, BSTR bstrSQLState)
{
	ATLTRACE2(atlTraceDBProvider, 2, "Error::RaiseError\n");

	CComVariant var;
	if(pwszText)
		var = pwszText;

	return RaiseError(hrError, dwMinor, iid, var, bstrSQLState);
}

//http://msdn.microsoft.com/en-us/library/ms171852.aspx
static LPOLESTR GetErrorDescriptionString(HRESULT hr)
{
	ATLTRACE2(atlTraceDBProvider, 2, "Error::GetErrorDescriptionString\n");

	static struct
	{
		HRESULT hr;
		LPOLESTR str;
	}
	desc[] =
	{
		{ E_UNEXPECTED, CLocale::Msg(CLocale::ID_E_UNEXPECTED) },
		{ E_FAIL, CLocale::Msg(CLocale::ID_E_FAIL) },
		{ E_NOINTERFACE, CLocale::Msg(CLocale::ID_E_NOINTERFACE) },
		{ E_INVALIDARG, CLocale::Msg(CLocale::ID_E_INVALIDARG) },
		{ DB_E_BADACCESSORHANDLE, CLocale::Msg(CLocale::ID_DB_E_BADACCESSORHANDLE) },
		{ DB_E_ROWLIMITEXCEEDED, CLocale::Msg(CLocale::ID_DB_E_READONLYACCESSOR) },
		{ DB_E_READONLYACCESSOR, CLocale::Msg(CLocale::ID_DB_E_READONLYACCESSOR) },
		{ DB_E_SCHEMAVIOLATION, CLocale::Msg(CLocale::ID_DB_E_SCHEMAVIOLATION) },
		{ DB_E_BADROWHANDLE, CLocale::Msg(CLocale::ID_DB_E_BADROWHANDLE) },
		{ DB_E_OBJECTOPEN, CLocale::Msg(CLocale::ID_DB_E_OBJECTOPEN) },
		{ DB_E_BADCHAPTER, CLocale::Msg(CLocale::ID_DB_E_BADCHAPTER) },
		{ DB_E_CANTCONVERTVALUE, CLocale::Msg(CLocale::ID_DB_E_CANTCONVERTVALUE) },
		{ DB_E_BADBINDINFO, CLocale::Msg(CLocale::ID_DB_E_BADBINDINFO) },
		{ DB_SEC_E_PERMISSIONDENIED, CLocale::Msg(CLocale::ID_DB_SEC_E_PERMISSIONDENIED) },
		{ DB_E_NOTAREFERENCECOLUMN, CLocale::Msg(CLocale::ID_DB_E_NOTAREFERENCECOLUMN) },
		{ DB_E_LIMITREJECTED, CLocale::Msg(CLocale::ID_DB_E_LIMITREJECTED) },
		{ DB_E_NOCOMMAND, CLocale::Msg(CLocale::ID_DB_E_NOCOMMAND) },
		{ DB_E_COSTLIMIT, CLocale::Msg(CLocale::ID_DB_E_COSTLIMIT) },
		{ DB_E_BADBOOKMARK, CLocale::Msg(CLocale::ID_DB_E_BADBOOKMARK) },
		{ DB_E_BADLOCKMODE, CLocale::Msg(CLocale::ID_DB_E_BADLOCKMODE) },
		{ DB_E_PARAMNOTOPTIONAL, CLocale::Msg(CLocale::ID_DB_E_PARAMNOTOPTIONAL) },
		{ DB_E_BADCOLUMNID, CLocale::Msg(CLocale::ID_DB_E_BADRATIO) },
		{ DB_E_BADRATIO, CLocale::Msg(CLocale::ID_DB_E_BADRATIO) },
		{ DB_E_BADVALUES, CLocale::Msg(CLocale::ID_DB_E_BADVALUES) },
		{ DB_E_ERRORSINCOMMAND, CLocale::Msg(CLocale::ID_DB_E_ERRORSINCOMMAND) },
		{ DB_E_CANTCANCEL, CLocale::Msg(CLocale::ID_DB_E_CANTCANCEL) },
		{ DB_E_DIALECTNOTSUPPORTED, CLocale::Msg(CLocale::ID_DB_E_DIALECTNOTSUPPORTED) },
		{ DB_E_DUPLICATEDATASOURCE, CLocale::Msg(CLocale::ID_DB_E_DUPLICATEDATASOURCE) },
		{ DB_E_CANNOTRESTART, CLocale::Msg(CLocale::ID_DB_E_CANNOTRESTART) },
		{ DB_E_NOTFOUND, CLocale::Msg(CLocale::ID_DB_E_NOTFOUND) },
		{ DB_E_NEWLYINSERTED, CLocale::Msg(CLocale::ID_DB_E_NEWLYINSERTED) },
		{ DB_E_CANNOTFREE, CLocale::Msg(CLocale::ID_DB_E_CANNOTFREE) },
		{ DB_E_GOALREJECTED, CLocale::Msg(CLocale::ID_DB_E_GOALREJECTED) },
		{ DB_E_UNSUPPORTEDCONVERSION, CLocale::Msg(CLocale::ID_DB_E_UNSUPPORTEDCONVERSION) },
		{ DB_E_BADSTARTPOSITION, CLocale::Msg(CLocale::ID_DB_E_BADSTARTPOSITION) },
		{ DB_E_NOQUERY, CLocale::Msg(CLocale::ID_DB_E_NOQUERY) },
		{ DB_E_ERRORSOCCURRED, CLocale::Msg(CLocale::ID_DB_E_ERRORSOCCURRED) },
		{ DB_E_NOAGGREGATION, CLocale::Msg(CLocale::ID_DB_E_NOAGGREGATION) },
		{ DB_E_DELETEDROW, CLocale::Msg(CLocale::ID_DB_E_DELETEDROW) },
		{ DB_E_CANTFETCHBACKWARDS, CLocale::Msg(CLocale::ID_DB_E_CANTFETCHBACKWARDS) },
		{ DB_E_ROWSNOTRELEASED, CLocale::Msg(CLocale::ID_DB_E_ROWSNOTRELEASED) },
		{ DB_E_BADSTORAGEFLAG, CLocale::Msg(CLocale::ID_DB_E_BADSTORAGEFLAG) },
		{ DB_E_BADCOMPAREOP, CLocale::Msg(CLocale::ID_DB_E_BADCOMPAREOP) },
		{ DB_E_BADSTATUSVALUE, CLocale::Msg(CLocale::ID_DB_E_BADSTATUSVALUE) },
		{ DB_E_CANTSCROLLBACKWARDS, CLocale::Msg(CLocale::ID_DB_E_CANTSCROLLBACKWARDS) },
		{ DB_E_BADREGIONHANDLE, CLocale::Msg(CLocale::ID_DB_E_BADREGIONHANDLE) },
		{ DB_E_NONCONTIGUOUSRANGE, CLocale::Msg(CLocale::ID_DB_E_NONCONTIGUOUSRANGE) },
		{ DB_E_INVALIDTRANSITION, CLocale::Msg(CLocale::ID_DB_E_INVALIDTRANSITION) },
		{ DB_E_NOTASUBREGION, CLocale::Msg(CLocale::ID_DB_E_NOTASUBREGION) },
		{ DB_E_MULTIPLESTATEMENTS, CLocale::Msg(CLocale::ID_DB_E_MULTIPLESTATEMENTS) },
		{ DB_E_INTEGRITYVIOLATION, CLocale::Msg(CLocale::ID_DB_E_INTEGRITYVIOLATION) },
		{ DB_E_BADTYPENAME, CLocale::Msg(CLocale::ID_DB_E_BADTYPENAME) },
		{ DB_E_ABORTLIMITREACHED, CLocale::Msg(CLocale::ID_DB_E_ABORTLIMITREACHED) },
		{ DB_E_ROWSETINCOMMAND, CLocale::Msg(CLocale::ID_DB_E_ROWSETINCOMMAND) },
		{ DB_E_CANTTRANSLATE, CLocale::Msg(CLocale::ID_DB_E_CANTTRANSLATE) },
		{ DB_E_CANTTRANSLATE, CLocale::Msg(CLocale::ID_DB_E_CANTTRANSLATE) },
		{ DB_E_NOINDEX, CLocale::Msg(CLocale::ID_DB_E_NOINDEX) },
		{ DB_E_INDEXINUSE, CLocale::Msg(CLocale::ID_DB_E_NOTABLE) },
		{ DB_E_NOTABLE, CLocale::Msg(CLocale::ID_DB_E_NOTABLE) },
		{ DB_E_CONCURRENCYVIOLATION, CLocale::Msg(CLocale::ID_DB_E_CONCURRENCYVIOLATION) },
		{ DB_E_BADCOPY, CLocale::Msg(CLocale::ID_DB_E_BADCOPY) },
		{ DB_E_BADPRECISION, CLocale::Msg(CLocale::ID_DB_E_BADPRECISION) },
		{ DB_E_BADSCALE, CLocale::Msg(CLocale::ID_DB_E_BADSCALE) },
		{ DB_E_BADTABLEID, CLocale::Msg(CLocale::ID_DB_E_BADTABLEID) },
		{ DB_E_BADTYPE, CLocale::Msg(CLocale::ID_DB_E_BADTYPE) },
		{ DB_E_DUPLICATECOLUMNID, CLocale::Msg(CLocale::ID_DB_E_DUPLICATECOLUMNID) },
		{ DB_E_DUPLICATETABLEID, CLocale::Msg(CLocale::ID_DB_E_DUPLICATETABLEID) },
		{ DB_E_TABLEINUSE, CLocale::Msg(CLocale::ID_DB_E_TABLEINUSE) },
		{ DB_E_NOLOCALE, CLocale::Msg(CLocale::ID_DB_E_NOLOCALE) },
		{ DB_E_BADRECORDNUM, CLocale::Msg(CLocale::ID_DB_E_BADRECORDNUM) },
		{ DB_E_BOOKMARKSKIPPED, CLocale::Msg(CLocale::ID_DB_E_BOOKMARKSKIPPED) },
		{ DB_E_BADPROPERTYVALUE, CLocale::Msg(CLocale::ID_DB_E_BADPROPERTYVALUE) },
		{ DB_E_INVALID, CLocale::Msg(CLocale::ID_DB_E_INVALID) },
		{ DB_E_BADACCESSORFLAGS, CLocale::Msg(CLocale::ID_DB_E_BADACCESSORFLAGS) },
		{ DB_E_BADSTORAGEFLAGS, CLocale::Msg(CLocale::ID_DB_E_BADSTORAGEFLAGS) },
		{ DB_E_BYREFACCESSORNOTSUPPORTED, CLocale::Msg(CLocale::ID_DB_E_NULLACCESSORNOTSUPPORTED) },
		{ DB_E_NULLACCESSORNOTSUPPORTED, CLocale::Msg(CLocale::ID_DB_E_NULLACCESSORNOTSUPPORTED) },
		{ DB_E_NOTPREPARED, CLocale::Msg(CLocale::ID_DB_E_NOTPREPARED) },
		{ DB_E_BADACCESSORTYPE, CLocale::Msg(CLocale::ID_DB_E_BADACCESSORTYPE) },
		{ DB_E_WRITEONLYACCESSOR, CLocale::Msg(CLocale::ID_DB_SEC_E_AUTH_FAILED) },
		{ DB_SEC_E_AUTH_FAILED, CLocale::Msg(CLocale::ID_DB_SEC_E_AUTH_FAILED) },
		{ DB_E_CANCELED, CLocale::Msg(CLocale::ID_DB_E_CANCELED) },
		{ DB_E_CHAPTERNOTRELEASED, CLocale::Msg(CLocale::ID_DB_E_CHAPTERNOTRELEASED) },
		{ DB_E_BADSOURCEHANDLE, CLocale::Msg(CLocale::ID_DB_E_PARAMUNAVAILABLE) },
		{ DB_E_PARAMUNAVAILABLE, CLocale::Msg(CLocale::ID_DB_E_PARAMUNAVAILABLE) },
		{ DB_E_ALREADYINITIALIZED, CLocale::Msg(CLocale::ID_DB_E_ALREADYINITIALIZED) },
		{ DB_E_NOTSUPPORTED, CLocale::Msg(CLocale::ID_DB_E_NOTSUPPORTED) },
		{ DB_E_MAXPENDCHANGESEXCEEDED, CLocale::Msg(CLocale::ID_DB_E_BADORDINAL) },
		{ DB_E_BADORDINAL, CLocale::Msg(CLocale::ID_DB_E_BADORDINAL) },
		{ DB_E_PENDINGCHANGES, CLocale::Msg(CLocale::ID_DB_E_PENDINGCHANGES) },
		{ DB_E_DATAOVERFLOW, CLocale::Msg(CLocale::ID_DB_E_DATAOVERFLOW) },
		{ DB_E_BADHRESULT, CLocale::Msg(CLocale::ID_DB_E_BADHRESULT) },
		{ DB_E_BADLOOKUPID, CLocale::Msg(CLocale::ID_DB_E_BADLOOKUPID) },
		{ DB_E_BADDYNAMICERRORID, CLocale::Msg(CLocale::ID_DB_E_BADDYNAMICERRORID) },
		{ DB_E_PENDINGINSERT, CLocale::Msg(CLocale::ID_DB_E_PENDINGINSERT) },
		{ DB_E_BADCONVERTFLAG, CLocale::Msg(CLocale::ID_DB_E_BADCONVERTFLAG) },
		{ DB_E_OBJECTMISMATCH, CLocale::Msg(CLocale::ID_DB_E_OBJECTMISMATCH) }
	};

	for(int i = 0; i < sizeof(desc) / sizeof(*desc); i++)
	{
		if(desc[i].hr == hr)
			return desc[i].str;
	}

	return CLocale::Msg(CLocale::ID_DB_E_ERROR_DESCRIPTION);
}

STDMETHODIMP CCUBRIDErrorLookup::GetErrorDescription(HRESULT hrError,
																										 DWORD dwLookupID, DISPPARAMS *pdispparams, LCID lcid,
																										 BSTR *pbstrSource, BSTR *pbstrDescription)
{
	ATLTRACE2(atlTraceDBProvider, 2, "Error::GetErrorDescription\n");

	if(pbstrSource == NULL || pbstrDescription == NULL)
		return E_INVALIDARG;

	CComBSTR strSource = "CUBRID.Provider";
	*pbstrSource = strSource.Detach();

	CComBSTR strDesc;

	if(dwLookupID == 0)
	{
		strDesc = GetErrorDescriptionString(hrError);
	}
	else if(dwLookupID == 1) // cci error
	{
		strDesc = V_BSTR(&pdispparams[0].rgvarg[0]);
	}

	*pbstrDescription = strDesc.Detach();

	return S_OK;
}

STDMETHODIMP CCUBRIDErrorLookup::GetHelpInfo(HRESULT hrError, DWORD dwLookupID,
																						 LCID lcid, BSTR *pbstrHelpFile, DWORD *pdwHelpContext)
{
	ATLTRACE2(atlTraceDBProvider, 2, "Error::GetHelpInfo\n");

	if(pbstrHelpFile == NULL || pdwHelpContext == NULL)
		return E_INVALIDARG;

	*pbstrHelpFile = 0;
	*pdwHelpContext = 0;

	return S_OK;
}

STDMETHODIMP CCUBRIDErrorLookup::ReleaseErrors(const DWORD dwDynamicErrorID)
{
	ATLTRACE2(atlTraceDBProvider, 2, "Error::ReleaseErrors\n");

	return S_OK;
}

STDMETHODIMP CCUBRIDErrorInfo::GetSQLInfo(BSTR *pbstrSQLState, LONG *plNativeError)
{
	ATLTRACE2(atlTraceDBProvider, 2, "Error::GetSQLInfo\n");

	if(pbstrSQLState)
		*pbstrSQLState = 0;
	if(plNativeError)
		*plNativeError = 0;
	if(pbstrSQLState == NULL || plNativeError == NULL)
		return E_INVALIDARG;

	*pbstrSQLState = SysAllocString(m_bstrSQLState);
	if(*pbstrSQLState == NULL)
		return E_OUTOFMEMORY;

	*plNativeError = m_lNativeError;

	return S_OK;
}
