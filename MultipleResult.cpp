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

// MultipleResult.cpp : Implementation of CMultipleResult

#include "stdafx.h"
#include "MultipleResult.h"
#include "Rowset.h"
#include "Session.h"
#include "Row.h"
#include "Error.h"

CCUBRIDSession *CMultipleResult::GetSessionPtr()
{
	return GetCommandPtr()->GetSessionPtr();
}

CCUBRIDCommand *CMultipleResult::GetCommandPtr()
{
	return m_command;
}

CMultipleResult::~CMultipleResult()
{
	ATLTRACE2(atlTraceDBProvider, 2, _T("CMultipleResult::~CMultipleResult()\n"));
	this->GetSessionPtr()->AutoCommit(NULL);
	if(m_spUnkSite)
		GetSessionPtr()->RegisterTxnCallback(this, false);
}

HRESULT CMultipleResult::SetSite(IUnknown *pUnkSite)
{
	HRESULT hr = IObjectWithSiteImpl<CMultipleResult>::SetSite(pUnkSite);
	GetSessionPtr()->RegisterTxnCallback(this, true);
	return hr;
}

void CMultipleResult::TxnCallback(const ITxnCallback *pOwner)
{
	if(pOwner!=this)
	{
		cci_close_req_handle(m_hReq);
		m_hReq = 0;
		m_bInvalidated = true;
	}
}

HRESULT CMultipleResult::GetResult(IUnknown *pUnkOuter, DBRESULTFLAG lResultFlag,
            REFIID riid, DBROWCOUNT *pcRowsAffected, IUnknown **ppRowset)
{
	ATLTRACE2(atlTraceDBProvider, 2, _T("CMultipleResult::GetResult\n"));

	HRESULT hr = S_OK;
	CCUBRIDCommand* cmd = NULL;
	CCUBRIDRowset* pRowset = NULL;
	int result_count = 0, rc;
	T_CCI_CUBRID_STMT cmd_type;
	T_CCI_ERROR error;

	ClearError();
	error.err_msg[0] = 0;

	//E_INVALIDARG ó��
	if (!(lResultFlag == DBRESULTFLAG_DEFAULT || lResultFlag == DBRESULTFLAG_ROWSET
		|| lResultFlag == DBRESULTFLAG_ROW))
	{
		hr = E_INVALIDARG;
		goto error;
	}
	
	//E_NOINTERFACE ó��
	if (lResultFlag == DBRESULTFLAG_ROWSET)
	{
		if (riid == IID_IRow)
			hr = E_NOINTERFACE;
	} else if (lResultFlag == DBRESULTFLAG_ROW)
	{
		if (riid == IID_IRowset)
			hr = E_NOINTERFACE;
	}

	if ( riid == IID_IRowsetUpdate )
		hr = E_NOINTERFACE;
	else if ( riid == IID_IMultipleResults)
		hr = E_NOINTERFACE;

	if (hr == E_NOINTERFACE)
		goto error;
	
	if (pUnkOuter && riid != IID_IUnknown)
	{
		hr = DB_E_NOAGGREGATION;
		goto error;
	}

	if (pcRowsAffected)
		*pcRowsAffected = DB_COUNTUNAVAILABLE;

	//��� ������ �� ����Ǿ����� m_qr ���� �� DB_S_NORESULT ���� 
	if (m_resultIndex > m_numQuery)
	{
		ATLTRACE2(atlTraceDBProvider, 2, _T("DB_S_NORESULT\n"));

		cci_query_result_free(m_qr, m_numQuery);
		m_qr = NULL;
		if (ppRowset)
			*ppRowset = NULL;
		if (pcRowsAffected)
			*pcRowsAffected = DB_COUNTUNAVAILABLE;

		return DB_S_NORESULT;
	}

	if (m_bInvalidated)
	{
		hr = E_UNEXPECTED;
		goto error;
	}

	//Rowset�� �����ִ� ��� DB_E_OBJECTOPEN�� ����
	if (m_command->m_cRowsetsOpen > 0)
	{
		hr = DB_E_OBJECTOPEN;
		goto error;
	}

	//������ ������� �����´�
	if (m_resultIndex != 1 && !m_bCommitted)
	{

		rc = cci_next_result(m_hReq, &error);
		if (rc < 0)
		{
			hr = E_FAIL;
			goto error;
		}
	}

	result_count = CCI_QUERY_RESULT_RESULT(m_qr, m_resultIndex);
	cmd_type = (T_CCI_CUBRID_STMT)CCI_QUERY_RESULT_STMT_TYPE(m_qr, m_resultIndex);
	//���� �ε��� ����
	m_resultIndex++;

	//CCUBRIDCommand ��ü�� ���� ���۷����� �޾ƿ�
	cmd = m_command;

	if (riid == IID_IRow)
	{	
		if (ppRowset && (cmd_type==CUBRID_STMT_SELECT ||
		   cmd_type==CUBRID_STMT_GET_STATS ||
		   cmd_type==CUBRID_STMT_CALL ||
		   cmd_type==CUBRID_STMT_EVALUATE))
		{
			CComPolyObject<CCUBRIDRow> *pRow;
			HRESULT hr = CComPolyObject<CCUBRIDRow>::CreateInstance(pUnkOuter, &pRow);
			if(FAILED(hr)) goto error;

			// ������ COM ��ü�� �����ؼ�, ���н� �ڵ� �����ϵ��� �Ѵ�.
			CComPtr<IUnknown> spUnk;
			hr = pRow->QueryInterface(&spUnk);
			if(FAILED(hr))
			{
				delete pRow; // �������� �ʾұ� ������ �������� �����.
				goto error;
			}

			//Command object�� IUnknown�� Row�� Site�� �����Ѵ�.
			CComPtr<IUnknown> spOuterUnk;
			GetCommandPtr()->QueryInterface(__uuidof(IUnknown), (void **)&spOuterUnk);
			hr = pRow->m_contained.SetSite(spOuterUnk,  CCUBRIDRow::FromCommand);
			if(FAILED(hr)) goto error;
			hr = pRow->m_contained.Initialize(m_hReq);

			if (FAILED(hr))
			{
				if (ppRowset)
					*ppRowset = NULL;
				if (pcRowsAffected)
					*pcRowsAffected = DB_COUNTUNAVAILABLE;

				return RaiseError(hr, 0, __uuidof(IMultipleResults));
			}
			//������ Row ��ü�� IRow �������̽� ��ȯ
			hr = pRow->QueryInterface(riid, (void **)ppRowset);
			if(FAILED(hr)) goto error;
			
			//if (result_count > 1)
			//	return DB_S_NOTSINGLETON;
		} else
		{
			if (cmd_type == CUBRID_STMT_SELECT)
			{
				if (pcRowsAffected)
					*pcRowsAffected = -1;
			}
			else
			{
				if (pcRowsAffected)
					*pcRowsAffected = result_count;
			}

			if (m_resultIndex > m_numQuery) //������ ������ ���
			{
				GetSessionPtr()->AutoCommit(this);
				m_hReq = 0;
				m_bCommitted = true;
			}

			if (ppRowset != NULL)
				*ppRowset = NULL;
		}
	} 
	//IID_IRowset�� �ƴ� ��� Rowset�� ����
	//IID_IRow ó�� �ٸ��� ó���� �־�� �ϴ� ��찡 �� ������?
	else
	{	
		if(cmd_type==CUBRID_STMT_SELECT ||
		   cmd_type==CUBRID_STMT_GET_STATS ||
		   cmd_type==CUBRID_STMT_CALL ||
		   cmd_type==CUBRID_STMT_EVALUATE)
		{
			if (ppRowset)
			{
				if (riid != IID_NULL)
				{
					ATLTRACE2(atlTraceDBProvider, 2, _T("CMultipleResult::CreateRowset\n"));

					//Rowset object ����
					hr = cmd->CreateRowset<CCUBRIDRowset>(pUnkOuter, riid, m_pParams, pcRowsAffected, ppRowset, pRowset);
					if (FAILED(hr))	goto error;

					pRowset->InitFromCommand(m_hReq, m_uCodepage, result_count);
				} else
					*ppRowset = NULL;
			}
		} else
		{
			if (pcRowsAffected)
				*pcRowsAffected = result_count;

			//Ŀ���� ���� �����Ƿ� ������ �������� ���� ������ ������Ʈ ������ �� �� ����!!
			//������ ������ ���� Ŀ��

			if (m_resultIndex > m_numQuery && m_numQuery > 1) //���Ǽ��� 2�� �̻��̸鼭 ������ ������ ��� Ŀ��
			{
				GetSessionPtr()->AutoCommit(this);
				m_hReq = 0;
				m_bCommitted = true;
			}

			*ppRowset = NULL;
		}
	} 
	

	return S_OK;

error:
	if (ppRowset)
		*ppRowset = NULL;
	if (pcRowsAffected)
		*pcRowsAffected = DB_COUNTUNAVAILABLE;
	m_hReq = 0;

	if (strlen(error.err_msg) > 0)
		return RaiseError(hr, 1, __uuidof(IMultipleResults), CA2W(error.err_msg, m_uCodepage));
	else
		return RaiseError(hr, 0, __uuidof(IMultipleResults));
}