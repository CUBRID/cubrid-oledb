////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

void ClearError();

HRESULT RaiseError(ERRORINFO &info, CComVariant &var, BSTR bstrSQLState = 0);
HRESULT RaiseError(HRESULT hrError, DWORD dwMinor, IID iid, CComVariant &var, BSTR bstrSQLState = 0);
HRESULT RaiseError(HRESULT hrError, DWORD dwMinor, IID iid, LPCSTR pszText, BSTR bstrSQLState = 0);
HRESULT RaiseError(HRESULT hrError, DWORD dwMinor, IID iid, LPWSTR pwszText = 0, BSTR bstrSQLState = 0);

[
	coclass,
	threading(apartment),
	vi_progid("CUBRID.ErrorLookup"),
	progid("CUBRID.ErrorLookup.8.4"),
	version(8.4),
	uuid("3165D76D-CB91-482f-9378-00C216FD5F32"),
	helpstring("CUBRID OLE DB Provider Error Lookup Service"),
	registration_script("CUBRIDErrorLookup.rgs")
]

class ATL_NO_VTABLE CCUBRIDErrorLookup :
	public IErrorLookup
{
public:
	STDMETHOD(GetErrorDescription)(HRESULT hrError, DWORD dwLookupID,
		DISPPARAMS *pdispparams, LCID lcid,
		BSTR *pbstrSource, BSTR *pbstrDescription);
	STDMETHOD(GetHelpInfo)(HRESULT hrError, DWORD dwLookupID, LCID lcid,
		BSTR *pbstrHelpFile, DWORD *pdwHelpContext);
	STDMETHOD(ReleaseErrors)(const DWORD dwDynamicErrorID);
};

[
	coclass,
	threading(apartment),
	noncreatable,
	uuid("ED0E5A7D-89F5-4862-BEF3-20E551E1D07B"),
	registration_script("none")
]

class ATL_NO_VTABLE CCUBRIDErrorInfo :
	public ISQLErrorInfo
{
public:
	CComBSTR m_bstrSQLState;
	LONG m_lNativeError;
	STDMETHOD(GetSQLInfo)(BSTR *pbstrSQLState, LONG *plNativeError);
};
