////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Command.h"

[
	coclass,
	noncreatable,
	uuid("BD659D91-36B5-4e4a-BE76-E979AEB132C3"),
	threading(apartment),
	registration_script("none"),
	support_error_info(IMultipleResults)
]
class ATL_NO_VTABLE CMultipleResult :
	public IObjectWithSiteImpl<CMultipleResult>,
	public IMultipleResults,
	public Util::ITxnCallback
{
public:
	CCUBRIDSession *GetSessionPtr();
	CCUBRIDCommand *GetCommandPtr();

public:
	CCUBRIDCommand*		m_command;
	int					m_hReq;
	bool				m_bInvalidated;
	DBPARAMS*			m_pParams;

	T_CCI_QUERY_RESULT*	m_qr;
	int					m_resultIndex;
	int					m_numQuery;
	bool				m_bCommitted;

	CMultipleResult() : m_numQuery(0), m_qr(NULL), m_pParams(NULL),
		m_resultIndex(1), m_hReq(0), m_bInvalidated(false), m_command(NULL), m_bCommitted(false)
	{
	}

	~CMultipleResult();

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

	void Initialize(CCUBRIDCommand* command, T_CCI_QUERY_RESULT* qr, DBPARAMS* pParams, int numQuery, bool bCommitted)
	{
		m_command = command;
		m_hReq = m_command->m_hReq;
		m_qr = qr;
		m_pParams = pParams;
		m_numQuery = numQuery;
		m_bCommitted = bCommitted;
	}

	STDMETHOD(SetSite)(IUnknown *pUnkSite);
	virtual void TxnCallback(const ITxnCallback *pOwner);

	STDMETHOD(GetResult)(IUnknown *pUnkOuter, DBRESULTFLAG lResultFlag,
		REFIID riid, DBROWCOUNT *pcRowsAffected, IUnknown **ppRowset);
};
