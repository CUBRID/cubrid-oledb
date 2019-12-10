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
#include "type.h"
#include "Error.h"
#include "DataSource.h"

CSRColumns::~CSRColumns()
{
	if(m_spUnkSite)
	{
		CCUBRIDSession* pSession = CCUBRIDSession::GetSessionPtr(this);
		if (pSession->m_cSessionsOpen == 1)	pSession->RowsetCommit();
	}
}

static void GetRestrictions(ULONG cRestrictions, const VARIANT *rgRestrictions,
							PWSTR table_name, PWSTR column_name)
{
	// restriction�� ���ٰ� �׻� cRestrictions==0�� �ƴϴ�.
	// ���� vt!=VT_EMPTY���� � �˻������ �Ѵ�.

	if(cRestrictions>=3 && V_VT(&rgRestrictions[2])==VT_BSTR && V_BSTR(&rgRestrictions[2])!=NULL)
	{	// TABLE_NAME restriction
		wcsncpy(table_name, V_BSTR(&rgRestrictions[2]), 1023);
		ATLTRACE2(L"\tTable Name = %s\n", V_BSTR(&rgRestrictions[2]));
		table_name[1023] = 0; // ensure zero-terminated string
	}

	if(cRestrictions>=4 && V_VT(&rgRestrictions[3])==VT_BSTR && V_BSTR(&rgRestrictions[3])!=NULL)
	{	// COLUMN_NAME restriction
		wcsncpy(column_name, V_BSTR(&rgRestrictions[3]), 1023);
		ATLTRACE2("\tColumn Name = %s\n", V_BSTR(&rgRestrictions[3]));
		column_name[1023] = 0; // ensure zero-terminated string
	}
}

// S_OK : ����
// E_FAIL : ����
static HRESULT FetchData(int hReq, UINT uCodepage, CCOLUMNSRow &crData)
{
	char *cvalue;
	int ivalue, ind, res;
	T_CCI_ERROR err_buf;
	int nCCIUType, nPrecision, nScale;
	bool bNullable;

	res = cci_fetch(hReq, &err_buf);
	if(res<0) return RaiseError(E_FAIL, 1, __uuidof(IDBSchemaRowset), CA2W(err_buf.err_msg, uCodepage));

	res = cci_get_data(hReq, 11, CCI_A_TYPE_STR, &cvalue, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

	wcsncpy(crData.m_szTableName, CA2W(cvalue, uCodepage), 128);
	crData.m_szTableName[128] = 0;
	_wcsupr(crData.m_szTableName);

	res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &cvalue, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

	wcsncpy(crData.m_szColumnName, CA2W(cvalue, uCodepage), 128);
	crData.m_szColumnName[128] = 0;
	_wcsupr(crData.m_szColumnName);

	res = cci_get_data(hReq, 2, CCI_A_TYPE_INT, &ivalue, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));

	nCCIUType = ivalue;

	if(nCCIUType==-1)
		crData.m_nDataType = Type::GetStaticTypeInfo(CCI_U_TYPE_STRING).nOLEDBType;
	else
		crData.m_nDataType = Type::GetStaticTypeInfo(ivalue).nOLEDBType;

	res = cci_get_data(hReq, 3, CCI_A_TYPE_INT, &ivalue, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	nScale = ivalue;

	res = cci_get_data(hReq, 4, CCI_A_TYPE_INT, &ivalue, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	nPrecision = ivalue;

	res = cci_get_data(hReq, 6, CCI_A_TYPE_INT, &ivalue, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	crData.m_bIsNullable = ( ivalue==0 ? ATL_VARIANT_TRUE : ATL_VARIANT_FALSE );
	bNullable = (ivalue==0);

	res = cci_get_data(hReq, 10, CCI_A_TYPE_INT, &ivalue, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	crData.m_ulOrdinalPosition = ivalue;

	res = cci_get_data(hReq, 9, CCI_A_TYPE_STR, &cvalue, &ind);
	if(res<0) return RaiseError(E_FAIL, 0, __uuidof(IDBSchemaRowset));
	if(ind==-1)
	{
		crData.m_bColumnHasDefault = ATL_VARIANT_FALSE;
		crData.m_szColumnDefault[0] = L'\x0';
	}
	else
	{
		crData.m_bColumnHasDefault = ATL_VARIANT_TRUE;
		wcsncpy(crData.m_szColumnDefault, CA2W(cvalue, uCodepage), 128);
		crData.m_szColumnDefault[128] = 0;
	}

	Type::DynamicTypeInfo dyn_info =
		( nCCIUType==-1 ? Type::GetDynamicTypeInfo(CCI_U_TYPE_STRING, 0, 0, true) :
						  Type::GetDynamicTypeInfo(nCCIUType, nPrecision, nScale, bNullable) );
	crData.m_ulColumnFlags = dyn_info.ulFlags;
	crData.m_nNumericPrecision = dyn_info.nNumericPrecision;
	crData.m_nNumericScale = dyn_info.nNumericScale;
	crData.m_ulDateTimePrecision = dyn_info.ulDateTimePrecision;

	// TODO: CCI_PARAM_MAX_STRING_LENGTH ����?
	crData.m_ulCharMaxLength = dyn_info.ulCharMaxLength;
	crData.m_ulCharOctetLength = dyn_info.ulCharOctetLength;

	return S_OK;
}

HRESULT CSRColumns::Execute(LONG * /*pcRowsAffected*/,
				ULONG cRestrictions, const VARIANT *rgRestrictions)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CSRColumns::Execute\n");

	ClearError();

	int hConn = CCUBRIDSession::GetSessionPtr(this)->GetConnection();
	UINT uCodepage = CCUBRIDSession::GetSessionPtr(this)->GetCodepage();
	ULONG ulMaxLen = CCUBRIDSession::GetSessionPtr(this)->GetDataSourcePtr()->PARAM_MAX_STRING_LENGTH;

	WCHAR table_name[1024]; table_name[0] = 0;
	WCHAR column_name[1024]; column_name[0] = 0;
	GetRestrictions(cRestrictions, rgRestrictions, table_name, column_name);

	{
		T_CCI_ERROR err_buf;
		CW2A _tableName(table_name, uCodepage);
		CW2A _columnName(column_name, uCodepage);
		int hReq = cci_schema_info(hConn, CCI_SCH_ATTRIBUTE,
							(table_name[0] ? (PSTR) _tableName : NULL),
							(column_name[0]? (PSTR) _columnName : NULL),
							CCI_CLASS_NAME_PATTERN_MATCH | CCI_ATTR_NAME_PATTERN_MATCH,
							&err_buf);
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
			CCOLUMNSRow crData;
			HRESULT hr = FetchData(hReq, uCodepage, crData);
			if(FAILED(hr)) goto error;

			//MAX STRING LENGTH �ݿ�
			if (crData.m_ulCharMaxLength != (ULONG)~0 && crData.m_ulCharMaxLength > ulMaxLen)
				crData.m_ulCharMaxLength = ulMaxLen;
			if (crData.m_ulCharMaxLength != (ULONG)~0 && crData.m_ulCharOctetLength > ulMaxLen)
				crData.m_ulCharOctetLength = ulMaxLen;

			_ATLTRY
			{
				// TABLE_NAME ������ �����Ѵ�.
				size_t nPos;
				for( nPos=0 ; nPos<m_rgRowData.GetCount() ; nPos++ )
				{
					int res = CompareStringW(LOCALE_USER_DEFAULT, 
							NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | SORT_STRINGSORT,
							m_rgRowData[nPos].m_szTableName, -1,
							crData.m_szTableName, -1);
					if(res==CSTR_GREATER_THAN) break;
				}
				m_rgRowData.InsertAt(nPos, crData);
			}
			_ATLCATCHALL()
			{
				ATLTRACE2("out of memory\n");
				cci_close_req_handle(hReq);
				return E_OUTOFMEMORY;
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

DBSTATUS CSRColumns::GetDBStatus(CSimpleRow *pRow, ATLCOLUMNINFO* pInfo)
{
	ATLTRACE2(atlTraceDBProvider, 3, "CSRColumns::GetDBStatus\n");
	switch(pInfo->iOrdinal)
	{
	case 3: // TABLE_NAME
	case 4: // COLUMN_NAME
	case 7: // ORDINAL_POSITION
	case 8: // COLUMN_HASDEFAULT
	case 10: // COLUMN_FLAGS
	case 11: // IS_NULLABLE
	case 12: // DATA_TYPE
		return DBSTATUS_S_OK;
	case 9: // COLUMN_DEFAULT
		if(m_rgRowData[pRow->m_iRowset].m_bColumnHasDefault==ATL_VARIANT_FALSE)
			return DBSTATUS_S_ISNULL;
		else
			return DBSTATUS_S_OK;
	case 14: // CHARACTER_MAXIMUM_LENGTH
		if(m_rgRowData[pRow->m_iRowset].m_ulCharMaxLength==(ULONG)~0)
			return DBSTATUS_S_ISNULL;
		else
			return DBSTATUS_S_OK;
	case 15: // CHARACTER_OCTET_LENGTH
		if(m_rgRowData[pRow->m_iRowset].m_ulCharOctetLength==(ULONG)~0)
			return DBSTATUS_S_ISNULL;
		else
			return DBSTATUS_S_OK;
	case 16: // NUMERIC_PRECISION
		if(m_rgRowData[pRow->m_iRowset].m_nNumericPrecision==(USHORT)~0)
			return DBSTATUS_S_ISNULL;
		else
			return DBSTATUS_S_OK;
	case 17: // NUMERIC_SCALE
		if(m_rgRowData[pRow->m_iRowset].m_nNumericScale==-1)
			return DBSTATUS_S_ISNULL;
		else
			return DBSTATUS_S_OK;
	case 18: // DATETIME_PRECISION
		if(m_rgRowData[pRow->m_iRowset].m_ulDateTimePrecision==(ULONG)~0)
			return DBSTATUS_S_ISNULL;
		else
			return DBSTATUS_S_OK;
	default:
		return DBSTATUS_S_ISNULL;
	}
}
