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
#include "DataSource.h"
#include "Session.h"
#include "Error.h"

CSRColumnPrivileges::~CSRColumnPrivileges()
{
	if(m_spUnkSite)
	{
		CCUBRIDSession* pSession = CCUBRIDSession::GetSessionPtr(this);
		if (pSession->m_cSessionsOpen == 1)	pSession->RowsetCommit();
	}
}

static HRESULT GetCurrentUser(CSRColumnPrivileges *pSR, CComVariant &var)
{
	CCUBRIDDataSource *pDS = CCUBRIDSession::GetSessionPtr(pSR)->GetDataSourcePtr();
	pDS->GetPropValue(&DBPROPSET_DBINIT, DBPROP_AUTH_USERID, &var);
	return S_OK;
}

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions, PWSTR table_name, PWSTR column_name)
{
	// restriction�� ���ٰ� �׻� cRestrictions==0�� �ƴϴ�.
	// ���� vt!=VT_EMPTY���� � �˻������ �Ѵ�.

	if(cRestrictions>=3 && V_VT(&rgRestrictions[2])==VT_BSTR && V_BSTR(&rgRestrictions[2])!=NULL)
	{	// TABLE_NAME restriction
		wcsncpy(table_name, V_BSTR(&rgRestrictions[2]), 255);
		ATLTRACE2(L"\tTable Name = %s\n", V_BSTR(&rgRestrictions[2]));
		table_name[255] = 0; // ensure zero-terminated string
	}

	if(cRestrictions>=4 && V_VT(&rgRestrictions[3])==VT_BSTR && V_BSTR(&rgRestrictions[3])!=NULL)
	{	// COLUMN_NAME restriction
		wcsncpy(column_name, V_BSTR(&rgRestrictions[3]), 255);
		ATLTRACE2(L"\tColumn Name = %s\n", V_BSTR(&rgRestrictions[3]));
		table_name[255] = 0; // ensure zero-terminated string
	}
}

// S_OK : ����
// S_FALSE : �����ʹ� ���������� Consumer�� ���ϴ� �����Ͱ� �ƴ�
// E_FAIL : ����
static HRESULT FetchData(int hReq, UINT uCodepage, CColumnPrivilegesRow &cprData)
{
	char *value;
	int ind, res;
	T_CCI_ERROR err_buf;

	res = cci_fetch(hReq, &err_buf);
	if (res==CCI_ER_NO_MORE_DATA) return S_FALSE;
	if(res<0) return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(err_buf.err_msg, uCodepage));

	res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &value, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	wcscpy(cprData.m_szColumnName, CA2W(value, uCodepage));

	res = cci_get_data(hReq, 2, CCI_A_TYPE_STR, &value, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	// CCI�� "ALTER", "EXECUTE"� ��ȯ�ϴµ� ��� ó���ؾ� ���� �𸣰ڴ�.
	if(strcmp(value, "SELECT")!=0 && strcmp(value, "DELETE")!=0 && strcmp(value, "INSERT")!=0
	   && strcmp(value, "UPDATE")!=0 && strcmp(value, "REFERENCES")!=0)
		return S_FALSE;
	wcscpy(cprData.m_szPrivilegeType, CA2W(value, uCodepage));

	res = cci_get_data(hReq, 3, CCI_A_TYPE_STR, &value, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	if(strcmp(value, "NO")==0)
		cprData.m_bIsGrantable = VARIANT_FALSE;
	else
		cprData.m_bIsGrantable = VARIANT_TRUE;

	return S_OK;
}

// S_OK : ����
HRESULT CSRColumnPrivileges::FillRowData(int hReq, UINT uCodepage, LONG *pcRowsAffected,
			const CComVariant &grantee, const char *table_name, const char *column_name)
{
	int res;
	T_CCI_ERROR err_buf;
	HRESULT hr = S_OK;

	res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
    if(res==CCI_ER_NO_MORE_DATA) return S_OK;
	else if(res<0) return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(err_buf.err_msg, uCodepage));

	while(1)
	{
		CColumnPrivilegesRow cprData;
		wcscpy(cprData.m_szGrantor, L"DBA");
		wcscpy(cprData.m_szGrantee, V_BSTR(&grantee));
		wcscpy(cprData.m_szTableName, CA2W(table_name, uCodepage));

		hr = FetchData(hReq, uCodepage, cprData);
		if(FAILED(hr)) return E_FAIL;

		if(hr==S_OK) // S_FALSE�� �߰����� ����
		{
			// TABLE_NAME, COLUMN_NAME, PRIVILEGE_TYPE ������ �����ؾ� �Ѵ�.
			size_t nPos;
			for( nPos=0 ; nPos<m_rgRowData.GetCount() ; nPos++ )
			{
				int res = CompareStringW(LOCALE_USER_DEFAULT, 
						NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
						m_rgRowData[nPos].m_szTableName, -1,
						cprData.m_szTableName, -1);
				if(res==CSTR_GREATER_THAN) break;
				if(res==CSTR_EQUAL)
				{
					res = CompareStringW(LOCALE_USER_DEFAULT, 
							NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
							m_rgRowData[nPos].m_szColumnName, -1,
							cprData.m_szColumnName, -1);
					if(res==CSTR_GREATER_THAN) break;
					if(res==CSTR_EQUAL)
					{
						res = CompareStringW(LOCALE_USER_DEFAULT, 
								NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
								m_rgRowData[nPos].m_szPrivilegeType, -1,
								cprData.m_szPrivilegeType, -1);
						if(res==CSTR_GREATER_THAN) break;
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
		if(res==CCI_ER_NO_MORE_DATA) return S_OK;
		else if(res<0) return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(err_buf.err_msg, uCodepage));
	}

	return S_OK;
}

HRESULT CSRColumnPrivileges::Execute(LONG* pcRowsAffected,
				ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRColumnPrivileges::Execute\n");

	ClearError();

	int hConn = CCUBRIDSession::GetSessionPtr(this)->GetConnection();
	UINT uCodepage = CCUBRIDSession::GetSessionPtr(this)->GetCodepage();

	if (pcRowsAffected)
		*pcRowsAffected = 0;

	WCHAR table_name[256]; table_name[0] = 0;
	WCHAR column_name[256]; column_name[0] = 0;

	GetRestrictions(cRestrictions, rgRestrictions, table_name, column_name);

	CComVariant grantee;
	HRESULT hr = GetCurrentUser(this, grantee);
	if(FAILED(hr)) return hr;

	CAtlArray<CStringA> rgTableNames;
	if(table_name[0])
		rgTableNames.Add(CW2A(table_name, uCodepage));
	else
		Util::GetTableNames(hConn, rgTableNames);

	for(size_t i=0;i<rgTableNames.GetCount();i++)
	{
		T_CCI_ERROR err_buf;
		int hReq;
		CW2A _columnName(column_name, uCodepage);
		hReq = cci_schema_info(hConn, CCI_SCH_ATTR_PRIVILEGE, rgTableNames[i].GetBuffer(),
								(column_name[0] ? (LPSTR) _columnName : NULL),
								CCI_ATTR_NAME_PATTERN_MATCH, &err_buf);
		
		if(hReq<0)
		{
			ATLTRACE2("CSRColumnPrivileges: cci_schema_info fail\n");
			return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(err_buf.err_msg, uCodepage));
		}

		hr = FillRowData(hReq, uCodepage, pcRowsAffected, grantee, rgTableNames[i], _columnName);
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
