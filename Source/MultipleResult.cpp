////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// MultipleResult.cpp : Implementation of CMultipleResult

#include "stdafx.h"
#include "MultipleResult.h"
#include "Rowset.h"
#include "Session.h"
#include "Row.h"
#include "Error.h"
#include "CUBRIDStream.h"

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
	if(pOwner != this)
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

	if (!(lResultFlag == DBRESULTFLAG_DEFAULT || lResultFlag == DBRESULTFLAG_ROWSET
		|| lResultFlag == DBRESULTFLAG_ROW))
	{
		hr = E_INVALIDARG;
		goto error;
	}

	if (lResultFlag == DBRESULTFLAG_ROWSET)
	{
		if (riid == IID_IRow)
			hr = E_NOINTERFACE;
	}
	else if (lResultFlag == DBRESULTFLAG_ROW)
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

	if (m_command->m_cRowsetsOpen > 0)
	{
		hr = DB_E_OBJECTOPEN;
		goto error;
	}

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
	m_resultIndex++;
	cmd = m_command;

	if (riid == IID_IRow)
	{
		if (ppRowset && (cmd_type == CUBRID_STMT_SELECT ||
			cmd_type == CUBRID_STMT_GET_STATS ||
			cmd_type == CUBRID_STMT_CALL ||
			cmd_type == CUBRID_STMT_EVALUATE))
		{
			CComPolyObject<CCUBRIDRow> *pRow;
			HRESULT hr = CComPolyObject<CCUBRIDRow>::CreateInstance(pUnkOuter, &pRow);
			if(FAILED(hr)) goto error;

			CComPtr<IUnknown> spUnk;
			hr = pRow->QueryInterface(&spUnk);
			if(FAILED(hr))
			{
				delete pRow;
				goto error;
			}

			CComPtr<IUnknown> spOuterUnk;
			GetCommandPtr()->QueryInterface(__uuidof(IUnknown), (void **)&spOuterUnk);
			//hr = pRow->m_contained.SetSite(spOuterUnk,  CCUBRIDRow::Type::FromCommand);
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

			hr = pRow->QueryInterface(riid, (void **)ppRowset);
			if(FAILED(hr)) goto error;

			//if (result_count > 1)
			//	return DB_S_NOTSINGLETON;
		}
		else
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

			if (m_resultIndex > m_numQuery)
			{
				GetSessionPtr()->AutoCommit(this);
				m_hReq = 0;
				m_bCommitted = true;
			}

			if (ppRowset != NULL)
				*ppRowset = NULL;
		}
	}
	else
	{
		if(cmd_type == CUBRID_STMT_SELECT ||
			cmd_type == CUBRID_STMT_GET_STATS ||
			cmd_type == CUBRID_STMT_CALL ||
			cmd_type == CUBRID_STMT_EVALUATE)
		{
			if (ppRowset)
			{
				if (riid != IID_NULL)
				{
					ATLTRACE2(atlTraceDBProvider, 2, _T("CMultipleResult::CreateRowset\n"));

					hr = cmd->CreateRowset<CCUBRIDRowset>(pUnkOuter, riid, m_pParams, pcRowsAffected, ppRowset, pRowset);
					if (FAILED(hr))	goto error;

					pRowset->InitFromCommand(m_hReq, result_count);
				} else
					*ppRowset = NULL;
			}
		} else
		{
			if (pcRowsAffected)
				*pcRowsAffected = result_count;

			if (m_resultIndex > m_numQuery && m_numQuery > 1)
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
		return RaiseError(hr, 1, __uuidof(IMultipleResults), error.err_msg);
	else
		return RaiseError(hr, 0, __uuidof(IMultipleResults));
}
