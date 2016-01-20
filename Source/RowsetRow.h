////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

class CCUBRIDRowsetRowColumn;
class CCUBRIDRowset;

class CCUBRIDRowsetRow
{
public:
	typedef DBCOUNTITEM KeyType;
	DWORD m_dwRef;
	DBPENDINGSTATUS m_status;
	KeyType m_iRowset;
	KeyType m_iOriginalRowset; // not used
	char m_szOID[32]; // OID of Row
private:
	CCUBRIDRowsetRowColumn *m_rgColumns;
	DBORDINAL m_cColumns;
	ATLCOLUMNINFO *m_pInfo;
	CComPtr<IDataConvert> m_spConvert;
	CAtlArray<CStringA>* m_defaultVal;

private:
	void FreeData();
public:
	CCUBRIDRowsetRow(DBCOUNTITEM iRowset, DBORDINAL cCols, ATLCOLUMNINFO *pInfo,
		CComPtr<IDataConvert> &spConvert, CAtlArray<CStringA>* defaultVal = NULL)
		: m_dwRef(0), m_rgColumns(0), m_status(0), m_iRowset(iRowset),
		m_iOriginalRowset(iRowset), m_cColumns(cCols), m_pInfo(pInfo), m_defaultVal(defaultVal),
		m_spConvert(spConvert)
	{
		ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRow::CCUBRIDRowsetRow\n");
		m_szOID[0] = NULL;
	}

	~CCUBRIDRowsetRow()
	{
		ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRow::~CCUBRIDRowsetRow\n");
		FreeData();
	}

	DWORD AddRefRow() {
		return CComObjectThreadModel::Increment((LPLONG)&m_dwRef);
	}
	DWORD ReleaseRow() {
		return CComObjectThreadModel::Decrement((LPLONG)&m_dwRef);
	}

	HRESULT Compare(CCUBRIDRowsetRow *pRow)
	{
		ATLASSERT(pRow);

		return ( m_iRowset == pRow->m_iRowset ? S_OK : S_FALSE );
	}

public:
	HRESULT ReadData(int hReq, bool bOIDOnly = false, bool bSensitive = false);
	HRESULT ReadData(ATLBINDINGS *pBinding, void *pData);
	HRESULT ReadData(int hReq, char* szOID);

public:
	HRESULT WriteData(int hConn, int hReq, CComBSTR &strTableName, ATLBINDINGS *pBinding, void *pData);
	HRESULT WriteData(ATLBINDINGS *pBinding, void *pData, DBROWCOUNT dwBookmark, CCUBRIDRowset* pRowset = NULL);
	HRESULT WriteData(DBORDINAL cColumns, DBCOLUMNACCESS rgColumns[]);

public:
	HRESULT Compare(void *pFindData, DBCOMPAREOP CompareOp, DBBINDING &rBinding);
};
