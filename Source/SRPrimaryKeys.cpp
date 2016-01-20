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

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions, char *table_name)
{
	if (cRestrictions >= 3 && V_VT(&rgRestrictions[2]) == VT_BSTR && V_BSTR(&rgRestrictions[2]) != NULL)
	{
		CW2A name(V_BSTR(&rgRestrictions[2]));

		ATLTRACE2("\tTable Name = %s\n", (LPSTR)name);

		strncpy(table_name, name, 127);
		table_name[127] = '\0'; // ensure zero-terminated string
	}
}

CPrimaryKeysRow::CPrimaryKeysRow()
{
	memset(this, 0, sizeof(*this));		
}

const wchar_t *CPrimaryKeysRow::GetTableNameColumn(void)
{
	return const_cast<wchar_t *>(this->tableName);
}

void CPrimaryKeysRow::SetTableNameColumn(const wchar_t *newTableName)
{
	wcscpy(this->tableName, newTableName);
	//_wcsupr(this->tableName);
}

void CPrimaryKeysRow::SetColumnNameColumn(const wchar_t *columnName)
{
	wcscpy(this->columnName, columnName);
	//_wcsupr(this->columnName);
}

void CPrimaryKeysRow::SetColumnOrdinalColumn(const int &ordinal)
{
	this->columnOrdinal = ordinal;
}

void CPrimaryKeysRow::SetPrimaryKeyNameColumn(const wchar_t *primaryKeyName)
{
	wcscpy(this->primaryKeyName, primaryKeyName);
}

CSRPrimaryKeys::~CSRPrimaryKeys()
{
	if (m_spUnkSite)
	{
		CCUBRIDSession* pSession = CCUBRIDSession::GetSessionPtr(this);
		if (pSession->m_cSessionsOpen == 1)
			pSession->RowsetCommit();
	}
}

HRESULT CSRPrimaryKeys::Execute(LONG *pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRPrimaryKeys::Execute()\n");

	int connectionHandle = -999;
	int cci_return_code = 0;
	int cci_request_handle = 0;
	char tableName[128] = { '\0', };

	HRESULT hr = E_FAIL;

	T_CCI_ERROR cci_error_buffer;

	ClearCurrentErrorObject();

	hr = GetConnectionHandleFromSessionObject(connectionHandle);
	//hr = CCUBRIDSession::GetSessionPtr(this)->GetConnectionHandle(&connectionHandle);
	if (FAILED(hr))
		return hr;
	GetRestrictions(cRestrictions, rgRestrictions, tableName);

#if 1
	cci_return_code = cci_schema_info(connectionHandle, CCI_SCH_PRIMARY_KEY, (tableName[0] ? tableName : NULL),
	                                  NULL,							// no attribute name is specified.
	                                  CCI_CLASS_NAME_PATTERN_MATCH, &cci_error_buffer);
#endif

	if (cci_return_code < 0)
	{
		ATLTRACE2(atlTraceDBProvider, 2, "CSRPrimaryKeys::cci_schema_info() FAILED! \n");
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}

	cci_request_handle = cci_return_code;

	cci_return_code = cci_cursor(cci_request_handle, 1, CCI_CURSOR_FIRST, &cci_error_buffer);
	if (cci_return_code < 0)
	{
		if (cci_return_code == CCI_ER_NO_MORE_DATA)
		{
			// it means that there is no primary key.
			cci_close_req_handle(cci_request_handle);
			return S_OK;
		}
		else
		{
			cci_close_req_handle(cci_request_handle);
			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
		}
	}

	while (true)
	{
		//CPrimaryKeysRow *newRow = static_cast<CPrimaryKeysRow *>(CoTaskMemAlloc(sizeof(CPrimaryKeysRow)));
		//if ( newRow == NULL )
		//	return E_OUTOFMEMORY;
		CPrimaryKeysRow newRow;

		hr = FetchData(cci_request_handle, newRow);
		if (FAILED(hr))
			return hr;

		if (hr == S_OK)
		{
			_ATLTRY
			{
				size_t position = 0;
				for (position = 0; position < m_rgRowData.GetCount() ; position++)
				{
					int result = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT, m_rgRowData[position].GetTableNameColumn(), -1, newRow.GetTableNameColumn(), -1);
					if ( result == CSTR_GREATER_THAN )
						break;
				}

				m_rgRowData.InsertAt(position, newRow);
			}
			_ATLCATCHALL()
			{
				cci_close_req_handle(cci_request_handle);
				return E_OUTOFMEMORY;
			}
		}

		cci_return_code = cci_cursor(cci_request_handle, 1, CCI_CURSOR_CURRENT, &cci_error_buffer);
		if (cci_return_code < 0)
		{
			if (cci_return_code == CCI_ER_NO_MORE_DATA)
			{
				//CoTaskMemFree(newRow);
				cci_close_req_handle(cci_request_handle);
				return S_OK;
			}
			else
			{
				//CoTaskMemFree(newRow);
				cci_close_req_handle(cci_request_handle);
				return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
			}
		}
	}

	return S_OK;
}

DBSTATUS CSRPrimaryKeys::GetDBStatus(CSimpleRow *pRow, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRPrimaryKeys::GetDBStatus()\n");

	switch(pInfo->iOrdinal)
	{
	case 1:
	case 2:
	case 5:
	case 6:
		return DBSTATUS_S_ISNULL;
	case 3:
	case 4:
	case 7:
	case 8:
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_E_UNAVAILABLE;
	}
}

void CSRPrimaryKeys::ClearCurrentErrorObject(void)
{
	ClearError();
}

HRESULT CSRPrimaryKeys::GetConnectionHandleFromSessionObject(int &connectionHandle)
{
	return CCUBRIDSession::GetSessionPtr(this)->GetConnectionHandle(&connectionHandle);
}

HRESULT CSRPrimaryKeys::FetchData(int cci_request_handle, CPrimaryKeysRow &newRow)
{
	int ordinal = 0;
	int cci_return_code = -999;
	int cci_indicator = -999;
	char *value = NULL;
	T_CCI_ERROR cci_error_buffer;

	cci_return_code = cci_fetch(cci_request_handle, &cci_error_buffer);
	if ( cci_return_code < 0 )
	{
		cci_close_req_handle(cci_request_handle);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}

	cci_return_code = cci_get_data(cci_request_handle, 1, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}

	newRow.SetTableNameColumn(CA2W(value));

	cci_return_code = cci_get_data(cci_request_handle, 2, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}

	newRow.SetColumnNameColumn(CA2W(value));

	cci_return_code = cci_get_data(cci_request_handle, 3, CCI_A_TYPE_INT, &ordinal, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}
	newRow.SetColumnOrdinalColumn(ordinal);

	cci_return_code = cci_get_data(cci_request_handle, 4, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}
	newRow.SetPrimaryKeyNameColumn(CA2W(value));

	return S_OK;
}
