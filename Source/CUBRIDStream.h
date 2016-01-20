////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "row.h"

// CCUBRIDStream
[
	coclass,
	noncreatable,
	uuid("857539EA-0140-40be-A8E5-1F347991CC0D"),
	threading(apartment),
	registration_script("none")
]

class ATL_NO_VTABLE CCUBRIDStream :
	public ISequentialStream,
	public CConvertHelper
{
public:

private:
	int				m_hConn;
	int				m_hReq;
	char			m_OID[32];
	int				m_colIndex;
	LPOLESTR		m_colName;
	T_CCI_U_TYPE	m_colType;
	int				m_colPrecision;
	int				m_colScale;
	ULONG			m_curPos;
	char*			m_writeBuf;
	T_CCI_BLOB		blob;
	T_CCI_CLOB		clob;

public:
	CCUBRIDStream(void) : m_curPos(0), m_hConn(-1), m_hReq(0), m_colIndex(0), m_colPrecision(0), m_colScale(0)
	{
		ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDStream::CCUBRIDStream\n");
		m_writeBuf = NULL;
		m_OID[0] = NULL;
	}

	~CCUBRIDStream(void)
	{
		ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDStream::~CCUBRIDStream\n");
		if (m_hReq)
			cci_close_req_handle(m_hReq);
	}

	HRESULT FinalConstruct()
	{
		HRESULT hr = CConvertHelper::FinalConstruct();
		if (FAILED (hr))
			return hr;
		return S_OK;
	}

	void Initialize(int hConn, char* cur_oid, T_CCI_COL_INFO info);
	void Initialize(int hConn, char* cur_oid, const ATLCOLUMNINFO* info);
	void Initialize(int hConn, int req,  const T_CCI_COL_INFO info);
	T_CCI_U_TYPE GetType();
	T_CCI_BLOB GetBlob();
	T_CCI_CLOB GetClob();

	//ISequentialStream
	STDMETHOD(Read)(void *pv, ULONG cb, ULONG *pcbRead);
	STDMETHOD(Write)(const void *pv, ULONG cb, ULONG *pcbWritten);
};
