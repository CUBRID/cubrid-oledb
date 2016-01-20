////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CUBRIDrowrowset.h"
#include "Session.h"
#include "CUBRIDStream.h"

HRESULT CCUBRIDRowRowset::GetConnectionHandle(int *hConn)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDRowRowset::GetConnectionHandle\n");

	CComPtr<IRow> spCom;
	HRESULT hr = GetSite(__uuidof(IRow), (void **)&spCom);

	CCUBRIDRow* pRow = static_cast<CCUBRIDRow *>((IRow *)spCom);
	hr = pRow->GetSessionPtr()->GetConnectionHandle(hConn);
	if (FAILED(hr))
		return E_FAIL;

	return S_OK;
}

HRESULT CCUBRIDRowRowset::FetchData(T_CCI_SET** set)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDRowRowset::FetchData\n");

	int hConn, ind, res;
	char* attr_list[2];
	T_CCI_ERROR error;

	HRESULT hr = GetConnectionHandle(&hConn);
	if (FAILED(hr))
		return E_FAIL;

	attr_list[0] = _strdup(m_colName);
	attr_list[1] = NULL;

	res = cci_oid_get(hConn, m_szOID, attr_list, &error);
	if (res < 0)
		return E_FAIL;

	m_hReq = res;

	res = cci_cursor(m_hReq, 1, CCI_CURSOR_CURRENT, &error);
	if (res == CCI_ER_NO_MORE_DATA)
		return E_FAIL;

	res = cci_fetch(m_hReq, &error);
	if(res < 0)
		return E_FAIL;

	res = cci_get_data(m_hReq, 1, CCI_A_TYPE_SET, &(*set), &ind);
	if(res < 0)
		return E_FAIL;

	return S_OK;
}

HRESULT CCUBRIDRowRowset::Execute(LONG *pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDRowRowset::Execute\n");

	HRESULT hr = S_OK;
	T_CCI_SET* set;
	int ind;
	int set_size;

	*pcRowsAffected = 0;

	hr = FetchData(&set);
	if(FAILED(hr))
		goto error;

	if(hr == S_OK)
	{
		_ATLTRY
		{
			set_size = cci_set_size(set);
			for (int i = 0; i < set_size; i++)
			{
				CCUBRIDClassRows crData;
				char* strVal = NULL;

				if (cci_set_get(set, i + 1, CCI_A_TYPE_STR, &strVal, &ind) < 0)
					return E_FAIL;

				//crData.m_strData = _wcsdup(CA2W(strVal));
				wcscpy(crData.m_strData , CA2W(strVal));
				m_rgRowData.InsertAt(i, crData);
			}

			cci_set_free(set);
		}
		_ATLCATCHALL()
		{
			ATLTRACE2("Out of memory\n");
			cci_close_req_handle(m_hReq);
			return E_OUTOFMEMORY;
		}
	}

	*pcRowsAffected = set_size;

	return hr;

error:
	ATLTRACE2("Failed to fetch data\n");
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