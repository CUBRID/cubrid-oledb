////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

void show_error(char *msg, int code, T_CCI_ERROR *error);

namespace Util
{
	//DBROWCOUNT FindBookmark(const CAtlArray<DBROWCOUNT> &rgBookmarks, DBROWCOUNT iRowset);

	HRESULT Connect(IDBProperties *pDBProps, int *phConn);
	HRESULT Disconnect(int *phConn);
	HRESULT DoesTableExist(int hConn, char *szTableName);
	HRESULT OpenTable(int hConn, const CComBSTR &strTableName, int *phReq, int *pcResult, char flag, bool bAsynch = false, int maxrows = 0);
	HRESULT GetUniqueTableName(CComBSTR& strTableName);
	HRESULT GetTableNames(int hConn, CAtlArray<CStringA> &rgTableNames);
	HRESULT GetIndexNamesInTable(int hConn, char* table_name, CAtlArray<CStringA> &rgIndexNames, CAtlArray<int> &rgIndexTypes);
	void ExtractTableName(const CComBSTR &strCommandText, CComBSTR &strTableName);
	bool RequestedRIIDNeedsUpdatability(REFIID riid);
	//bool RequestedRIIDNeedsOID(REFIID riid);
	//bool CheckOIDFromProperties(ULONG cSets, const DBPROPSET rgSets[]);
	bool CheckUpdatabilityFromProperties(ULONG cSets, const DBPROPSET rgSets[]);

	class CColumnsInfo
	{
	public:
		int m_cColumns;
		ATLCOLUMNINFO *m_pInfo;
		CAtlArray<CStringA>* m_defaultVal;

		CColumnsInfo() : m_cColumns(0), m_pInfo(0), m_defaultVal(0) {}
		~CColumnsInfo() {
			FreeColumnInfo();
		}

		HRESULT GetColumnInfo(T_CCI_COL_INFO* info, T_CCI_CUBRID_STMT cmd_type, int cCol, bool bBookmarks = false, ULONG ulMaxLen = 0);
		HRESULT GetColumnInfo(int hReq, bool bBookmarks = false, ULONG ulMaxLen = 0);
		HRESULT GetColumnInfoCommon(T_CCI_COL_INFO* info, T_CCI_CUBRID_STMT cmd_type, bool bBookmarks = false, ULONG ulMaxLen = 0);

		void FreeColumnInfo();
	};

	class ITxnCallback
	{
	public:
		virtual void TxnCallback(const ITxnCallback *pOwner) = 0;
	};

}
