////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Session.h"
#include "Error.h"
#include "CUBRIDStream.h"

CSRStatistics::~CSRStatistics()
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRStatistics::~CSRStatistics\n");

	if(m_spUnkSite)
	{
		CCUBRIDSession* pSession = CCUBRIDSession::GetSessionPtr(this);
		if (pSession->m_cSessionsOpen == 1)
			pSession->RowsetCommit();
	}
}

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions, char *table_name)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRStatistics::GetRestrictions\n");

	if(cRestrictions >= 3 && V_VT(&rgRestrictions[2]) == VT_BSTR && V_BSTR(&rgRestrictions[2]) != NULL)
	{
		// TABLE_NAME restriction
		CW2A name(V_BSTR(&rgRestrictions[2]));
		ATLTRACE2("\tTable Name = %s\n", (LPSTR)name);

		strncpy(table_name, name, 255);
		table_name[255] = 0; // ensure zero-terminated string
	}
}

static HRESULT FetchData(int hReq, CStatisticsRow &tprData)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRStatistics::FetchData\n");

	char *value;
	int ind, res;
	T_CCI_ERROR err_buf;

	res = cci_fetch(hReq, &err_buf);
	if(res < 0)
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

	res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &value, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	wcscpy(tprData.m_szTableName, CA2W(value));

	return S_OK;
}

static HRESULT GetCardinality(int hConn, WCHAR* tableName, ULARGE_INTEGER* cardinality)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRStatistics::GetCardinality\n");

	CComBSTR query;
	T_CCI_ERROR error;
	int hReq, res, ind;

	query.Append("select count(*) from [");
	query.Append(tableName);
	query.Append("]");

	hReq = cci_prepare(hConn, CW2A(query.m_str), 0, &error);
	if (hReq < 0)
		return S_FALSE;

	res = cci_execute(hReq, CCI_EXEC_QUERY_ALL, 0, &error);
	if (res < 0)
		return S_FALSE;

	res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &error);
	if(res < 0)
		return S_FALSE;

	res = cci_fetch(hReq, &error);
	if(res < 0)
		return S_FALSE;

	res = cci_get_data(hReq, 1, CCI_A_TYPE_INT, &cardinality->QuadPart, &ind);
	if(res < 0)
		return S_FALSE;

	cci_close_req_handle(hReq);

	return S_OK;
}

HRESULT CSRStatistics::Execute(LONG* pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRStatistics::Execute\n");

	ClearError();

	int hConn = -1;
	HRESULT hr = CCUBRIDSession::GetSessionPtr(this)->GetConnectionHandle(&hConn);
	if(FAILED(hr))
		return hr;

	char table_name[256];
	memset(table_name, '\0', sizeof(table_name));

	GetRestrictions(cRestrictions, rgRestrictions, table_name);
	{
		T_CCI_ERROR err_buf;
		int hReq = cci_schema_info(hConn, CCI_SCH_CLASS, (table_name[0] ? table_name : NULL),
			NULL, CCI_CLASS_NAME_PATTERN_MATCH, &err_buf);
		if(hReq < 0)
		{
			ATLTRACE2("cci_schema_info fail\n");
			return E_FAIL;
		}

		int res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
		if(res == CCI_ER_NO_MORE_DATA)
			goto done;
		if(res < 0)
			goto error;

		if (pcRowsAffected)
			*pcRowsAffected = 0;

		while(true)
		{
			CStatisticsRow tprData;
			hr = FetchData(hReq, tprData);
			hr = GetCardinality(hConn, tprData.m_szTableName, &tprData.m_ulCardinality);

			if(FAILED(hr))
			{
				cci_close_req_handle(hReq);
				return hr;
			}

			if(hr == S_OK)
			{
				_ATLTRY
				{
					size_t nPos;
					for( nPos = 0 ; nPos < m_rgRowData.GetCount() ; nPos++ )
					{
						int res = CompareStringW(LOCALE_USER_DEFAULT,
							NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT | NORM_IGNORECASE,
							m_rgRowData[nPos].m_szTableName, -1,
							tprData.m_szTableName, -1);
						if(res == CSTR_GREATER_THAN)
							break;
					}
					m_rgRowData.InsertAt(nPos, tprData);
				}
				_ATLCATCHALL()
				{
					ATLTRACE2("out of memory\n");
					cci_close_req_handle(hReq);
					return E_OUTOFMEMORY;
				}
				if (pcRowsAffected) (*pcRowsAffected)++;
			}

			res = cci_cursor(hReq, 1, CCI_CURSOR_CURRENT, &err_buf);
			if(res == CCI_ER_NO_MORE_DATA)
				goto done;
			if(res < 0)
				goto error;
		}

error:
		ATLTRACE2("fail to fetch data\n");
		cci_close_req_handle(hReq);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

done:
		cci_close_req_handle(hReq);
	}

	return S_OK;
}

DBSTATUS CSRStatistics::GetDBStatus(CSimpleRow *pRow, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRStatistics::GetDBStatus\n");

	switch(pInfo->iOrdinal)
	{
	case 3: // TABLE_NAME
	case 4: // CARDINALITY
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_S_ISNULL;
	}
}
