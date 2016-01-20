/*
 * Copyright (C) 2008 Search Solution Corporation. All rights reserved by Search Solution. 
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met: 
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer. 
 *
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution. 
 *
 * - Neither the name of the <ORGANIZATION> nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software without 
 *   specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE. 
 *
 */

#include "stdafx.h"
#include "Session.h"
#include "Error.h"

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions, PWSTR tableName)
{
	//if (cRestrictions >= 3 && V_VT(&rgRestrictions[2]) == VT_BSTR && V_BSTR(&rgRestrictions[2]) != NULL)
	//{
	//	CW2A constraint(V_BSTR(&rgRestrictions[2]));
	//	ATLTRACE2("\tConstraint Name = %s\n", (LPSTR)constraint);

	//	strncpy(constraint, constraintName, 127);
	//	constraint[127] = '\0';
	//}

	if (cRestrictions >= 5 && V_VT(&rgRestrictions[5]) == VT_BSTR && V_BSTR(&rgRestrictions[5]) != NULL)
	{	
		ATLTRACE2(L"\tTable Name = %s\n", V_BSTR(&rgRestrictions[5]));
		wcsncpy(tableName, V_BSTR(&rgRestrictions[5]), 127);
		tableName[127] = '\0'; // ensure zero-terminated string
	}

	//if (cRestrictions >= 6 && V_VT(&rgRestrictions[5]) == VT_BSTR && V_BSTR(&rgRestrictions[5]) != NULL)
	//{
	//	CW2A column_name(V_BSTR(&rgRestrictions[5]));
	//	ATLTRACE2("\tColumn Name = %s\n", (LPSTR)column_name);

	//}
}

CKeyColumnUsageRow::CKeyColumnUsageRow()
{
	this->columnName[0] = '\0';
	this->columnPropID = 0;
	this->constraintCatalog[0] = '\0';
	this->constraintName[0] = '\0';
	this->constraintSchema[0]= '\0';
	this->ordinalPosition = 0;
	this->tableCatalog[0] = '\0';
	this->tableSchema[0] = '\0';
	this->tableName[0]= '\0';
}

const wchar_t *CKeyColumnUsageRow::GetColumnName(void)
{
	return this->columnName;
}

const wchar_t *CKeyColumnUsageRow::GetTableName(void)
{
	return const_cast<wchar_t *>(this->tableName);
}

const wchar_t *CKeyColumnUsageRow::GetConstraintName(void)
{
	return this->constraintName;
}

void CKeyColumnUsageRow::SetColumnName(const wchar_t *columnName)
{
	wcscpy(this->columnName, columnName);
	_wcsupr(this->columnName);
}

void CKeyColumnUsageRow::SetTableName(const wchar_t *tableName)
{
	wcscpy(this->tableName, tableName);
	_wcsupr(this->tableName);
}

void CKeyColumnUsageRow::SetConstraintName(const wchar_t *constraintName)
{
	wcscpy(this->constraintName, constraintName);
	//_wcsupr(this->constraintName);
}

void CKeyColumnUsageRow::SetOrdinalPosition(const int &ordinal)
{
	this->ordinalPosition = ordinal;
}

const int CKeyColumnUsageRow::GetOrdinalPosition(void)
{
	return this->ordinalPosition;
}
CSRKeyColumnUsage::~CSRKeyColumnUsage()
{
	if (m_spUnkSite)
	{
		CCUBRIDSession* pSession = CCUBRIDSession::GetSessionPtr(this);
		if (pSession->m_cSessionsOpen == 1)	
			pSession->RowsetCommit();
	}
}

DBSTATUS CSRKeyColumnUsage::GetDBStatus(CSimpleRow *, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRKeyColumnUsage::GetDBStatus()\n");

	switch(pInfo->iOrdinal) 
	{
	case 1:
	case 2:
	case 4:
	case 5:
	case 8:
	case 9:
		return DBSTATUS_S_ISNULL;
	case 3:
	case 6:
	case 7:
	case 10:
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_E_UNAVAILABLE;
	}
}

HRESULT CSRKeyColumnUsage::Execute(LONG *pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRKeyColumnUsage::Execute()\n");
	////
	int cci_connection_handle = -999;

	WCHAR tableName[128] = {'\0', };

	HRESULT hr = E_FAIL;
	////

	cci_connection_handle = CCUBRIDSession::GetSessionPtr(this)->GetConnection();
	UINT uCodepage = CCUBRIDSession::GetSessionPtr(this)->GetCodepage();
	GetRestrictions(cRestrictions, rgRestrictions, tableName);

	////
	int cci_return_code = -999;
	int cci_request_handle = 0;
	T_CCI_ERROR cci_error_buffer;
	////
#if 1
	CW2A _tableName(tableName, uCodepage);
	cci_return_code = cci_schema_info
		(
		cci_connection_handle, 
		CCI_SCH_PRIMARY_KEY, 
		(tableName[0]? (PSTR) _tableName : NULL), 
		NULL,							// no attribute name is specified.
		CCI_CLASS_NAME_PATTERN_MATCH, 
		&cci_error_buffer
		);
#endif
	if (cci_return_code < 0)
	{
		ATLTRACE2(atlTraceDBProvider, 2, "CSRPrimaryKeys::cci_schema_info() FAILED! \n");
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(cci_error_buffer.err_msg, uCodepage));
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
			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(cci_error_buffer.err_msg, uCodepage));
		}
	}

	while (1)
	{
		CKeyColumnUsageRow row;

		hr = FetchData(cci_request_handle, uCodepage, row);
		if (FAILED(hr))
			return hr;

		if (hr == S_OK)
		{
			_ATLTRY
			{
				size_t position = 0;
				for (position = 0; position < m_rgRowData.GetCount() ; position++)
				{
					int result = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT, m_rgRowData[position].GetConstraintName(), -1, row.GetConstraintName(), -1);
					if ( result == CSTR_GREATER_THAN ) 
					{
							break;
					}
					else if (result == CSTR_EQUAL)
					{
						result = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT, m_rgRowData[position].GetTableName(), -1, row.GetTableName(), -1);
						if (result == CSTR_GREATER_THAN)
							break;
						else if (result == CSTR_EQUAL)
						{
							if (m_rgRowData[position].GetOrdinalPosition() >= row.GetOrdinalPosition())
								break;
						}
					}
				}
				m_rgRowData.InsertAt(position, row);
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
				cci_close_req_handle(cci_request_handle);
				return S_OK;
			}
			else
			{
				cci_close_req_handle(cci_request_handle);
				return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(cci_error_buffer.err_msg, uCodepage));
			}
		}
	}
	return S_OK;
}

HRESULT CSRKeyColumnUsage::FetchData(int cci_request_handle, UINT uCodepage, CKeyColumnUsageRow &newRow)
{
	////
	int cci_return_code = 0;
	int cci_indicator = 0;

	int isPrimaryKey = -1;

	T_CCI_ERROR cci_error_buffer;
	////

	cci_return_code = cci_fetch(cci_request_handle, &cci_error_buffer);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(cci_error_buffer.err_msg, uCodepage));
	}

	////
	char *value = NULL;
	////

	cci_return_code = cci_get_data(cci_request_handle, 1, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(cci_error_buffer.err_msg, uCodepage));
	}
	newRow.SetTableName(CA2W(value, uCodepage));

	cci_return_code = cci_get_data(cci_request_handle, 2, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(cci_error_buffer.err_msg, uCodepage));
	}
	newRow.SetColumnName(CA2W(value, uCodepage));

	////
	int ordinal = 0;
	////

	cci_return_code = cci_get_data(cci_request_handle, 3, CCI_A_TYPE_INT, &ordinal, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(cci_error_buffer.err_msg, uCodepage));
	}
	newRow.SetOrdinalPosition(ordinal);

	cci_return_code = cci_get_data(cci_request_handle, 4, CCI_A_TYPE_STR, &value, &cci_indicator);
	if (cci_return_code < 0)
	{
		cci_close_req_handle(cci_request_handle);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(cci_error_buffer.err_msg, uCodepage));
	}
	newRow.SetConstraintName(CA2W(value, uCodepage));

	return S_OK;
}
