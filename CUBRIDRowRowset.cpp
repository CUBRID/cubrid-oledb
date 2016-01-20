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

#include "StdAfx.h"
#include "CUBRIDrowrowset.h"
#include "Session.h"

// S_OK : ����
// S_FALSE : �����ʹ� ���������� Consumer�� ���ϴ� �����Ͱ� �ƴ�
// E_FAIL : ����
HRESULT CCUBRIDRowRowset::FetchData(T_CCI_SET** set)
{
	int hConn, ind, res;
	char* attr_list[2];
	T_CCI_ERROR error;

	CComPtr<IRow> spCom;
	HRESULT hr = GetSite(__uuidof(IRow), (void **)&spCom);
	if (FAILED(hr)) return E_FAIL;

	CCUBRIDRow* pRow = static_cast<CCUBRIDRow *>((IRow *)spCom);
	hConn = pRow->GetSessionPtr()->GetConnection();
	m_uCodepage = pRow->GetSessionPtr()->GetCodepage();

	//Read�� �÷��� �÷����� �����Ѵ�
	CW2A _colName(m_colName, m_uCodepage);
	attr_list[0] = (PSTR) _colName;
	attr_list[1] = NULL;

	res = cci_oid_get(hConn, m_szOID, attr_list, &error);
	if (res < 0) return E_FAIL;

	m_hReq = res;

	res = cci_cursor(m_hReq, 1, CCI_CURSOR_CURRENT, &error);
    if (res == CCI_ER_NO_MORE_DATA)
		return E_FAIL;

	res = cci_fetch(m_hReq, &error);
	if(res<0) return E_FAIL;
	
	res = cci_get_data(m_hReq, 1, CCI_A_TYPE_SET, &(*set), &ind);
	if(res<0) return E_FAIL;
	 
	return S_OK;
}

HRESULT CCUBRIDRowRowset::Execute(LONG* pcRowsAffected,
				ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDRowRowset::Execute\n");
	HRESULT hr = S_OK;
	T_CCI_SET* set;
	int ind;
	int set_size;

	*pcRowsAffected = 0;

	hr = FetchData(&set);
	if(FAILED(hr)) goto error;

	if(hr==S_OK) // S_FALSE�� �߰����� ����
	{
		_ATLTRY
		{
			set_size = cci_set_size(set);
			for (int i = 0; i < set_size; i++)
			{
				CCUBRIDClassRows crData;
				char* strVal = NULL;

				if (cci_set_get(set, i+1, CCI_A_TYPE_STR, &strVal, &ind) < 0)
					return E_FAIL;

				wcscpy(crData.m_strData , CA2W(strVal, m_uCodepage));
				
				m_rgRowData.InsertAt(i, crData);
			}

			cci_set_free(set);
		}
		_ATLCATCHALL()
		{
			ATLTRACE2("out of memory\n");
			cci_close_req_handle(m_hReq);
			return E_OUTOFMEMORY;
		}	
	}

	*pcRowsAffected = set_size;
	return hr;

error:
		ATLTRACE2("fail to fetch data\n");
		cci_close_req_handle(m_hReq);
		return E_FAIL;
}

DBSTATUS CCUBRIDRowRowset::GetDBStatus(CSimpleRow *pRow, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CCUBRIDRowRowset::GetDBStatus\n");
	switch(pInfo->iOrdinal)
	{
	case 1: 
		return DBSTATUS_S_OK;
	default:
		return DBSTATUS_S_ISNULL;
	}
}