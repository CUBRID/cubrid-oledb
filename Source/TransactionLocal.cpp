////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Session.h"
#include "CUBRIDStream.h"

HRESULT CCUBRIDSession::DoCASCCICommit(bool bCommit)
{
	int hConn;

	HRESULT hr = GetConnectionHandle(&hConn);
	if(FAILED(hr))
		return E_FAIL;

	T_CCI_ERROR err_buf;
	int rc = cci_end_tran(hConn, bCommit ? CCI_TRAN_COMMIT : CCI_TRAN_ROLLBACK, &err_buf);
	if(rc < 0)
		return bCommit ? XACT_E_COMMITFAILED : E_FAIL;

	POSITION pos = m_grpTxnCallbacks.GetHeadPosition();
	while(pos)
		m_grpTxnCallbacks.GetNext(pos)->TxnCallback(0);

	return S_OK;
}

HRESULT CCUBRIDSession::AutoCommit(const Util::ITxnCallback *pOwner)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::AutoCommit\n");

	if(m_bAutoCommit)
	{
		if(pOwner)
		{
			POSITION pos = m_grpTxnCallbacks.GetHeadPosition();
			while(pos)
				m_grpTxnCallbacks.GetNext(pos)->TxnCallback(pOwner);

			//HRESULT hr = DoCASCCICommit(true);
			//if(FAILED(hr)) return hr;
			//return S_OK;
		}
		else
		{
			DoCASCCICommit(true);
		}
	}

	return S_OK;
}

HRESULT CCUBRIDSession::RowsetCommit()
{
	return DoCASCCICommit(true);
}

HRESULT CCUBRIDSession::SetCASCCIIsoLevel(ISOLEVEL isoLevel, bool bCheckOnly)
{
	int hConn;

	if(!bCheckOnly)
	{
		HRESULT hr = GetConnectionHandle(&hConn);
		if(FAILED(hr))
			return E_FAIL;
	}

	int cci_isolevel;

	switch(isoLevel)
	{
	case ISOLATIONLEVEL_READUNCOMMITTED:
		cci_isolevel = TRAN_COMMIT_CLASS_UNCOMMIT_INSTANCE;
		break;
	case ISOLATIONLEVEL_READCOMMITTED:
		cci_isolevel = TRAN_COMMIT_CLASS_COMMIT_INSTANCE;
		break;
	case ISOLATIONLEVEL_REPEATABLEREAD:
		cci_isolevel = TRAN_REP_CLASS_REP_INSTANCE;
		break;
	case ISOLATIONLEVEL_SERIALIZABLE:
		cci_isolevel = TRAN_SERIALIZABLE;
		break;
	default:
		return XACT_E_ISOLATIONLEVEL;
	}

	if(!bCheckOnly)
	{
		T_CCI_ERROR err_buf;
		int rc = cci_set_db_parameter(hConn, CCI_PARAM_ISOLATION_LEVEL, &cci_isolevel, &err_buf);
		if(rc < 0)
			return E_FAIL;
		m_isoLevel = isoLevel;
	}

	ATLTRACE(atlTraceDBProvider, 2, "Current Isolation Level:[%ld]\n", isoLevel);

	return S_OK;
}

HRESULT CCUBRIDSession::EnterAutoCommitMode()
{
	g_autocommit = m_bAutoCommit = true;

	CComVariant var;
	GetPropValue(&DBPROPSET_SESSION, DBPROP_SESS_AUTOCOMMITISOLEVELS, &var);
	SetCASCCIIsoLevel(V_I4(&var));

	return S_OK;
}

STDMETHODIMP CCUBRIDSession::GetOptionsObject(ITransactionOptions **ppOptions)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::GetOptionsObject\n");

	return DB_E_NOTSUPPORTED;
}

STDMETHODIMP CCUBRIDSession::StartTransaction(ISOLEVEL isoLevel, ULONG isoFlags,
																							ITransactionOptions *pOtherOptions, ULONG *pulTransactionLevel)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::StartTransaction\n");

	//TODO Document that we do not support nested transactions!
	if(!m_bAutoCommit)
	{
		ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::StartTransaction - Autocommit is OFF, Can't start nested transactions!\n");

		return XACT_E_XTIONEXISTS;
	}

	if(isoFlags != 0)
		return XACT_E_NOISORETAIN;

	HRESULT hr = SetCASCCIIsoLevel(isoLevel);
	if(FAILED(hr))
		return hr;

	//TODO Document this
	//If AutoCommit is ON, let's turn it OFF
	CCI_AUTOCOMMIT_MODE mode;
	if(GetAutoCommitMode(&mode) != S_OK)
	{
		ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::StartTransaction - Getting AUTOCOMMIT mode failed!\n");
		return E_FAIL;
	}
	if(mode == CCI_AUTOCOMMIT_TRUE && SetAutoCommitMode(CCI_AUTOCOMMIT_FALSE) != S_OK)
	{
		ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::StartTransaction - Switching AUTOCOMMIT to OFF failed!\n");
		return E_FAIL;
	}

	if(pulTransactionLevel)
		*pulTransactionLevel = 1;

	g_autocommit = m_bAutoCommit = false;

	return S_OK;
}

STDMETHODIMP CCUBRIDSession::Commit(BOOL fRetaining, DWORD grfTC, DWORD grfRM)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::Commit\n");

	if(grfTC == XACTTC_ASYNC_PHASEONE || grfTC == XACTTC_SYNC_PHASEONE || grfRM != 0)
		return XACT_E_NOTSUPPORTED;

	if(m_bAutoCommit)
	{
		ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::Commit - No need for Commit when Autocommit is ON!\n");

		return XACT_E_NOTRANSACTION;
	}

	HRESULT hr = DoCASCCICommit(true);
	if(FAILED(hr))
		return hr;

	if(!fRetaining)
	{
		//If AutoCommit is OFF, let's switch it ON
		CCI_AUTOCOMMIT_MODE mode;
		if(GetAutoCommitMode(&mode) != S_OK)
		{
			ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::Commit - Getting AUTOCOMMIT mode failed!\n");
			return E_FAIL;
		}
		if(mode == CCI_AUTOCOMMIT_FALSE && SetAutoCommitMode(CCI_AUTOCOMMIT_TRUE) != S_OK)
		{
			ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::Commit - Switching AUTOCOMMIT to ON failed!\n");
			return E_FAIL;
		}

		g_autocommit = m_bAutoCommit = true; //EnterAutoCommitMode();
	}

	return S_OK;
}

STDMETHODIMP CCUBRIDSession::Abort(BOID *pboidReason, BOOL fRetaining, BOOL fAsync)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::Abort\n");

	if(fAsync)
		return XACT_E_NOTSUPPORTED;
	if(m_bAutoCommit)
		return XACT_E_NOTRANSACTION;

	HRESULT hr = DoCASCCICommit(false);
	if(FAILED(hr))
		return hr;

	if(!fRetaining)
	{
		//If AutoCommit is OFF, let's switch it ON
		CCI_AUTOCOMMIT_MODE mode;
		if(GetAutoCommitMode(&mode) != S_OK)
		{
			ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::Abort - Getting AUTOCOMMIT mode failed!\n");
			return E_FAIL;
		}
		if(mode == CCI_AUTOCOMMIT_FALSE && SetAutoCommitMode(CCI_AUTOCOMMIT_TRUE) != S_OK)
		{
			ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::Abort - Switching AUTOCOMMIT to ON failed!\n");
			return E_FAIL;
		}

		g_autocommit = m_bAutoCommit = true; //EnterAutoCommitMode();
	}

	return S_OK;
}

STDMETHODIMP CCUBRIDSession::GetTransactionInfo(XACTTRANSINFO *pinfo)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::GetTransactionInfo\n");

	if(!pinfo)
		return E_INVALIDARG;
	if(m_bAutoCommit)
		return XACT_E_NOTRANSACTION;

	int hConn;
	HRESULT hr = GetConnectionHandle(&hConn);
	if(FAILED(hr))
		return E_FAIL;

	memset(pinfo, 0, sizeof(*pinfo));
	memcpy(&pinfo->uow, &hConn, sizeof(int));
	pinfo->isoLevel = m_isoLevel;
	pinfo->grfTCSupported = XACTTC_NONE;

	return S_OK;
}


HRESULT CCUBRIDSession::GetAutoCommitMode(CCI_AUTOCOMMIT_MODE *mode)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::GetAutoCommitMode\n");

	int conn_handle;
	HRESULT hr = GetConnectionHandle(&conn_handle);
	if(FAILED(hr))
		return S_FALSE;

	//if(g_autocommit)
	//{
	//	*mode = CCI_AUTOCOMMIT_MODE::CCI_AUTOCOMMIT_TRUE;
	//}
	//else
	//{
	//	*mode = CCI_AUTOCOMMIT_MODE::CCI_AUTOCOMMIT_FALSE;
	//}

	*mode = cci_get_autocommit (conn_handle);

	return S_OK;
}

HRESULT CCUBRIDSession::SetAutoCommitMode(CCI_AUTOCOMMIT_MODE mode)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDSession::SetAutoCommitMode: %d\n", mode);

	int conn_handle;
	HRESULT hr = GetConnectionHandle(&conn_handle);
	if(FAILED(hr))
		return S_FALSE;

	int res = cci_set_autocommit(conn_handle, mode);
	if(res != 0)
	{
		return E_FAIL;
	}

	if(mode == CCI_AUTOCOMMIT_TRUE)
	{
		g_autocommit = m_bAutoCommit = true;
	}
	else
	{
		g_autocommit = m_bAutoCommit = false;
	}

	return S_OK;
}
