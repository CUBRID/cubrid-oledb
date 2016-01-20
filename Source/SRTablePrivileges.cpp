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

static HRESULT GetCurrentUser(CSRTablePrivileges *pSR, CComVariant &var)
{
	CCUBRIDDataSource *pDS = CCUBRIDSession::GetSessionPtr(pSR)->GetDataSourcePtr();
	pDS->GetPropValue(&DBPROPSET_DBINIT, DBPROP_AUTH_USERID, &var);

	return S_OK;
}

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions, char *table_name)
{
	if(cRestrictions >= 3 && V_VT(&rgRestrictions[2]) == VT_BSTR && V_BSTR(&rgRestrictions[2]) != NULL)
	{
		// TABLE_NAME restriction
		CW2A name(V_BSTR(&rgRestrictions[2]));

		ATLTRACE2("\tTable Name = %s\n", (LPSTR)name);

		strncpy(table_name, name, 1023);
		table_name[1023] = 0; // ensure zero-terminated string
	}
}

static HRESULT FetchData(int hReq, CTablePrivilegesRow &tprData)
{
	char *value;
	int ind, res;
	T_CCI_ERROR err_buf;

	res = cci_fetch(hReq, &err_buf);
	if(res < 0)
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

	res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &value, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	wcsncpy(tprData.m_szTableName, CA2W(value), 128);
	tprData.m_szTableName[128] = 0;

	res = cci_get_data(hReq, 2, CCI_A_TYPE_STR, &value, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	if(strcmp(value, "SELECT") != 0 && strcmp(value, "DELETE") != 0 && strcmp(value, "INSERT") != 0
	    && strcmp(value, "UPDATE") != 0 && strcmp(value, "REFERENCES") != 0)
		return S_FALSE;

	wcscpy(tprData.m_szPrivilegeType, CA2W(value));

	res = cci_get_data(hReq, 3, CCI_A_TYPE_STR, &value, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

	if(strcmp(value, "NO") == 0)
		tprData.m_bIsGrantable = VARIANT_FALSE;
	else
		tprData.m_bIsGrantable = VARIANT_TRUE;

	return S_OK;
}

HRESULT CSRTablePrivileges::Execute(LONG * /*pcRowsAffected*/, ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRTablePrivileges::Execute\n");

	ClearError();

	int hConn = -1;
	HRESULT hr = CCUBRIDSession::GetSessionPtr(this)->GetConnectionHandle(&hConn);
	if(FAILED(hr))
		return hr;

	char table_name[1024];
	table_name[0] = 0;
	GetRestrictions(cRestrictions, rgRestrictions, table_name);

	CComVariant grantee;
	hr = GetCurrentUser(this, grantee);
	if(FAILED(hr))
		return hr;

	{
		T_CCI_ERROR err_buf;
		int hReq = cci_schema_info(hConn, CCI_SCH_CLASS_PRIVILEGE, (table_name[0] ? table_name : NULL),
		                           NULL, CCI_CLASS_NAME_PATTERN_MATCH, &err_buf);
		if(hReq < 0)
		{
			ATLTRACE2("cci_schema_info fail\n");
			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);
		}

		int res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
		if(res == CCI_ER_NO_MORE_DATA)
			goto done;
		if(res < 0)
			goto error;

		while(1)
		{
			CTablePrivilegesRow tprData;
			wcscpy(tprData.m_szGrantor, L"DBA"); //TODO Verify Grantor
			wcscpy(tprData.m_szGrantee, V_BSTR(&grantee));
			hr = FetchData(hReq, tprData);
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
						int res = wcscmp(m_rgRowData[nPos].m_szTableName, tprData.m_szTableName);
						if(res > 0)
							break;
						if(res == 0 && wcscmp(m_rgRowData[nPos].m_szPrivilegeType, tprData.m_szPrivilegeType) > 0)
							break;
					}
					m_rgRowData.InsertAt(nPos, tprData);
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

DBSTATUS CSRTablePrivileges::GetDBStatus(CSimpleRow *pRow, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRTablePrivileges::GetDBStatus\n");
	switch(pInfo->iOrdinal)
	{
	case 1: // GRANTOR
	case 2: // GRANTEE
	case 5: // TABLE_NAME
	case 6: // PRIVILEGE_TYPE
	case 7: // IS_GRANTABLE
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_S_ISNULL;
	}
}
