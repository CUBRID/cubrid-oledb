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

// Row.cpp : Implementation of CCUBRIDRow

#include "stdafx.h"
#include "Row.h"
#include "Rowset.h"
#include "CUBRIDRowRowset.h"
#include "CUBRIDStream.h"
#include "Session.h"
#include "Error.h"

CCUBRIDSession *CCUBRIDRow::GetSessionPtr()
{
	switch(m_eType)
	{
	case FromSession:
		return CCUBRIDSession::GetSessionPtr(this);
	case FromCommand:
		return GetCommandPtr()->GetSessionPtr();
	case FromRowset:
		return GetRowsetPtr()->GetSessionPtr();
	case FromRow:
		return GetRowPtr()->GetSessionPtr();
	default:
		return NULL;
	}
}

CCUBRIDCommand *CCUBRIDRow::GetCommandPtr()
{
	switch(m_eType)
	{
	case FromCommand:
		return CCUBRIDCommand::GetCommandPtr(this);
	case FromRowset:
		// Rowset�� Session�� ���ؼ� �����Ǿ����� NULL�� ��ȯ�� ���� �ִ�.
		return GetRowsetPtr()->GetCommandPtr();
	default:
		return NULL;
	}
}

CCUBRIDRowset *CCUBRIDRow::GetRowsetPtr()
{
	if(m_eType!=FromRowset) return NULL;

	return CCUBRIDRowset::GetRowsetPtr(this);
}

CCUBRIDRow *CCUBRIDRow::GetRowPtr()
{
	if(m_eType!=FromRow) return NULL;

	CComPtr<IRow> spCom;
	HRESULT hr = GetSite(__uuidof(IRow), (void **)&spCom);
	// ����� ���α׷��� ������, �����ϴ� ��찡 ������?
	ATLASSERT(SUCCEEDED(hr));
	// ���� ������带 �����ذ��� dynamic_cast�� �� �ʿ�� ���� ��
	return static_cast<CCUBRIDRow *>((IRow *)spCom);
}

CCUBRIDRow::CCUBRIDRow()
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRow::CCUBRIDRow\n");
	m_eType = Invalid;
	m_bIsCommand = FALSE;
	m_hRow = 0;
	m_cCol = 0;
	m_bHasParamaters = FALSE;
	m_bIsValid = true;
	m_uCodepage = _AtlGetConversionACP();
}

CCUBRIDRow::~CCUBRIDRow()
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRow::~CCUBRIDRow\n");
	if(m_eType==FromRowset)
		GetRowsetPtr()->ReleaseRows(1, &m_hRow, 0, 0, 0);

	if(m_spUnkSite)
		GetSessionPtr()->RegisterTxnCallback(this, false);
}

void CCUBRIDRow::TxnCallback(const ITxnCallback *pOwner)
{
	if(pOwner!=this)
	{
		cci_close_req_handle(m_hReq);
		m_hReq = 0;
		m_bIsValid = false;
	}
}

STDMETHODIMP CCUBRIDRow::SetSite(IUnknown *pUnkSite, Type eType)
{
	m_eType = eType;
	HRESULT hr = IObjectWithSiteImpl<CCUBRIDRow>::SetSite(pUnkSite);
	GetSessionPtr()->RegisterTxnCallback(this, true);
	return hr;
}

HRESULT CCUBRIDRow::Initialize(int hReq, bool bBookmarks, HROW hRow, DBCOUNTITEM iRowset)
{
	char** attr_list;
	int hConn, rc;
	T_CCI_ERROR error;
	HRESULT hr = S_OK;
	int i;

	//Row ��ü�� ������ request handle�� ������ ����
	//by risensh1ne
	//20030624
	
	hConn = GetSessionPtr()->GetConnection();
	m_uCodepage = GetSessionPtr()->GetCodepage();
	m_info = cci_get_result_info(hReq, &m_cmdType, &m_cCol);
	if (!m_info)
		return RaiseError(E_FAIL, 0, __uuidof(IRow));
	
	attr_list = (char **)CoTaskMemAlloc((m_cCol + 1) * sizeof(char *));
	for (i = 0; i < m_cCol; i++)
		attr_list[i] = _strdup(m_info[i].real_attr);
	attr_list[i] = NULL;

	if (m_eType == FromRowset)
	{
		CCUBRIDRowsetRow *pRow = 0;
		if(!GetRowsetPtr()->m_rgRowHandles.Lookup((ULONG)hRow, pRow) || pRow==NULL)
			return E_FAIL;
		strcpy(m_szOID, pRow->m_szOID);
	} else if (m_eType == FromCommand || m_eType == FromSession)
	{
		rc = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &error);

		if (rc == -5)
			return RaiseError(DB_E_NOTFOUND, 0, __uuidof(IRow), CA2W(error.err_msg, m_uCodepage));
		if (rc < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IRow), CA2W(error.err_msg, m_uCodepage));
		rc = cci_fetch(hReq, &error);
		if (rc < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IRow), CA2W(error.err_msg, m_uCodepage));
		rc = cci_get_cur_oid(hReq, m_szOID);
		if (rc < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IRow), CA2W(error.err_msg, m_uCodepage));
	} else
	{
		strcpy(m_szOID, GetRowPtr()->m_szOID);
	}

	//�Լ�� ���� ������ ��� OID���� @0|0|0�� ��ȯ�ȴ�.
	//�� ��� �Ѿ�� request handle�� �״�� ����Ѵ�.
	if (!strcmp(m_szOID, "@0|0|0"))
		m_hReq = hReq;
	else
	{
		rc = cci_oid_get(hConn, m_szOID, attr_list, &error);

		//������ ��� �Ѿ�� request handle�� �״�� ���
		//�����ϴ� ���� �ϳ� �̻��� �÷��� �Լ��� ����� ��� �߻� 
		if (rc < 0)
			m_hReq = hReq;
			//return RaiseError(E_FAIL, 0, __uuidof(IRow), error.err_msg);
		else
			m_hReq = rc;
	}

	m_bBookmarks = bBookmarks;
	m_hRow = hRow;
	m_iRowset = iRowset;
	if(m_eType==FromRowset)
		GetRowsetPtr()->AddRefRows(1, &m_hRow, 0, 0);

	return hr;
}

ATLCOLUMNINFO* CCUBRIDRow::GetColumnInfo(CCUBRIDRow *pv, DBORDINAL *pcCols)
{
	if(!pv->m_Columns.m_pInfo)
	{
		// TODO : check error?
		pv->m_Columns.GetColumnInfo(pv->m_uCodepage, pv->m_info, pv->m_cmdType, pv->m_cCol, pv->m_bBookmarks);
	}

	if(pcCols)
		*pcCols = pv->m_Columns.m_cColumns;
	return pv->m_Columns.m_pInfo;
}

STDMETHODIMP CCUBRIDRow::GetColumnInfo(DBORDINAL *pcColumns, DBCOLUMNINFO **prgInfo,
						OLECHAR **ppStringsBuffer)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRow::GetColumnInfo\n");

	ClearError();

	if(!m_bIsValid) return RaiseError(E_UNEXPECTED, 0, __uuidof(IColumnsInfo));
	return IColumnsInfoImpl<CCUBRIDRow>::GetColumnInfo(pcColumns, prgInfo, ppStringsBuffer);
}

STDMETHODIMP CCUBRIDRow::MapColumnIDs(DBORDINAL cColumnIDs, const DBID rgColumnIDs[],
						DBORDINAL rgColumns[])
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRow::MapColumnIDs\n");

	ClearError();

	if(!m_bIsValid) return RaiseError(E_UNEXPECTED, 0, __uuidof(IColumnsInfo));
	return IColumnsInfoImpl<CCUBRIDRow>::MapColumnIDs(cColumnIDs, rgColumnIDs, rgColumns);
}

STDMETHODIMP CCUBRIDRow::GetSourceRowset(REFIID riid, IUnknown **ppRowset, HROW *phRow)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRow::GetSourceRowset\n");
	ClearError();
	HRESULT hr = S_OK;

	if (!m_bIsValid)
	{
		hr = E_UNEXPECTED;
		goto error1;
	}

	if(ppRowset) *ppRowset = NULL;
	if(phRow) *phRow = NULL;
	if(ppRowset==NULL && phRow==NULL)
	{
		hr = E_INVALIDARG;
		goto error1;
	}

	if(m_eType==FromSession || m_eType==FromCommand || m_eType==FromRow)
	{
		hr = DB_E_NOSOURCEOBJECT;
		goto error1;
	}
	else if(ppRowset)
	{
		hr = GetSite(riid, (void **)ppRowset);
		if (FAILED(hr)) goto error1;
	}

	if(SUCCEEDED(hr) && phRow)
		*phRow = m_hRow;

	return hr;

error1:
	if (ppRowset) *ppRowset = NULL;
	if (phRow) *phRow = DB_NULL_HROW;
	return RaiseError(hr, 0, __uuidof(IRow));
}

STDMETHODIMP CCUBRIDRow::GetColumns(DBORDINAL cColumns, DBCOLUMNACCESS rgColumns[])
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRow::GetColumns\n");

	ClearError();

	HRESULT hr = S_OK;
	CCUBRIDRowsetRow *pRow = 0;

	if(cColumns && !rgColumns)
	{
		hr = E_INVALIDARG;
		goto error2;
	}
	
	DBORDINAL cCols;
	ATLCOLUMNINFO *pInfo = GetColumnInfo(this, &cCols);

	// Session�̳� Command���� ������ ���� ù��° row��
	// Rowset���� ������ ���� m_hRow�� �ش��ϴ� row�� �����´�.
	if(m_eType==FromRowset)
	{
		CCUBRIDRowset *pRowset = GetRowsetPtr();
		if(!pRowset->m_rgRowHandles.Lookup((ULONG)m_hRow, pRow) || pRow==NULL)
		{
			hr = DB_E_ERRORSOCCURRED; // or DB_E_DELETEDROW?
			goto error2;
		}
	}
	else if(m_eType==FromSession || m_eType==FromCommand)
	{
		pRow = new CCUBRIDRowsetRow(m_uCodepage, 0, cCols, pInfo, m_spConvert);
		if(pRow==NULL)
			return E_OUTOFMEMORY;
		HRESULT hr = pRow->ReadData(m_hReq);
		if(FAILED(hr))
		{
			delete pRow;
			pRow = NULL;
			return hr;
		}
	}
	else if (m_eType==FromRow)
	{
		pRow = new CCUBRIDRowsetRow(m_uCodepage, 0, cCols, pInfo, m_spConvert);
		if(pRow==NULL)
			return E_OUTOFMEMORY;
		HRESULT hr = pRow->ReadData(m_hReq, m_szOID);
		if(FAILED(hr))
		{
			delete pRow;
			pRow = NULL;
			goto error2;
		}
	} else
	{
		hr = E_FAIL;
		goto error2;
	}
		
	if(pRow->m_status==DBPENDINGSTATUS_INVALIDROW || pRow->m_status==DBPENDINGSTATUS_DELETED)
		return DB_E_DELETEDROW;

	//DB_E_DELETEDROW���� ���߿� �Ǵ��Ѵ�.
	if (!m_bIsValid)
	{
		hr = E_UNEXPECTED;
		goto error2;
	}

	hr = pRow->WriteData(cColumns, rgColumns);
	if (FAILED(hr)) goto error2;
		
	if(m_eType==FromSession || m_eType==FromCommand)
	{
		delete pRow;
	}
	return hr;

error2:
	if(pRow && (m_eType==FromSession || m_eType==FromCommand))
	{
		delete pRow;
	}
	return RaiseError(hr, 0, __uuidof(IRow));
}

STDMETHODIMP CCUBRIDRow::Open(IUnknown *pUnkOuter, DBID *pColumnID, REFGUID rguidColumnType,
						DWORD dwBindFlags, REFIID riid, IUnknown **ppUnk)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRow::Open\n");

	HRESULT hr = S_OK;
	int col_index;
	LPOLESTR colName;
	T_CCI_U_TYPE col_type;
	bool col_found = false;
	
	ClearError();

	if (!pColumnID || !ppUnk)
		return RaiseError(E_INVALIDARG, 0, __uuidof(IRow));
	*ppUnk = NULL;

	if(!m_bIsValid) return RaiseError(E_UNEXPECTED, 0, __uuidof(IRow));

	if (pUnkOuter && riid != IID_IUnknown)
		return RaiseError(DB_E_NOAGGREGATION, 0, __uuidof(IRow));
	
	if (IsValidDBID(pColumnID) != S_OK)
		return RaiseError(DB_E_BADCOLUMNID, 0, __uuidof(IRow));
	if (pColumnID->eKind == DBKIND_PROPID ||
		pColumnID->eKind == DBKIND_GUID ||
		pColumnID->eKind == DBKIND_PGUID_PROPID ||
		pColumnID->eKind == DBKIND_GUID_PROPID)
		return RaiseError(DB_E_BADCOLUMNID, 0, __uuidof(IRow));

	//�÷� �ε����� �˾Ƴ���
	colName = pColumnID->uName.pwszName;
	if (colName)
	{
		for (int i = 0; i < m_cCol; i++)
		{
			char* _colName = CCI_GET_RESULT_INFO_ATTR_NAME(m_info, i+1);
			if (!_wcsicmp(CA2W(_colName, m_uCodepage), colName))
			{
				col_index = i+1;
				col_found = true;
				break;
			}
		}
		if (!col_found)
			return RaiseError(DB_E_BADCOLUMNID, 0, __uuidof(IRow));
	} else
		return RaiseError(DB_E_BADCOLUMNID, 0, __uuidof(IRow));
	
	col_type = CCI_GET_RESULT_INFO_TYPE(m_info, col_index);
	
	if (rguidColumnType == DBGUID_STREAM)
	{
		if (!(col_type == CCI_U_TYPE_STRING || col_type == CCI_U_TYPE_CHAR ||
			 col_type == CCI_U_TYPE_BIT || col_type == CCI_U_TYPE_VARBIT))
			return RaiseError(DB_E_OBJECTMISMATCH, 0, __uuidof(IRow));

		//�÷����� NULL�� ��� DB_E_NOTFOUND�� �����Ѵ�.
		{
			T_CCI_ERROR error;
			void* buffer;
			int res, ind;
			res = cci_cursor(m_hReq, 1, CCI_CURSOR_FIRST, &error);
			if (res < 0) return RaiseError(E_FAIL, 0, __uuidof(IRow));
			res = cci_fetch(m_hReq, &error);
			if (res < 0) return RaiseError(E_FAIL, 0, __uuidof(IRow));
			res = cci_get_data(m_hReq, col_index, CCI_A_TYPE_STR, &buffer, &ind);
			if (res < 0) return RaiseError(E_FAIL, 0, __uuidof(IRow));
			if (ind == -1) return DB_E_NOTFOUND;
		}

		CComPolyObject<CCUBRIDStream>* pObjStream;
	
		hr = CComPolyObject<CCUBRIDStream>::CreateInstance(pUnkOuter, &pObjStream);
		if (FAILED(hr))
			return hr;

		// ������ COM ��ü�� �����ؼ�, ���н� �ڵ� �����ϵ��� �Ѵ�.
		CComPtr<IUnknown> spUnk;
		hr = pObjStream->QueryInterface(&spUnk);
		if(FAILED(hr))
		{
			delete pObjStream; // �������� �ʾұ� ������ �������� �����.
			return hr;
		}

		int hConn = GetSessionPtr()->GetConnection();
		UINT uCodepage = GetSessionPtr()->GetCodepage();
		pObjStream->m_contained.Initialize(hConn, uCodepage, m_szOID, m_info[col_index - 1]);
		
		CComPtr<ISequentialStream> spStream;
		hr = pObjStream->QueryInterface(IID_ISequentialStream, (void **)&spStream);
		if (FAILED(hr))
			return E_NOINTERFACE;
		
		*ppUnk = spStream.Detach();
	} else
	{
		//�÷��� collection type�� �ƴ� ��� ����
		if (!CCI_IS_SET_TYPE(col_type) && !CCI_IS_MULTISET_TYPE(col_type) && !CCI_IS_SEQUENCE_TYPE(col_type))
			return RaiseError(DB_E_OBJECTMISMATCH, 0, __uuidof(IRow));

		if (rguidColumnType == DBGUID_ROW)
		{
			//Row object�� �����Ѵ�.
			CComPolyObject<CCUBRIDRow> *pRow;
			hr = CComPolyObject<CCUBRIDRow>::CreateInstance(pUnkOuter, &pRow);
			if(FAILED(hr))
				return hr;

			// ������ COM ��ü�� �����ؼ�, ���н� �ڵ� �����ϵ��� �Ѵ�.
			CComPtr<IUnknown> spUnk;
			hr = pRow->QueryInterface(&spUnk);
			if(FAILED(hr))
			{
				delete pRow; // �������� �ʾұ� ������ �������� �����.
				return hr;
			}

			// Command object�� IUnknown�� Row�� Site�� �����Ѵ�.
			CComPtr<IUnknown> spOuterUnk;
			QueryInterface(__uuidof(IUnknown), (void **)&spOuterUnk);
			pRow->m_contained.SetSite(spOuterUnk, CCUBRIDRow::FromRow);
			pRow->m_contained.Initialize(m_hReq);

			//������ Row ��ü�� IRow �������̽� ��ȯ
			hr = pRow->QueryInterface(riid, (void **)ppUnk);
			if(FAILED(hr))
				return hr;

			/*
			if (m_eType == FromRowset)
			{
				CCUBRIDRowset* pRowset = GetRowsetPtr();
				CCUBRIDRowsetRow *pRow;
				if( !pRowset->m_rgRowHandles.Lookup((CCUBRIDRowsetRow::KeyType)m_hRow, pRow) )
					return DB_E_BADROWHANDLE;
			}
			*/
		} else //if (rguidColumnType == DBGUID_ROWSET)
		{
			CCUBRIDRowRowset* pRowset = NULL;
			LONG pcRowsAffected;
			
			//CCUBRIDRowRowset ����� ComPolyObject ����
			CComPolyObject<CCUBRIDRowRowset>* pPolyObj; 
			if (FAILED(hr = CComPolyObject<CCUBRIDRowRowset>::CreateInstance(pUnkOuter, &pPolyObj)))
				return hr;

			// ������ COM ��ü�� �����ؼ�, ���н� �ڵ� �����ϵ��� �Ѵ�.
			CComPtr<IUnknown> spUnk;
			hr = pPolyObj->QueryInterface(&spUnk);
			if (FAILED(hr))
			{
				delete pPolyObj; // �������� �ʾұ� ������ �������� �����
				return hr;
			}
			//CCUBRIDRowRowset ��ü�� ���� �����͸� �����Ѵ�
			pRowset = &(pPolyObj->m_contained);
			
			//CCUBRIDRowRowset ��ü �ʱ�ȭ
			if (FAILED(hr = pRowset->FInit()))
				return hr;
			
			strcpy(pRowset->m_szOID, m_szOID);
			wcscpy(pRowset->m_colName, colName);
			pRowset->m_colIndex = col_index;

			//Row�� Site�� �����Ѵ�.
			CComPtr<IUnknown> spOuterUnk;
			this->QueryInterface(__uuidof(IUnknown), (void**)&spOuterUnk);
			pRowset->SetSite(spOuterUnk);
			
			// Check to make sure we set any 'post' properties based on the riid
			// requested.  Do this before calling Execute in case provider has
			// property specific processing.
			if (FAILED(pRowset->OnInterfaceRequested(riid)))
				return hr;

			pRowset->QueryInterface(riid, (void **)ppUnk);
			hr = pRowset->Execute(&pcRowsAffected, 0, NULL);
		} 
	}

	return hr;
}

STDMETHODIMP CCUBRIDRow::GetSession(REFIID riid, IUnknown **ppSession)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRow::GetSession\n");
	ClearError();

	if(!m_bIsValid) return RaiseError(E_UNEXPECTED, 0, __uuidof(IGetSession));

	if(ppSession)
		*ppSession = 0;
	else
		return E_INVALIDARG;

	CCUBRIDSession *pSession = GetSessionPtr();
	return pSession->QueryInterface(riid, (void **)ppSession);
}
