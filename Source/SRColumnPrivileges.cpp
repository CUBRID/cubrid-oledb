////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DataSource.h"
#include "Session.h"
#include "Error.h"
#include "CUBRIDStream.h"

CSRColumnPrivileges::~CSRColumnPrivileges()
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRColumnPrivileges::~CSRColumnPrivileges\n");

	if(m_spUnkSite)
	{
		CCUBRIDSession* pSession = CCUBRIDSession::GetSessionPtr(this);
		if (pSession->m_cSessionsOpen == 1)
			pSession->RowsetCommit();
	}
}

static HRESULT GetCurrentUser(CSRColumnPrivileges *pSR, CComVariant &var)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRColumnPrivileges::GetCurrentUser\n");

	CCUBRIDDataSource *pDS = CCUBRIDSession::GetSessionPtr(pSR)->GetDataSourcePtr();
	pDS->GetPropValue(&DBPROPSET_DBINIT, DBPROP_AUTH_USERID, &var);

	return S_OK;
}

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions, char *table_name, char* column_name)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRColumnPrivileges::GetRestrictions\n");

	if(cRestrictions >= 3 && V_VT(&rgRestrictions[2]) == VT_BSTR && V_BSTR(&rgRestrictions[2]) != NULL)
	{
		// TABLE_NAME restriction
		CW2A name(V_BSTR(&rgRestrictions[2]));
		ATLTRACE2("\tTable name = %s\n", (LPSTR)name);

		strncpy(table_name, name, 255);
		table_name[255] = '\0'; // ensure zero-terminated string
	}

	if(cRestrictions >= 4 && V_VT(&rgRestrictions[3]) == VT_BSTR && V_BSTR(&rgRestrictions[3]) != NULL)
	{
		// COLUMN_NAME restriction
		CW2A name(V_BSTR(&rgRestrictions[3]));
		ATLTRACE2("\tColumn Name = %s\n", (LPSTR)name);

		strncpy(column_name, name, 255);
		column_name[1023] = '\0'; // ensure zero-terminated string
	}
}

static HRESULT FetchData(int hReq, CColumnPrivilegesRow &cprData)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRColumnPrivileges::FetchData\n");

	char *value;
	int ind, res;
	T_CCI_ERROR err_buf;

	res = cci_fetch(hReq, &err_buf);
	if (res == CCI_ER_NO_MORE_DATA)
		return S_FALSE;
	if(res < 0)
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

	res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &value, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	wcscpy(cprData.m_szColumnName, CA2W(value));

	res = cci_get_data(hReq, 2, CCI_A_TYPE_STR, &value, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

	if(strcmp(value, "SELECT") != 0 && strcmp(value, "DELETE") != 0 && strcmp(value, "INSERT") != 0
		&& strcmp(value, "UPDATE") != 0 && strcmp(value, "REFERENCES") != 0)
		return S_FALSE;

	wcscpy(cprData.m_szPrivilegeType, CA2W(value));

	res = cci_get_data(hReq, 3, CCI_A_TYPE_STR, &value, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

	if(strcmp(value, "NO") == 0)
		cprData.m_bIsGrantable = VARIANT_FALSE;
	else
		cprData.m_bIsGrantable = VARIANT_TRUE;

	return S_OK;
}

HRESULT CSRColumnPrivileges::FillRowData(int hReq, LONG *pcRowsAffected,
																				 const CComVariant &grantee, const char *table_name, const char *column_name)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRColumnPrivileges::FillRowData\n");

	int res;
	T_CCI_ERROR err_buf;
	HRESULT hr = S_OK;

	res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
	if(res == CCI_ER_NO_MORE_DATA)
		return S_OK;
	else if(res < 0)
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

	while(true)
	{
		CColumnPrivilegesRow cprData;
		wcscpy(cprData.m_szGrantor, L"DBA"); //TODO Investigate grantor value
		wcscpy(cprData.m_szGrantee, V_BSTR(&grantee));
		wcscpy(cprData.m_szTableName, CA2W(table_name));

		hr = FetchData(hReq, cprData);
		if(FAILED(hr)) return E_FAIL;

		if(hr == S_OK)
		{
			size_t nPos;
			for( nPos = 0 ; nPos < m_rgRowData.GetCount() ; nPos++ )
			{
				int res = CompareStringW(LOCALE_USER_DEFAULT,
					NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
					m_rgRowData[nPos].m_szTableName, -1,
					cprData.m_szTableName, -1);

				if(res == CSTR_GREATER_THAN)
					break;
				if(res == CSTR_EQUAL)
				{
					res = CompareStringW(LOCALE_USER_DEFAULT,
						NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
						m_rgRowData[nPos].m_szColumnName, -1,
						cprData.m_szColumnName, -1);
					if(res == CSTR_GREATER_THAN)
						break;
					if(res == CSTR_EQUAL)
					{
						res = CompareStringW(LOCALE_USER_DEFAULT,
							NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
							m_rgRowData[nPos].m_szPrivilegeType, -1,
							cprData.m_szPrivilegeType, -1);
						if(res == CSTR_GREATER_THAN)
							break;
					}
				}
			}

			_ATLTRY
			{
				m_rgRowData.InsertAt(nPos, cprData);
			}
			_ATLCATCHALL()
			{
				return E_OUTOFMEMORY;
			}
			(*pcRowsAffected)++;
		}

		res = cci_cursor(hReq, 1, CCI_CURSOR_CURRENT, &err_buf);
		if(res == CCI_ER_NO_MORE_DATA)
			return S_OK;
		else if(res < 0)
			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);
	}

	return S_OK;
}

HRESULT CSRColumnPrivileges::Execute(LONG* pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRColumnPrivileges::Execute\n");

	ClearError();

	int hConn;
	HRESULT hr = CCUBRIDSession::GetSessionPtr(this)->GetConnectionHandle(&hConn);
	if(FAILED(hr))
		return hr;

	if (pcRowsAffected)
		*pcRowsAffected = 0;

	char table_name[256];
	memset(table_name, '\0', sizeof(table_name));
	char column_name[256];
	memset(column_name, '\0', sizeof(column_name));

	GetRestrictions(cRestrictions, rgRestrictions, table_name, column_name);

	CComVariant grantee;
	hr = GetCurrentUser(this, grantee);
	if(FAILED(hr))
		return hr;

	CAtlArray<CStringA> rgTableNames;
	if(table_name[0])
		rgTableNames.Add(table_name);
	else
		Util::GetTableNames(hConn, rgTableNames);

	for(size_t i = 0; i < rgTableNames.GetCount(); i++)
	{
		T_CCI_ERROR err_buf;
		int hReq;
		hReq = cci_schema_info(hConn, CCI_SCH_ATTR_PRIVILEGE, rgTableNames[i].GetBuffer(),
			(column_name[0] ? column_name : NULL),
			CCI_ATTR_NAME_PATTERN_MATCH, &err_buf);

		if(hReq < 0)
		{
			ATLTRACE2("CSRColumnPrivileges: cci_schema_info fail\n");

			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);
		}

		hr = FillRowData(hReq, pcRowsAffected, grantee, rgTableNames[i].GetBuffer(), column_name);
		if(FAILED(hr))
		{
			cci_close_req_handle(hReq);
			return E_FAIL;
		}

		cci_close_req_handle(hReq);
	}

	return S_OK;
}

DBSTATUS CSRColumnPrivileges::GetDBStatus(CSimpleRow *pRow, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRColumnPrivileges::GetDBStatus\n");

	switch(pInfo->iOrdinal)
	{
	case 1: // GRANTOR
	case 2: // GRANTEE
	case 5: // TABLE_NAME
	case 6: // COLUMN_NAME
	case 9: // PRIVILEGE_TYPE
	case 10: // IS_GRANTABLE
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_S_ISNULL;
	}
}
