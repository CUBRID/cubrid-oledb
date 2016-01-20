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

CSRViews::~CSRViews()
{
	if(m_spUnkSite)
	{
		CCUBRIDSession* pSession = CCUBRIDSession::GetSessionPtr(this);
		if (pSession->m_cSessionsOpen == 1)	pSession->RowsetCommit();
	}
}

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions, PWSTR view_name)
{
	// restriction�� ���ٰ� �׻� cRestrictions==0�� �ƴϴ�.
	// ���� vt!=VT_EMPTY���� � �˻������ �Ѵ�.

	if(cRestrictions>=3 && V_VT(&rgRestrictions[2])==VT_BSTR && V_BSTR(&rgRestrictions[2])!=NULL)
	{	// view_name restriction
		ATLTRACE2(L"\tView Name = %s\n", V_BSTR(&rgRestrictions[2]));
		wcsncpy(view_name, V_BSTR(&rgRestrictions[2]), 255);
		view_name[255] = 0; // ensure zero-terminated string
	}
}

// S_OK : ����
// S_FALSE : �����ʹ� ���������� Consumer�� ���ϴ� �����Ͱ� �ƴ�
// E_FAIL : ����
static HRESULT FetchData(int hReq, UINT uCodepage, CViewsRow &vData)
{
	char *value;
	int ind, res;
	T_CCI_ERROR err_buf;
	
	res = cci_fetch(hReq, &err_buf);
	if(res<0) return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(err_buf.err_msg, uCodepage));

	res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &value, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	wcscpy(vData.m_szTableName, CA2W(value, uCodepage));
	
	return S_OK;
}

static HRESULT GetViewDefinition(int hConn, UINT uCodepage, CViewsRow &vData)
{
	char *value;
	int ind, res;
	T_CCI_ERROR err_buf;
	
	int hReq = cci_schema_info(hConn, CCI_SCH_QUERY_SPEC, CW2A(vData.m_szTableName, uCodepage),
			NULL, 0, &err_buf);

	res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
    if(res==CCI_ER_NO_MORE_DATA)
		return S_OK;

	if(res<0) return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(err_buf.err_msg, uCodepage));

	res = cci_fetch(hReq, &err_buf);
	if(res<0) return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(err_buf.err_msg, uCodepage));

	res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &value, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	wcscpy(vData.m_szViewDefinition, CA2W(value, uCodepage));

	// TODO: �´� ��?
	vData.m_bCheckOption = ATL_VARIANT_FALSE;
	vData.m_bIsUpdatable = ATL_VARIANT_FALSE;

	cci_close_req_handle(hReq);

	return S_OK;
}

HRESULT CSRViews::Execute(LONG* pcRowsAffected,
				ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRViews::Execute\n");

	ClearError();

	if (pcRowsAffected)
			*pcRowsAffected = 0;

	int hConn = CCUBRIDSession::GetSessionPtr(this)->GetConnection();
	UINT uCodepage = CCUBRIDSession::GetSessionPtr(this)->GetCodepage();
	WCHAR view_name[256]; view_name[0] = 0;
	
	GetRestrictions(cRestrictions, rgRestrictions, view_name);
	{
		T_CCI_ERROR err_buf;
		CW2A _viewName(view_name, uCodepage);
		int hReq = cci_schema_info(hConn, CCI_SCH_VCLASS, (view_name[0] ? (PSTR) _viewName : NULL),
			NULL, CCI_CLASS_NAME_PATTERN_MATCH, &err_buf);
		if(hReq<0)
		{
			ATLTRACE2("cci_schema_info fail\n");
			return E_FAIL;
		}

		int res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
        if(res==CCI_ER_NO_MORE_DATA) goto done;
		if(res<0) goto error;

		while(1)
		{
			CViewsRow vData;
			HRESULT hr = FetchData(hReq, uCodepage, vData);
			if(FAILED(hr))
			{
				cci_close_req_handle(hReq);
				return hr;
			}

			hr = GetViewDefinition(hConn, uCodepage, vData);
			if(FAILED(hr))
			{
				cci_close_req_handle(hReq);
				return hr;
			}

			if(hr==S_OK) // S_FALSE�� �߰����� ����
			{
				_ATLTRY
				{
					// TABLE_NAME ������ �����ؾ� �Ѵ�.
					size_t nPos;
					for( nPos=0 ; nPos<m_rgRowData.GetCount() ; nPos++ )
					{
						int res = CompareStringW(LOCALE_USER_DEFAULT, 
								NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
								m_rgRowData[nPos].m_szTableName, -1,
								vData.m_szTableName, -1);
						if(res==CSTR_GREATER_THAN) break;
					}
					m_rgRowData.InsertAt(nPos, vData);
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
			if(res==CCI_ER_NO_MORE_DATA) goto done;
			if(res<0) goto error;
		}

error:
		ATLTRACE2("fail to fetch data\n");
		cci_close_req_handle(hReq);
		return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(err_buf.err_msg, uCodepage));
done:
		cci_close_req_handle(hReq);
	}

	return S_OK;
}

DBSTATUS CSRViews::GetDBStatus(CSimpleRow *pRow, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRViews::GetDBStatus\n");
	switch(pInfo->iOrdinal)
	{
	case 3: // TABLE_NAME
	case 4: // VIEW_DEFINITION
	case 5: // CHECK_OPTION
	case 6: // IS_UPDATABLE
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_S_ISNULL;
	}
}
