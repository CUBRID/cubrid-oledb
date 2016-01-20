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

#pragma once

class CCUBRIDRowsetRowColumn;
class CCUBRIDRowset;

/*
 * Storage�� �� Row�� Local Copy �Ǵ� Deferred Update �����͸� �����صδ� Ŭ����.
 *
 * m_rgColumns: BOOKMARK �÷� ����. Storage ���� ������ ����
 * m_iRowset: 0���� ����. Storage ���� ��ġ.
 */
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
	UINT m_uCodepage;
	CCUBRIDRowsetRowColumn *m_rgColumns;
	DBORDINAL m_cColumns;
	ATLCOLUMNINFO *m_pInfo;
	CComPtr<IDataConvert> m_spConvert;
	CAtlArray<CStringA>* m_defaultVal;
	INT m_con;

private:
	// m_rgColumns �޸𸮸� ����
	void FreeData();
public:
	CCUBRIDRowsetRow(UINT uCodepage, DBCOUNTITEM iRowset, DBORDINAL cCols, ATLCOLUMNINFO *pInfo, 
				CComPtr<IDataConvert> &spConvert, CAtlArray<CStringA>* defaultVal = NULL, INT con = -1)
		: m_uCodepage(uCodepage), m_dwRef(0), m_rgColumns(0), m_status(0), m_iRowset(iRowset),
		  m_iOriginalRowset(iRowset), m_cColumns(cCols), m_pInfo(pInfo), m_defaultVal(defaultVal),
		  m_spConvert(spConvert), m_con(con)
	{
		ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRow::CCUBRIDRowsetRow\n");
		m_szOID[0] = NULL;
	}

	~CCUBRIDRowsetRow()
	{
		ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRow::~CCUBRIDRowsetRow\n");
		FreeData();
	}

	DWORD AddRefRow() { return CComObjectThreadModel::Increment((LPLONG)&m_dwRef); } 
	DWORD ReleaseRow() { return CComObjectThreadModel::Decrement((LPLONG)&m_dwRef); }

	HRESULT Compare(CCUBRIDRowsetRow *pRow)
	{
		ATLASSERT(pRow);
		return ( m_iRowset==pRow->m_iRowset ? S_OK : S_FALSE );
	}

	//===== ReadData: �ٸ� ���� �����͸� �� Ŭ���� ������ �о���δ�.
public:
	// hReq�� ���� �����͸� �о����
	HRESULT ReadData(int hReq, bool bOIDOnly=false, bool bSensitive=false);
	// pBinding�� ���� �̷���� pData�� ���� �����͸� �о����
	HRESULT ReadData(ATLBINDINGS *pBinding, void *pData, UINT uCodepage);
	HRESULT ReadData(int hReq, char* szOID);

	//===== WriteData: �� Ŭ������ �����͸� �ٸ� ���� �����Ѵ�.
public:
	// m_rgColumns�� �����͸� storage�� ����
	HRESULT WriteData(int hConn, UINT uCodepage, int hReq, CComBSTR &strTableName);
	// m_rgColumns�� �����͸� pBinding�� ���� pData�� ����
	HRESULT WriteData(ATLBINDINGS *pBinding, void *pData, DBROWCOUNT dwBookmark, CCUBRIDRowset* pRowset = NULL);
	// m_rgColumns�� �����͸� rgColumns�� ����
	HRESULT WriteData(DBORDINAL cColumns, DBCOLUMNACCESS rgColumns[]);

	//===== Compare: �� Ŭ������ �����Ͱ� ���ǿ� �����ϴ� �� �˻�
public:
	// ���� row�� rBinding�� ���ǿ� �´��� �˻�
	HRESULT Compare(void *pFindData, DBCOMPAREOP CompareOp, DBBINDING &rBinding);
};
