////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Session.h"
#include "Error.h"
#include "SRForeignKeys.h"
#include "CUBRIDStream.h"

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions, char *table_name)
{
	if (cRestrictions >= 6 && V_VT(&rgRestrictions[5]) == VT_BSTR && V_BSTR(&rgRestrictions[5]) != NULL)
	{
		CW2A name(V_BSTR(&rgRestrictions[5]));

		ATLTRACE2("\tTable Name = %s\n", (LPSTR)name);

		strncpy(table_name, name, 127);
		table_name[127] = '\0'; // ensure zero-terminated string
	}
}

CForeignKeysRow::CForeignKeysRow()
{
	memset(this, 0, sizeof(*this));		
}

const wchar_t *CForeignKeysRow::GetTableNameColumn(void)
{
	return const_cast<wchar_t *>(this->FKTableName);
}

void CForeignKeysRow::SetPKTableNameColumn(const wchar_t *newTableName)
{
	wcscpy(this->PKTableName, newTableName);
}

void CForeignKeysRow::SetFKTableNameColumn(const wchar_t *newTableName)
{
	wcscpy(this->FKTableName, newTableName);
}

void CForeignKeysRow::SetPKColumnNameColumn(const wchar_t *columnName)
{
	wcscpy(this->PKColumnName, columnName);
}

void CForeignKeysRow::SetFKColumnNameColumn(const wchar_t *columnName)
{
	wcscpy(this->FKColumnName, columnName);
}

void CForeignKeysRow::SetUpdateRuleColumn(const wchar_t *rule)
{
	wcscpy(this->UpdateRule, rule);
	//_wcsupr(this->UpdateRule);
}

void CForeignKeysRow::SetDeleteRuleColumn(const wchar_t *rule)
{
	wcscpy(this->DeleteRule, rule);
	//_wcsupr(this->DeleteRule);
}

void CForeignKeysRow::SetKeySeqColumn(const int &seq)
{
	this->KeySeq = seq;
}

void CForeignKeysRow::SetColumnOrdinalColumn(const int &ordinal)
{
	this->ColumnOrdinal = ordinal;
}

void CForeignKeysRow::SetForeignKeyNameColumn(const wchar_t *foreignKeyName)
{
	wcscpy(this->foreignKeyName, foreignKeyName);
}

void CForeignKeysRow::SetPrimaryKeyNameColumn(const wchar_t *primaryKeyName)
{
	wcscpy(this->primaryKeyName, primaryKeyName);
}

CSRForeignKeys::~CSRForeignKeys()
{
	if (m_spUnkSite)
	{
		CCUBRIDSession* pSession = CCUBRIDSession::GetSessionPtr(this);
		if (pSession->m_cSessionsOpen == 1)
			pSession->RowsetCommit();
	}
}

HRESULT CSRForeignKeys::Execute(LONG *pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRForeignKeys::Execute()\n");

	int connectionHandle = -999;
	int cci_return_code = 0;
	int cci_request_handle = 0;
	char tableName[128] = { '\0', };

	HRESULT hr = E_FAIL;

	T_CCI_ERROR cci_error_buffer;

	ClearCurrentErrorObject();

	hr = GetConnectionHandleFromSessionObject(connectionHandle);

	if (FAILED(hr))
		return hr;
	GetRestrictions(cRestrictions, rgRestrictions, tableName);
#if 1
	cci_return_code = cci_schema_info(connectionHandle, CCI_SCH_IMPORTED_KEYS, (tableName[0] ? tableName : NULL),
		NULL,							// no attribute name is specified.
		CCI_CLASS_NAME_PATTERN_MATCH, &cci_error_buffer);
#endif
	if (cci_return_code < 0)
	{
		ATLTRACE2(atlTraceDBProvider, 2, "CSRForeignKeys::cci_schema_info() FAILED! \n");

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}

	cci_request_handle = cci_return_code;

	cci_return_code = cci_cursor(cci_request_handle, 1, CCI_CURSOR_FIRST, &cci_error_buffer);
	if (cci_return_code < 0)
	{
		if (cci_return_code == CCI_ER_NO_MORE_DATA)
		{
			cci_close_req_handle(cci_request_handle);

			return S_OK;
		}
		else
		{
			cci_close_req_handle(cci_request_handle);

			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
		}
	}

	while (1)
	{
		CForeignKeysRow newRow;

		hr = FetchData(cci_request_handle, newRow);
		if (FAILED(hr))
			return hr;

		if (hr == S_OK)
		{
			_ATLTRY
			{
				size_t position;
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

DBSTATUS CSRForeignKeys::GetDBStatus(CSimpleRow *pRow, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRForeignKeys::GetDBStatus()\n");

	switch(pInfo->iOrdinal)
	{
	case 1:
	case 2:
	case 5:
	case 6:
	case 7:
	case 8:
	case 11:
	case 12:
	case 13:
		return DBSTATUS_S_ISNULL;
	case 3:
	case 4:
	case 9:
	case 10:
	case 14:
	case 15:
	case 16:
	case 17:
	case 18:
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_E_UNAVAILABLE;
	}
}

void CSRForeignKeys::ClearCurrentErrorObject(void)
{
	ClearError();
}

HRESULT CSRForeignKeys::GetConnectionHandleFromSessionObject(int &connectionHandle)
{
	return CCUBRIDSession::GetSessionPtr(this)->GetConnectionHandle(&connectionHandle);
}

HRESULT CSRForeignKeys::FetchData(int cci_request_handle, CForeignKeysRow &newRow)
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

	newRow.SetPKTableNameColumn(CA2W(value));

	cci_return_code = cci_get_data(cci_request_handle, 2, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}

	newRow.SetPKColumnNameColumn(CA2W(value));

	cci_return_code = cci_get_data(cci_request_handle, 3, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}
	newRow.SetFKTableNameColumn(CA2W(value));

	cci_return_code = cci_get_data(cci_request_handle, 4, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}
	newRow.SetFKColumnNameColumn(CA2W(value));

	cci_return_code = cci_get_data(cci_request_handle, 5, CCI_A_TYPE_INT, &ordinal, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}
	newRow.SetKeySeqColumn(ordinal); //KEY SEQ

	cci_return_code = cci_get_data(cci_request_handle, 6, CCI_A_TYPE_INT, &ordinal, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}
	switch (ordinal)
	{
	case 1:
		newRow.SetUpdateRuleColumn(CA2W("restrict"));
		break;
	case 2:
		newRow.SetUpdateRuleColumn(CA2W("no action"));
		break;
	case 3:
		newRow.SetUpdateRuleColumn(CA2W("set null"));
		break;
	default:
		newRow.SetUpdateRuleColumn(CA2W("no rule"));
	}

	cci_return_code = cci_get_data(cci_request_handle, 7, CCI_A_TYPE_INT, &ordinal, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}

	switch (ordinal)
	{
	case 0:
		newRow.SetDeleteRuleColumn(CA2W("cascade"));
		break;
	case 1:
		newRow.SetDeleteRuleColumn(CA2W("restrict"));
		break;
	case 2:
		newRow.SetDeleteRuleColumn(CA2W("no action"));
		break;
	case 3:
		newRow.SetDeleteRuleColumn(CA2W("set null"));
		break;
	default:
		newRow.SetDeleteRuleColumn(CA2W("no rule"));
	}

	cci_return_code = cci_get_data(cci_request_handle, 8, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}
	newRow.SetForeignKeyNameColumn(CA2W(value));

	cci_return_code = cci_get_data(cci_request_handle, 9, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);

		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), cci_error_buffer.err_msg);
	}
	newRow.SetPrimaryKeyNameColumn(CA2W(value));

	return S_OK;
}