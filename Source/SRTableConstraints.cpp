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

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions, char *table_name, int* constraint_type)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRTableConstraints::GetRestrictions\n");

	if(cRestrictions >= 6 && V_VT(&rgRestrictions[5]) == VT_BSTR && V_BSTR(&rgRestrictions[5]) != NULL)
	{
		// TABLE_NAME restriction
		CW2A name(V_BSTR(&rgRestrictions[5]));
		ATLTRACE2("\tTable Name = %s\n", (LPSTR)name);

		strncpy(table_name, name, 255);
		//strncpy : The behavior of strncpy is undefined if the source and destination strings overlap.
		table_name[255] = 0; // ensure zero-terminated string
	}

	if(cRestrictions >= 7 && V_VT(&rgRestrictions[6]) == VT_BSTR && V_BSTR(&rgRestrictions[6]) != NULL)
	{
		// CONSTRAINT_TYPE restriction
		BSTR name = V_BSTR(&rgRestrictions[6]);
		ATLTRACE("\tConstraint Type = %s\n", CW2A(name));

		*constraint_type = 2; // wrong restriction
		if(wcscmp(name, L"UNIQUE") == 0)
			*constraint_type = 0;
		else if(wcscmp(name, L"PRIMARY KEY") == 0)
			*constraint_type = 1;
	}
}

static HRESULT FetchData(int hReq, CTableConstraintsRow &tcrData, int constraint_type)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRTableConstraints::FetchData\n");

	char *value;
	int int_value;
	int ind, res;
	T_CCI_ERROR err_buf;

	res = cci_fetch(hReq, &err_buf);
	if(res < 0)
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

	res = cci_get_data(hReq, 1, CCI_A_TYPE_INT, &int_value, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

	if (constraint_type >= 0 && (constraint_type != int_value))
		return S_FALSE;

	if (int_value == 0) // 0 means the unique constraint. But 1 represents just index.
	{
		wcscpy(tcrData.m_szConstraintType, L"UNIQUE");
		int result = cci_get_data(hReq, 2, CCI_A_TYPE_STR, &value, &ind);
		if (result < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

		wcscpy(tcrData.m_szConstraintName, CA2W(value));
	}
	else
		return S_FALSE;

#if 0
	if (int_value == 0)
		wcscpy(tcrData.m_szConstraintType, L"UNIQUE");
	else if (int_value == 1)
		wcscpy(tcrData.m_szConstraintType, L"PRIMARY KEY");
	else
		wcscpy(tcrData.m_szConstraintType, L"");

	res = cci_get_data(hReq, 2, CCI_A_TYPE_STR, &value, &ind);
	if(res < 0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

	wcscpy(tcrData.m_szConstraintName, CA2W(value));
#endif

	return S_OK;
}

HRESULT CSRTableConstraints::FillRowData(int hReq, LONG* pcRowsAffected, const char *table_name, int constraint_type)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRTableConstraints::FillRowData\n");

	int res;
	T_CCI_ERROR err_buf;
	HRESULT hr = S_OK;

	res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
	if(res == CCI_ER_NO_MORE_DATA)
		return S_OK;
	if(res < 0)
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);

	while(true)
	{
		CTableConstraintsRow tcrData;
		wcscpy(tcrData.m_szTableName, CA2W(table_name));

		hr = FetchData(hReq, tcrData, constraint_type);
		if(FAILED(hr))
			return E_FAIL;

		if(hr == S_OK)
		{
			size_t nPos;
			for( nPos = 0 ; nPos < m_rgRowData.GetCount() ; nPos++ )
			{
				int res = CompareStringW(LOCALE_USER_DEFAULT,
					NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
					m_rgRowData[nPos].m_szConstraintName, -1,
					tcrData.m_szConstraintName, -1);
				if(res == CSTR_GREATER_THAN)
					break;
				if(res == CSTR_EQUAL)
				{
					res = CompareStringW(LOCALE_USER_DEFAULT,
						NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT | NORM_IGNORECASE,
						m_rgRowData[nPos].m_szTableName, -1,
						tcrData.m_szTableName, -1);
					if(res == CSTR_GREATER_THAN)
						break;
					if(res == CSTR_EQUAL)
					{
						res = CompareStringW(LOCALE_USER_DEFAULT,
							NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
							m_rgRowData[nPos].m_szConstraintType, -1,
							tcrData.m_szConstraintType, -1);
						if(res == CSTR_GREATER_THAN)
							break;
					}
				}
			}

			_ATLTRY
			{
				m_rgRowData.InsertAt(nPos, tcrData);
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
		if(res < 0)
			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);
	}

	return S_OK;
}

HRESULT CSRTableConstraints::Execute(LONG* pcRowsAffected,
																		 ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRTableConstraints::Execute\n");

	const int INVALID_CONSTRAINT_TYPE = 2;

	ClearError();

	T_CCI_ERROR err_buf;
	int hConn = -1;
	HRESULT hr = CCUBRIDSession::GetSessionPtr(this)->GetConnectionHandle(&hConn);
	if(FAILED(hr)) return hr;

	if (pcRowsAffected)
		*pcRowsAffected = 0;

	char table_name[256];
	table_name[0] = 0;
	int constraint_type = -1;

	GetRestrictions(cRestrictions, rgRestrictions, table_name, &constraint_type);

	if (pcRowsAffected)
		*pcRowsAffected = 0;

	if (constraint_type == INVALID_CONSTRAINT_TYPE)
		return S_OK;

	CAtlArray<CStringA> rgTableNames;
	if(table_name[0])
		rgTableNames.Add(table_name);
	else
		Util::GetTableNames(hConn, rgTableNames);

	for(size_t i = 0; i < rgTableNames.GetCount(); i++)
	{
		int hReq;
		hReq = cci_schema_info(hConn, CCI_SCH_CONSTRAINT, rgTableNames[i].GetBuffer(), NULL, 0, &err_buf);
		if(hReq < 0)
		{
			ATLTRACE2("CSRTableConstraints: cci_schema_info fail\n");
			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), err_buf.err_msg);
		}

		hr = FillRowData(hReq, pcRowsAffected, rgTableNames[i].GetBuffer(), constraint_type);
		if(FAILED(hr))
		{
			cci_close_req_handle(hReq);
			return E_FAIL;
		}

		cci_close_req_handle(hReq);
	}

	return S_OK;
}

DBSTATUS CSRTableConstraints::GetDBStatus(CSimpleRow *pRow, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRTableConstraints::GetDBStatus\n");

	switch(pInfo->iOrdinal)
	{
	case 3: // CONSTRAINT_NAME
	case 6: // TABLE_NAME
	case 7: // CONSTRAINT_TYPE
	case 8: // IS_DEFERRABLE
	case 9: // INITIALLY_DEFERRED
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_S_ISNULL;
	}
}
