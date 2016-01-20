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

CSRTablesInfo::~CSRTablesInfo()
{
	if(m_spUnkSite)
	{
		CCUBRIDSession* pSession = CCUBRIDSession::GetSessionPtr(this);
		if (pSession->m_cSessionsOpen == 1)
			pSession->RowsetCommit();
	}
}

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions, char *table_name, int* table_type)
{
	if(cRestrictions >= 3 && V_VT(&rgRestrictions[2]) == VT_BSTR && V_BSTR(&rgRestrictions[2]) != NULL)
	{
		// TABLE_NAME restriction
		CW2A name(V_BSTR(&rgRestrictions[2]));
		ATLTRACE2("\tTable Name = %s\n", (LPSTR)name);

		strncpy(table_name, name, 255);
		table_name[255] = 0; // ensure zero-terminated string
	}

	if(cRestrictions >= 4 && V_VT(&rgRestrictions[3]) == VT_BSTR && V_BSTR(&rgRestrictions[3]) != NULL)
	{
		// TABLE_NAME restriction
		CW2A name(V_BSTR(&rgRestrictions[3]));
		ATLTRACE2("\tTable Type = %s\n", (LPSTR)name);

		*table_type = 3; // wrong restriction
		if (!strcmp(name, "TABLE"))
			*table_type = 2;
		else if (!strcmp(name, "VIEW"))
			*table_type = 1;
		else if (!strcmp(name, "SYSTEM TABLE"))
			*table_type = 0;
	}
}

static HRESULT FetchData(int hReq, CTablesInfoRow &tirData, int table_type)
{
	char *value;
	int int_value;
	int ind, res;
	T_CCI_ERROR err_buf;

	res = cci_fetch(hReq, &err_buf);
	if(res < 0)
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

	res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &value, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

	wcscpy(tirData.m_szTableName, CA2W(value));

	res = cci_get_data(hReq, 2, CCI_A_TYPE_INT, &int_value, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

	if (table_type >= 0 && table_type != int_value)
		return S_FALSE;

	if (int_value == 2)
		wcscpy(tirData.m_szTableType, L"TABLE");
	else if (int_value == 1)
		wcscpy(tirData.m_szTableType, L"VIEW");
	else if (int_value == 0)
		wcscpy(tirData.m_szTableType, L"SYSTEM TABLE");

	tirData.m_bBookmarks = ATL_VARIANT_TRUE;
	tirData.m_bBookmarkType = DBPROPVAL_BMK_NUMERIC;
	tirData.m_bBookmarkDatatype = DBTYPE_BYTES;
	tirData.m_bBookmarkMaximumLength = sizeof(DBROWCOUNT);

	return S_OK;
}

static HRESULT GetCardinality(int hConn, WCHAR* tableName, ULARGE_INTEGER* cardinality)
{
	CComBSTR query;
	T_CCI_ERROR error;
	int hReq, res, ind;

	query.Append("select count(*) from [");
	query.Append(tableName);
	query.Append("]");

	hReq = cci_prepare(hConn, CW2A(query.m_str), 0, &error);
	if (hReq < 0)
		return S_OK;

	res = cci_execute(hReq, CCI_EXEC_QUERY_ALL, 0, &error);
	if (res < 0)
		return S_OK;

	res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &error);
	if(res < 0)
		return S_OK;

	res = cci_fetch(hReq, &error);
	if(res < 0)
		return S_OK;

	res = cci_get_data(hReq, 1, CCI_A_TYPE_INT, &cardinality->QuadPart, &ind);
	if(res < 0)
		return S_OK;

	cci_close_req_handle(hReq);

	return S_OK;
}

HRESULT CSRTablesInfo::Execute(LONG* pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRTablesInfo::Execute\n");

	ClearError();

	int hConn = -1;
	HRESULT hr = CCUBRIDSession::GetSessionPtr(this)->GetConnectionHandle(&hConn);
	if(FAILED(hr))
		return hr;

	char table_name[256];
	table_name[0] = 0;
	int table_type = -1;

	GetRestrictions(cRestrictions, rgRestrictions, table_name, &table_type);
	{
		T_CCI_ERROR err_buf;
		int hReq = cci_schema_info(hConn, CCI_SCH_CLASS, (table_name[0] ? table_name : NULL), NULL, CCI_CLASS_NAME_PATTERN_MATCH, &err_buf);
		if(hReq < 0)
		{
			ATLTRACE2("cci_schema_info failed\n");
			return E_FAIL;
		}

		int res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
		if(res == CCI_ER_NO_MORE_DATA)
			goto done;
		if(res < 0)
			goto error;

		while(1)
		{
			CTablesInfoRow tirData;
			hr = FetchData(hReq, tirData, table_type);
			if(FAILED(hr))
			{
				cci_close_req_handle(hReq);
				return hr;
			}
			HRESULT hrCard = GetCardinality(hConn, tirData.m_szTableName, &tirData.m_ulCardinality);
			if(FAILED(hrCard))
			{
				cci_close_req_handle(hReq);
				return hrCard;
			};

			if(hr == S_OK)
			{
				_ATLTRY
				{
					size_t nPos;
					for( nPos = 0 ; nPos < m_rgRowData.GetCount() ; nPos++ )
					{
						int res = CompareStringW(LOCALE_USER_DEFAULT,
						NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
						m_rgRowData[nPos].m_szTableType, -1,
						tirData.m_szTableType, -1);
						if(res == CSTR_GREATER_THAN)
							break;
						if(res == CSTR_EQUAL)
						{
							res = CompareStringW(LOCALE_USER_DEFAULT,
							NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
							m_rgRowData[nPos].m_szTableName, -1,
							tirData.m_szTableName, -1);
							if(res == CSTR_GREATER_THAN)
								break;
						}
					}
					m_rgRowData.InsertAt(nPos, tirData);
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
		ATLTRACE2("fail to fetch data\n");
		cci_close_req_handle(hReq);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

done:
		cci_close_req_handle(hReq);
	}

	return S_OK;
}

DBSTATUS CSRTablesInfo::GetDBStatus(CSimpleRow *pRow, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRTablesInfo::GetDBStatus\n");
	switch(pInfo->iOrdinal)
	{
	case 3: // TABLE_NAME
	case 4: // TABLE_TYPE
	case 6: // BOOKMARKS
	case 7: // BOOKMARK_TYPE
	case 8: // BOOKMARK_DATATYPE
	case 9: // BOOKMARK_MAXIMUM_LENGTH
	case 12: // CARDINALITY
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_S_ISNULL;
	}
}
