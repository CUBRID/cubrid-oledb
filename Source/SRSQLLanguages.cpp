////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Session.h"
#include "Error.h"

//http://msdn.microsoft.com/en-us/library/ms714374%28v=vs.85%29.aspx

CSRSQLLanguages::~CSRSQLLanguages()
{
	if(m_spUnkSite)
	{
		CCUBRIDSession* pSession = CCUBRIDSession::GetSessionPtr(this);
		if (pSession->m_cSessionsOpen == 1)
			pSession->RowsetCommit();
	}
}

static HRESULT FetchData(int hReq, CSQLLanguagesRow &trData)
{
	char *value;
	int ind, res;
	T_CCI_ERROR err_buf;

	res = cci_fetch(hReq, &err_buf);
	if(res < 0)
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

	{
		res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &value, &ind);
		if(res < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

		CA2W str(value);
		wcsncpy(trData.m_szSQL_LANGUAGE_SOURCE, str, 128);
		trData.m_szSQL_LANGUAGE_SOURCE[128] = 0;
	}
	{
		res = cci_get_data(hReq, 2, CCI_A_TYPE_STR, &value, &ind);
		if(res < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

		CA2W str(value);
		wcsncpy(trData.m_szSQL_LANGUAGE_YEAR, str, 128);
		trData.m_szSQL_LANGUAGE_YEAR[128] = 0;
	}
	{
		res = cci_get_data(hReq, 3, CCI_A_TYPE_STR, &value, &ind);
		if(res < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

		CA2W str(value);
		wcsncpy(trData.m_szSQL_LANGUAGE_CONFORMANCE, str, 128);
		trData.m_szSQL_LANGUAGE_CONFORMANCE[128] = 0;
	}
	{
		res = cci_get_data(hReq, 4, CCI_A_TYPE_STR, &value, &ind);
		if(res < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

		CA2W str(value);
		wcsncpy(trData.m_szSQL_LANGUAGE_INTEGRITY, str, 128);
		trData.m_szSQL_LANGUAGE_INTEGRITY[128] = 0;
	}
	{
		res = cci_get_data(hReq, 5, CCI_A_TYPE_STR, &value, &ind);
		if(res < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

		CA2W str(value);
		wcsncpy(trData.m_szSQL_LANGUAGE_BINDING_STYLE, str, 128);
		trData.m_szSQL_LANGUAGE_BINDING_STYLE[128] = 0;
	}

	trData.m_szSQL_LANGUAGE_IMPLEMENTATION[0] = NULL;
	trData.m_szSQL_LANGUAGE_PROGRAMMING_LANGUAGE[0] = NULL;
	//memset(trData.m_szSQL_LANGUAGE_IMPLEMENTATION, '\0', 128); //NULL
	//memset(trData.m_szSQL_LANGUAGE_PROGRAMMING_LANGUAGE, '\0', 128); //NULL

	return S_OK;
}

HRESULT CSRSQLLanguages::Execute(LONG * /*pcRowsAffected*/, ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRSQLLanguages::Execute\n");

	ClearError();

	int hConn = -1;
	HRESULT hr = CCUBRIDSession::GetSessionPtr(this)->GetConnectionHandle(&hConn);
	if(FAILED(hr))
		return hr;

	{
		T_CCI_ERROR err_buf;

		int hReq = cci_prepare(hConn, "select 'ISO 9075', '1992', 'INTERMEDIATE', 'NO', NULL, 'DIRECT', NULL", 0, &err_buf);
		if(hReq < 0)
		{
			ATLTRACE2("cci_prepare fail\n");
			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);
		}

		hReq = cci_execute(hReq, 0, 0, &err_buf);
		if(hReq < 0)
		{
			ATLTRACE2("cci_execute fail\n");
			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);
		}

		int res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
		if(res == CCI_ER_NO_MORE_DATA)
			goto done;
		if(res < 0)
			goto error;

		while(true)
		{
			CSQLLanguagesRow trData;
			hr = FetchData(hReq, trData);
			if(FAILED(hr))
			{
				cci_close_req_handle(hReq);
				return hr;
			}

			if(hr == S_OK)
			{
				_ATLTRY
				{
					m_rgRowData.InsertAt(m_rgRowData.GetCount(), trData);
				}
				_ATLCATCHALL()
				{
					ATLTRACE2("Out of memory\n");
					cci_close_req_handle(hReq);

					return E_OUTOFMEMORY;
				}
			}

			res = cci_cursor(hReq, 1, CCI_CURSOR_CURRENT, &err_buf);
			if(res == CCI_ER_NO_MORE_DATA)
				goto done;
			if(res < 0)
				goto error;
		}

error:
		ATLTRACE2("Fail to fetch data\n");
		cci_close_req_handle(hReq);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

done:
		cci_close_req_handle(hReq);
	}

	return S_OK;
}

DBSTATUS CSRSQLLanguages::GetDBStatus(CSimpleRow*, ATLCOLUMNINFO* pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 2, _T("CSRSQLLanguages::GetDBStatus[%d]\n"), pInfo->iOrdinal);
	switch(pInfo->iOrdinal)
	{
	case 1: // SQL_LANGUAGE_SOURCE
	case 2: // SQL_LANGUAGE_YEAR
	case 3: // SQL_LANGUAGE_CONFORMANCE
	case 4: // SQL_LANGUAGE_INTEGRITY
	case 6: // SQL_LANGUAGE_BINDING_STYLE
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_S_ISNULL;
	}
}
