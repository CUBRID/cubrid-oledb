////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Rowset.cpp : Implementation of CCUBRIDCommand

#include "stdafx.h"
#include "Rowset.h"
#include "Row.h"
#include "DataSource.h"
#include "Error.h"
#include "CUBRIDStream.h"

CCUBRIDDataSource *CCUBRIDRowset::GetDataSourcePtr()
{
	//return GetSessionPtr()->GetDataSourcePtr();
	CCUBRIDDataSource *dsPtr = GetSessionPtr()->GetDataSourcePtr();

	return dsPtr;
}

CCUBRIDSession *CCUBRIDRowset::GetSessionPtr()
{
	CCUBRIDSession *sessionPtr = NULL;

	switch(m_eType)
	{
	case FromSession:
		sessionPtr = CCUBRIDSession::GetSessionPtr(this);

		return sessionPtr;
	case FromCommand:
		sessionPtr = GetCommandPtr()->GetSessionPtr();

		return sessionPtr;
	case FromRow:
		return NULL; // TODO
	default: // Invalid
		return NULL;
	}
}

CCUBRIDCommand *CCUBRIDRowset::GetCommandPtr()
{
	CCUBRIDCommand *cmdPtr = CCUBRIDCommand::GetCommandPtr(this);

	return cmdPtr;
}

CCUBRIDRowset *CCUBRIDRowset::GetRowsetPtr(IObjectWithSite *pSite)
{
	CComPtr<IRowset> spCom;
	HRESULT hr = pSite->GetSite(__uuidof(IRowset), (void **)&spCom);

	ATLASSERT(SUCCEEDED(hr));

	return static_cast<CCUBRIDRowset *>((IRowset *)spCom);
}

HRESULT CCUBRIDRowset::GetConnectionHandle(int *phConn)
{
	return GetSessionPtr()->GetConnectionHandle(phConn);
}

int CCUBRIDRowset::GetRequestHandle()
{
	switch(m_eType)
	{
	case FromSession:
		return m_hReq;
	case FromCommand:
		CCUBRIDCommand *cmdPtr;
		int hReq;

		cmdPtr = GetCommandPtr();
		hReq = cmdPtr->GetRequestHandle();
		return hReq;
	case FromRow:
		return 0; // TODO
	default: // Invalid
		ATLTRACE(atlTraceDBProvider, 1, "CCUBRIDRowset::GetRequestHandle - Invalid value\n");
		return 0;
	}
}

CCUBRIDRowset::CCUBRIDRowset()
	: m_eType(Invalid), m_hReq(0), m_bAsynch(false), m_nStatus(0), m_bFindForward(true)
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowset::CCUBRIDRowset\n");
}

CCUBRIDRowset::~CCUBRIDRowset()
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowset::~CCUBRIDRowset\n");

	CCUBRIDSession *pSession = GetSessionPtr();
	if(pSession)
	{
		//pSession->RowsetCommit();
#if 1
		if(m_bIsChangeable)
			pSession->RowsetCommit();
#endif
		pSession->RegisterTxnCallback(this, false);
	}
}

HRESULT CCUBRIDRowset::ValidateCommandID(DBID *pTableID, DBID *pIndexID)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::ValidateCommandID\n");

	HRESULT hr = _RowsetBaseClass::ValidateCommandID(pTableID, pIndexID);
	if (hr != S_OK)
		return hr;

	if(pIndexID)
		return RaiseError(DB_E_NOINDEX, 0, __uuidof(IOpenRowset));

	if(pTableID && (pTableID->uName.pwszName == NULL || wcslen(pTableID->uName.pwszName) == 0))
		return RaiseError(DB_E_NOTABLE, 0, __uuidof(IOpenRowset));

	return S_OK;
}

static HRESULT SetFetchSize(CCUBRIDRowset *pRowset)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::SetFetchSize\n");

	CCUBRIDDataSource *pDS = pRowset->GetDataSourcePtr();
	CComVariant var;
	pDS->GetPropValue(&DBPROPSET_CUBRIDPROVIDER_DBINIT, DBPROP_CUBRIDPROVIDER_FETCH_SIZE, &var);

	int rc = cci_fetch_size(pRowset->GetRequestHandle(), V_I4(&var));
	if(rc < 0)
		RaiseError(E_FAIL, 1, __uuidof(IRowset), "Failed to set the fetch size");

	return S_OK;
}

HRESULT CCUBRIDRowset::InitCommon(int resultCount, bool bRegist)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::InitCommon\n");

	m_rgRowData.SetCount(resultCount);
	m_rgBookmarks.SetCount(resultCount + 3);
	m_rgBookmarks[0] = m_rgBookmarks[1] = m_rgBookmarks[2] = -1;
	for(int i = 3; i < resultCount + 3; i++)
	{
		m_rgBookmarks[i] = i - 2;
	}

	HRESULT hr = SetFetchSize(this);
	if(FAILED(hr))
		return hr;

	if(bRegist)
		GetSessionPtr()->RegisterTxnCallback(this, true);

	return S_OK;
}

HRESULT CCUBRIDRowset::InitFromSession(DBID *pTID, char flag)
{
	int hConn;
	{
		HRESULT hr = CCUBRIDSession::GetSessionPtr(this)->GetConnectionHandle(&hConn);
		if(FAILED(hr))
			return RaiseError(E_FAIL, 0, __uuidof(IOpenRowset));
	}

	/*
	CComVariant var;
	GetPropValue(&DBPROPSET_ROWSET, DBPROP_ROWSET_ASYNCH, &var);
	ATLASSERT(V_VT(&var)==VT_I4);
	m_bAsynch = ( (V_I4(&var)&DBPROPVAL_ASYNCH_POPULATEONDEMAND) != 0 );
	*/

	int hReq, cResult;
	{
		CComVariant var;
		GetPropValue(&DBPROPSET_ROWSET, DBPROP_MAXROWS, &var);

		HRESULT hr = Util::OpenTable(hConn, pTID->uName.pwszName, &hReq, &cResult, flag, m_bAsynch, V_I4(&var));
		if(FAILED(hr))
			return hr;
	}

	m_hReq = hReq;
	m_eType = FromSession;
	m_strTableName = m_strCommandText;

	return InitCommon(cResult);
}

HRESULT CCUBRIDRowset::InitFromCommand(int hReq, int resultCount, bool bAsynch)
{
	/*
	m_bAsynch = bAsynch;
	*/
	m_eType = FromCommand;
	Util::ExtractTableName(m_strCommandText, m_strTableName);

	char buf[256];
	WideCharToMultiByte(CP_UTF8,
		0,
		m_strTableName.m_str,
		SysStringLen(m_strTableName.m_str),
		buf,
		sizeof(buf),
		NULL,
		NULL);
	buf[SysStringLen(m_strTableName.m_str)] = '\0';
	ATLTRACE("CCUBRIDRowset::InitFromCommand - Extracted table name is %s\n", buf);

	return InitCommon(resultCount);
}

/*
HRESULT CCUBRIDRowset::InitFromRow(int hReq, int resultCount)
{
m_eType = FromRow;
return InitCommon(hReq, resultCount);
}
*/

HRESULT CCUBRIDRowset::Reexecute()
{
	int hConn;
	{
		HRESULT hr = GetConnectionHandle(&hConn);
		if(FAILED(hr))
			return E_FAIL;
	}

	int cResult = 0;
	char flag = CCI_PREPARE_INCLUDE_OID | CCI_PREPARE_UPDATABLE;
	{
		if(m_eType == FromCommand)
		{
			int &hReq = GetCommandPtr()->m_hReq;
			cci_close_req_handle(hReq);
			hReq = 0;

			T_CCI_ERROR err_buf;
			hReq = cci_prepare(hConn, CW2A(m_strCommandText.m_str), flag, &err_buf);
			if(hReq > 0)
			{
				CComVariant var;
				GetPropValue(&DBPROPSET_ROWSET, DBPROP_MAXROWS, &var);
				//cci_set_max_row(hReq, V_I4(&var));

				cResult = cci_execute(hReq, 0, 0, &err_buf);
			}
			else
				hReq = 0;
		}
		else if(m_eType == FromSession)
		{
			CComVariant var;
			GetPropValue(&DBPROPSET_ROWSET, DBPROP_MAXROWS, &var);

			int hReq = 0;
			HRESULT hr = Util::OpenTable(hConn, m_strCommandText, &hReq, &cResult, flag, false, V_I4(&var));
			if(FAILED(hr))
				return E_FAIL;

			m_hReq = hReq;
		}
	}

	InitCommon(cResult, false);

	m_nStatus = 0;

	return S_OK;
}

void CCUBRIDRowset::TxnCallback(const ITxnCallback *pOwner)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::TxnCallback\n");

	ITxnCallback *pMyOwner = this;
	if(m_eType == FromCommand)
		pMyOwner = GetCommandPtr();

	if(pOwner != pMyOwner)
	{
		cci_close_req_handle(m_hReq);
		m_hReq = 0;
		m_nStatus = 1;
	}
	else
	{
		//m_nStatus = 2;
		m_nStatus = 0;
	}
}

STDMETHODIMP CCUBRIDRowset::AddRefAccessor(HACCESSOR hAccessor, DBREFCOUNT *pcRefCount)
{
	ClearError();
	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IAccessor), "This object is in a zombie state");

	HRESULT hr = _RowsetBaseClass::AddRefAccessor(hAccessor, pcRefCount);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IAccessor));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::CreateAccessor(DBACCESSORFLAGS dwAccessorFlags, DBCOUNTITEM cBindings,
																					 const DBBINDING rgBindings[], DBLENGTH cbRowSize, HACCESSOR *phAccessor, DBBINDSTATUS rgStatus[])
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IAccessor), "This object is in a zombie state");

	HRESULT hr = _RowsetBaseClass::CreateAccessor(dwAccessorFlags, cBindings,
		rgBindings, cbRowSize, phAccessor, rgStatus);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IAccessor));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::GetBindings(HACCESSOR hAccessor, DBACCESSORFLAGS *pdwAccessorFlags,
																				DBCOUNTITEM *pcBindings, DBBINDING **prgBindings)
{
	ClearError();
	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IAccessor), "This object is in a zombie state");

	HRESULT hr = _RowsetBaseClass::GetBindings(hAccessor, pdwAccessorFlags,
		pcBindings, prgBindings);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IAccessor));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::ReleaseAccessor(HACCESSOR hAccessor, DBREFCOUNT *pcRefCount)
{
	ClearError();

	HRESULT hr = _RowsetBaseClass::ReleaseAccessor(hAccessor, pcRefCount);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IAccessor));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::GetColumnDefaultValue(CCUBRIDRowset* pv)
{
	HRESULT hr = S_OK;
	int hConn, hReq, res;
	T_CCI_ERROR error;

	if (!pv->m_Columns.m_defaultVal)
	{
		hr = GetConnectionHandle(&hConn);
		if (FAILED(hr))
			return hr;

		res = cci_schema_info(hConn, CCI_SCH_ATTRIBUTE, CW2A(m_strTableName.m_str), NULL,
			CCI_ATTR_NAME_PATTERN_MATCH, &error);
		if (res < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IColumnsInfo));
		hReq = res;

		res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &error);
		if(pv->m_Columns.m_cColumns > 0 && res == CCI_ER_NO_MORE_DATA)
			return RaiseError(E_FAIL, 0, __uuidof(IColumnsInfo));

		if(res < 0)
			return RaiseError(E_FAIL, 0, __uuidof(IColumnsInfo));

		pv->m_Columns.m_defaultVal = new CAtlArray<CStringA>();

		while(true)
		{
			char* buffer;
			int ind;

			res = cci_fetch(hReq, &error);
			if(res < 0)
				return RaiseError(E_FAIL, 0, __uuidof(IColumnsInfo));

			res = cci_get_data(hReq, 9, CCI_A_TYPE_STR, &buffer, &ind);
			if(res < 0)
				return RaiseError(E_FAIL, 0, __uuidof(IColumnsInfo));

			if (ind == -1)
				pv->m_Columns.m_defaultVal->Add("");
			else
			{
				ATLASSERT(buffer);
				pv->m_Columns.m_defaultVal->Add(buffer);
			}
			res = cci_cursor(hReq, 1, CCI_CURSOR_CURRENT, &error);
			if(res == CCI_ER_NO_MORE_DATA)
				break;
		}

		cci_close_req_handle(hReq);
	}

	return hr;
}

ATLCOLUMNINFO* CCUBRIDRowset::GetColumnInfo(CCUBRIDRowset *pv, DBORDINAL *pcCols)
{
	if(!pv->m_Columns.m_pInfo)
	{
		CComVariant var;
		pv->GetPropValue(&DBPROPSET_ROWSET, DBPROP_BOOKMARKS, &var);
		HRESULT hr = pv->m_Columns.GetColumnInfo(pv->GetRequestHandle(),
			V_BOOL(&var) == ATL_VARIANT_TRUE,
			pv->GetDataSourcePtr()->PARAM_MAX_STRING_LENGTH);
		if(FAILED(hr))
			return NULL;

		{
			ULONG iCurSet, iCurProp;
			pv->GetIndexofPropSet(&DBPROPSET_ROWSET, &iCurSet);
			pv->GetIndexofPropIdinPropSet(iCurSet, DBPROP_FINDCOMPAREOPS, &iCurProp);

			DBPROP prop;
			prop.dwPropertyID = DBPROP_FINDCOMPAREOPS;
			prop.dwStatus = DBSTATUS_S_OK;
			prop.dwOptions = 0;
			prop.vValue.vt = VT_I4;

			for(int i = 0; i < pv->m_Columns.m_cColumns; i++)
			{
				pv->CDBIDOps::CopyDBIDs(&prop.colid, &pv->m_Columns.m_pInfo[i].columnid);
				prop.vValue.lVal = ::Type::GetFindCompareOps(pv->m_Columns.m_pInfo[i].wType);
				pv->SetProperty(iCurSet, iCurProp, &prop);
			}
		}
	}

	if(pcCols)
		*pcCols = pv->m_Columns.m_cColumns;

	return pv->m_Columns.m_pInfo;
}

STDMETHODIMP CCUBRIDRowset::GetColumnInfo(DBORDINAL *pcColumns, DBCOLUMNINFO **prgInfo,
																					OLECHAR **ppStringsBuffer)
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IColumnsInfo), "This object is in a zombie state");

	HRESULT hr = IColumnsInfoImpl<CCUBRIDRowset>::GetColumnInfo(pcColumns, prgInfo, ppStringsBuffer);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IColumnsInfo));


	return hr;
}

STDMETHODIMP CCUBRIDRowset::MapColumnIDs(DBORDINAL cColumnIDs, const DBID rgColumnIDs[],
																				 DBORDINAL rgColumns[])
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IColumnsInfo), "This object is in a zombie state");
	HRESULT hr = _RowsetBaseClass::MapColumnIDs(cColumnIDs, rgColumnIDs, rgColumns);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IColumnsInfo));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::CanConvert(DBTYPE wFromType, DBTYPE wToType, DBCONVERTFLAGS dwConvertFlags)
{
	ClearError();

	HRESULT hr = _RowsetBaseClass::CanConvert(wFromType, wToType, dwConvertFlags);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IConvertType));
	else
		return hr;
}

HRESULT CCUBRIDRowset::CreateRow(DBROWOFFSET lRowsOffset, DBCOUNTITEM &cRowsObtained, HROW *rgRows)
{
	CCUBRIDRowsetRow *pRow = NULL;
	ATLASSERT(lRowsOffset >= 0);

	CCUBRIDRowsetRow::KeyType key = lRowsOffset + 1;
	ATLASSERT(key > 0);

	bool bFound = m_rgRowHandles.Lookup(key, pRow);
	if (!bFound || pRow == NULL)
	{
		DBORDINAL cCols;
		ATLCOLUMNINFO *pInfo = GetColumnInfo(this, &cCols);
		ATLTRY(pRow = new CCUBRIDRowsetRow(lRowsOffset, cCols, pInfo, m_spConvert, m_Columns.m_defaultVal))
			if (pRow == NULL)
				return E_OUTOFMEMORY;

		bool bSensitive = false;
		{
			CComVariant var;
			GetPropValue(&DBPROPSET_ROWSET, DBPROP_OWNUPDATEDELETE, &var);
			bSensitive = (V_BOOL(&var) == VARIANT_TRUE);
		}

		//HRESULT hr = pRow->ReadData(GetRequestHandle(), false, bSensitive);
		int req = GetRequestHandle();
		HRESULT hr = pRow->ReadData(req, false, bSensitive);
		if(FAILED(hr))
		{
			delete pRow;
			pRow = NULL;
			return hr;
		}

		_ATLTRY
		{
			m_rgRowHandles.SetAt(key, pRow);
		}
		_ATLCATCH( e )
		{
			_ATLDELETEEXCEPTION( e );
			delete pRow;
			pRow = NULL;
			return E_OUTOFMEMORY;
		}
	}
	else // found pRow
	{
		if(pRow->m_status == DBPENDINGSTATUS_INVALIDROW)
			return DB_E_DELETEDROW;
	}
	pRow->AddRefRow();
	m_bReset = false;
	rgRows[cRowsObtained++] = (HROW)key;

	return S_OK;
}

STDMETHODIMP CCUBRIDRowset::AddRefRows(DBCOUNTITEM cRows, const HROW rghRows[],
																			 DBREFCOUNT rgRefCounts[], DBROWSTATUS rgRowStatus[])
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowset), "This object is in a zombie state");

	HRESULT hr = _RowsetBaseClass::AddRefRows(cRows, rghRows, rgRefCounts, rgRowStatus);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowset));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::GetData(HROW hRow, HACCESSOR hAccessor, void *pDstData)
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowset), "This object is in a zombie state");

	ATLTRACE(atlTraceDBProvider, 2, _T("IRowset::GetData\n"));

	HRESULT hr;
	CCUBRIDRowsetRow *pRow = 0;
	ATLBINDINGS *pBinding = 0;
	{
		// check arguments and prepare data
		if(hRow == NULL || !m_rgRowHandles.Lookup((ULONG)hRow, pRow) || pRow == NULL)
			return RaiseError(DB_E_BADROWHANDLE, 0, __uuidof(IRowset));
		if(!m_rgBindings.Lookup((ULONG)hAccessor, pBinding) || pBinding == NULL)
			return RaiseError(DB_E_BADACCESSORHANDLE, 0, __uuidof(IRowset));
		if(pDstData == NULL && pBinding->cBindings != 0)
			return RaiseError(E_INVALIDARG, 0, __uuidof(IRowset));
	}

	if(pRow->m_status == DBPENDINGSTATUS_INVALIDROW || pRow->m_status == DBPENDINGSTATUS_DELETED)
		return RaiseError(DB_E_DELETEDROW, 0, __uuidof(IRowset));

	//TODO Verify this
	DBROWCOUNT dwBookmark = pRow->m_iRowset + 3; //Util::FindBookmark(m_rgBookmarks, (LONG)pRow->m_iRowset+1);

	hr = pRow->WriteData(pBinding, pDstData, dwBookmark, this);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowset));
	else
		return hr;
}

HRESULT CCUBRIDRowset::GetNextRowsAsynch(HCHAPTER hReserved, DBROWOFFSET lRowsOffset,
																				 DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW **prghRows)
{
	// TODO: backward fetch
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::GetNextRowsAsynch\n");

	if(prghRows == NULL || pcRowsObtained == NULL)
		return E_INVALIDARG;
	if(cRows == 0) return S_OK;

	if(lRowsOffset < 0 && !m_bCanScrollBack)
		return DB_E_CANTSCROLLBACKWARDS;
	if(cRows < 0  && !m_bCanFetchBack)
		return DB_E_CANTFETCHBACKWARDS;

	// In the case where the user is moving backwards after moving forwards,
	// we do not wrap around to the end of the rowset.
	if(m_iRowset == 0 && !m_bReset && cRows < 0)
		return DB_S_ENDOFROWSET;

	if(lRowsOffset < 0 && m_bReset)
		return DB_E_CANTSCROLLBACKWARDS;

	int iStepSize = ( cRows >= 0 ? 1 : -1 );
	cRows = AbsVal(cRows); // if cRows==MINLONG_PTR?

	DBROWOFFSET lTmpRows = lRowsOffset;
	lRowsOffset += m_iRowset;

	CComHeapPtr<HROW> rghRowsAllocated;
	if(*prghRows == NULL)
	{
		rghRowsAllocated.Allocate(cRows); // �ϴ� �ִ��� ���´�.
		if(rghRowsAllocated == NULL)
			return E_OUTOFMEMORY;

		*prghRows = rghRowsAllocated;
	}

	HRESULT hr = S_OK;

	while(lRowsOffset >= 0 && cRows != 0)
	{
		// cRows > cRowsInSet && iStepSize < 0
		if (lRowsOffset == 0 && cRows > 0 && iStepSize < 0)
			break;

		hr = CreateRow(lRowsOffset, *pcRowsObtained, *prghRows);
		if(FAILED(hr))
		{
			RefRows(*pcRowsObtained, *prghRows, NULL, NULL, FALSE);
			for(ULONG iRowDel = 0; iRowDel < *pcRowsObtained; iRowDel++)
				(*prghRows)[iRowDel] = NULL;
			*pcRowsObtained = 0;
			return hr;
		}

		if(hr != S_OK)
			break;

		if(m_rgRowData.GetCount() <= (size_t)lRowsOffset)
			m_rgRowData.SetCount(lRowsOffset + 1);
		if(m_rgBookmarks.GetCount() <= (size_t)lRowsOffset + 3)
			m_rgBookmarks.SetCount(lRowsOffset + 4);

		m_rgBookmarks[lRowsOffset + 3] = lRowsOffset + 1;
		cRows--;
		lRowsOffset += iStepSize;
	}

	m_iRowset = lRowsOffset;

	if(SUCCEEDED(hr) && *pcRowsObtained > 0)
		rghRowsAllocated.Detach();
	else
	{
		if(rghRowsAllocated)
			*prghRows = 0;
	}

	return hr;
}

STDMETHODIMP CCUBRIDRowset::GetNextRows(HCHAPTER hReserved, DBROWOFFSET lRowsOffset,
																				DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW **prghRows)
{
	ClearError();

	if(pcRowsObtained)
		*pcRowsObtained = 0;
	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowset), "This object is in a zombie state");
	if(cRows == 0)
		return S_OK;

	CHECK_RESTART(__uuidof(IRowset));

	if(!m_bExternalFetch)
		CHECK_CANHOLDROWS(__uuidof(IRowset));

	if(m_bAsynch)
		return GetNextRowsAsynch(hReserved, lRowsOffset, cRows, pcRowsObtained, prghRows);

	bool bProvAlloc = (pcRowsObtained && prghRows && *prghRows == 0);

	HRESULT hr;
	hr = _RowsetBaseClass::GetNextRows(hReserved, lRowsOffset, cRows, pcRowsObtained, prghRows);

	if(bProvAlloc && *pcRowsObtained == 0)
	{
		*prghRows = 0;
	}

	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowset));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::ReleaseRows(DBCOUNTITEM cRows, const HROW rghRows[],
																				DBROWOPTIONS rgRowOptions[], DBREFCOUNT rgRefCounts[],
																				DBROWSTATUS rgRowStatus[])
{
	ClearError();

	HRESULT hr = _RowsetBaseClass::ReleaseRows(cRows, rghRows, rgRowOptions, rgRefCounts, rgRowStatus);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowset));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::RestartPosition(HCHAPTER hReserved)
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowset), "This object is in a zombie state");
	HRESULT hr = _RowsetBaseClass::RestartPosition(hReserved);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowset));

	cci_fetch_buffer_clear(GetRequestHandle());

	//int m_nStatus; // 0: normal, 1: zombie, 2: need reexecute
	//if(m_nStatus==2)
	{
		hr = Reexecute();
		if(FAILED(hr))
			return RaiseError(E_FAIL, 1, __uuidof(IRowset), "Failed to reexecute the command");

		return DB_S_COMMANDREEXECUTED;
	}

	return S_OK;
}

STDMETHODIMP CCUBRIDRowset::IsSameRow(HROW hThisRow, HROW hThatRow)
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetIdentity), "This object is in a zombie state");
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::IsSameRow\n");

	// Validate row handles
	CCUBRIDRowsetRow *pRow1;
	if( ! m_rgRowHandles.Lookup((CCUBRIDRowsetRow::KeyType)hThisRow, pRow1) )
		return DB_E_BADROWHANDLE;

	CCUBRIDRowsetRow *pRow2;
	if (! m_rgRowHandles.Lookup((CCUBRIDRowsetRow::KeyType)hThatRow, pRow2) )
		return DB_E_BADROWHANDLE;

	if (pRow1->m_status == DBPENDINGSTATUS_NEW ||
		pRow2->m_status == DBPENDINGSTATUS_NEW)
		return DB_E_NEWLYINSERTED;

	if (pRow1->m_status == DBPENDINGSTATUS_INVALIDROW ||
		pRow2->m_status == DBPENDINGSTATUS_INVALIDROW)
		return DB_E_DELETEDROW;

	return pRow1->Compare(pRow2);
}

HRESULT CCUBRIDRowset::OnPropertyChanged(ULONG iCurSet, DBPROP* pDBProp)
{
	HRESULT hr = CUBRIDOnPropertyChanged(this, iCurSet, pDBProp);
	if(hr == S_FALSE)
		return _RowsetBaseClass::OnPropertyChanged(iCurSet, pDBProp);
	else
		return hr;
}

HRESULT CCUBRIDRowset::IsValidValue(ULONG iCurSet, DBPROP* pDBProp)
{
	ATLASSERT(pDBProp);

	if(pDBProp->dwPropertyID == DBPROP_ROWSET_ASYNCH)
	{
		LONG val = V_I4(&pDBProp->vValue);
		if(val == 0 || val == DBPROPVAL_ASYNCH_PREPOPULATE // --> synchronous
			|| val == DBPROPVAL_ASYNCH_POPULATEONDEMAND) // --> asynchronous
			return S_OK;

		return S_FALSE;
	}

	return _RowsetBaseClass::IsValidValue(iCurSet, pDBProp);
}

STDMETHODIMP CCUBRIDRowset::GetProperties(const ULONG cPropertyIDSets, const DBPROPIDSET rgPropertyIDSets[],
																					ULONG *pcPropertySets, DBPROPSET **prgPropertySets)
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetInfo), "This object is in a zombie state");
	HRESULT hr = _RowsetBaseClass::GetProperties(cPropertyIDSets, rgPropertyIDSets,
		pcPropertySets, prgPropertySets);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowsetInfo));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::GetReferencedRowset(DBORDINAL iOrdinal, REFIID riid,
																								IUnknown **ppReferencedRowset)
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetInfo), "This object is in a zombie state");
	HRESULT hr = _RowsetBaseClass::GetReferencedRowset(iOrdinal, riid, ppReferencedRowset);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowsetInfo));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::GetSpecification(REFIID riid, IUnknown **ppSpecification)
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetInfo), "This object is in a zombie state");
	HRESULT hr = _RowsetBaseClass::GetSpecification(riid, ppSpecification);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowsetInfo));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::GetRowFromHROW(IUnknown *pUnkOuter, HROW hRow,
																					 REFIID riid, IUnknown **ppUnk)
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IGetRow), "This object is in a zombie state");
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::GetRowFromHROW\n");

	DBCOUNTITEM iRowset;

	{
		// check arguments
		CCUBRIDRowsetRow *pRow = 0;
		if(hRow == NULL || !m_rgRowHandles.Lookup((ULONG)hRow, pRow) || pRow == NULL)
			return DB_E_BADROWHANDLE;
		if(pRow->m_status == DBPENDINGSTATUS_INVALIDROW || pRow->m_status == DBPENDINGSTATUS_DELETED)
			return DB_E_DELETEDROW;
		iRowset = pRow->m_iRowset;
	}

	if(!ppUnk)
		return E_INVALIDARG;
	if(pUnkOuter && !InlineIsEqualUnknown(riid))
		return DB_E_NOAGGREGATION;

	CComPolyObject<CCUBRIDRow> *pRow;
	HRESULT hr = CComPolyObject<CCUBRIDRow>::CreateInstance(pUnkOuter, &pRow);
	if(FAILED(hr))
		return hr;

	CComPtr<IUnknown> spUnk;

	hr = pRow->QueryInterface(&spUnk);
	if(FAILED(hr))
	{
		delete pRow;
		return hr;
	}

	CComPtr<IUnknown> spOuterUnk;
	QueryInterface(__uuidof(IUnknown), (void **)&spOuterUnk);
	pRow->m_contained.SetSite(spOuterUnk, CCUBRIDRow::FromRowset);

	CComVariant var;
	GetPropValue(&DBPROPSET_ROWSET, DBPROP_BOOKMARKS, &var);
	hr = pRow->m_contained.Initialize(GetRequestHandle(), V_BOOL(&var) == ATL_VARIANT_TRUE, hRow, iRowset);
	if (FAILED(hr))
		return E_FAIL;

	return pRow->QueryInterface(riid, (void **)ppUnk);
}

STDMETHODIMP CCUBRIDRowset::GetURLFromHROW(HROW hRow, LPOLESTR *ppwszURL)
{
	ClearError();

	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::GetURLFromHROW\n");
	return RaiseError(DB_E_NOTSUPPORTED, 0, __uuidof(IGetRow));
}

STDMETHODIMP CCUBRIDRowset::FindNextRow(HCHAPTER hChapter, HACCESSOR hAccessor,
																				void *pFindValue, DBCOMPAREOP CompareOp, DBBKMARK cbBookmark,
																				const BYTE *pBookmark, DBROWOFFSET lRowsOffset, DBROWCOUNT cRows,
																				DBCOUNTITEM *pcRowsObtained, HROW **prghRows)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::FindNextRow\n");

	ClearError();

	if(pcRowsObtained)
		*pcRowsObtained = 0;
	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetFind), "This object is in a zombie state");

	CHECK_RESTART(__uuidof(IRowsetFind));

	if(pcRowsObtained == NULL || prghRows == NULL)
		return E_INVALIDARG;
	if(cbBookmark != 0 && pBookmark == NULL)
		return E_INVALIDARG;
	if(hChapter != DB_NULL_HCHAPTER)
		return DB_E_BADCHAPTER;

	CHECK_CANHOLDROWS(__uuidof(IRowsetFind));

	ATLBINDINGS *pBinding;
	{
		bool bFound = m_rgBindings.Lookup((ULONG)hAccessor, pBinding);
		if(!bFound || pBinding == NULL)
			return DB_E_BADACCESSORHANDLE;
		if(!(pBinding->dwAccessorFlags & DBACCESSOR_ROWDATA))
			return DB_E_BADACCESSORTYPE; // row accessor
		if(pBinding->cBindings != 1)
			return DB_E_BADBINDINFO;
	}

	{
		DBCOMPAREOP LocalOp = CompareOp & ~DBCOMPAREOPS_CASESENSITIVE & ~DBCOMPAREOPS_CASEINSENSITIVE;
		if(LocalOp < 0 || LocalOp > DBCOMPAREOPS_NOTCONTAINS)
			return DB_E_BADCOMPAREOP;
		if((CompareOp & DBCOMPAREOPS_CASESENSITIVE) && (CompareOp & DBCOMPAREOPS_CASEINSENSITIVE))
			return DB_E_BADCOMPAREOP;
	}

	if(cRows == 0 && cbBookmark != 0)
		return S_OK;

	DBROWOFFSET iDir = 1;
	if(cRows < 0)
	{
		iDir = -1;
		cRows = -cRows;
	}

	if(cRows == 0)
	{
		iDir = (m_bFindForward ? 1 : -1);
	}

	m_bFindForward = (iDir == 1);

	CComHeapPtr<HROW> rghRowsAllocated;
	if(*prghRows == NULL)
	{
		rghRowsAllocated.Allocate(cRows ? cRows : 1);
		if(rghRowsAllocated == NULL)
			return E_OUTOFMEMORY;
		*prghRows = rghRowsAllocated;
	}

	DBROWOFFSET iRowsetTemp = -1;
	if(cbBookmark != 0)
	{
		if(!m_bCanScrollBack)
			return DB_E_CANTSCROLLBACKWARDS;

		HRESULT hr = ValidateBookmark(cbBookmark, pBookmark);
		if(FAILED(hr))
			return hr;

		iRowsetTemp = m_iRowset; // cache the current rowset

		if(cbBookmark == 1)
		{
			if(*pBookmark == DBBMK_FIRST)
				m_iRowset = 1;
			else // *pBookmark==DBBMK_LAST
				m_iRowset = (DBCOUNTITEM)m_rgRowData.GetCount();
		}
		else
		{
			m_iRowset = m_rgBookmarks[*pBookmark];
		}
		if(iDir == 1) m_iRowset--;
	}

	while(true)
	{
		DBCOUNTITEM cTmp;
		HROW *phRow = &(*prghRows)[0];
		m_bExternalFetch = true;
		HRESULT hr = GetNextRows(hChapter, lRowsOffset, iDir, &cTmp, &phRow);
		m_bExternalFetch = false;
		lRowsOffset = 0;

		if(FAILED(hr) || hr == DB_S_ENDOFROWSET)
			goto error;

		bool bMatch = true;

		if(CompareOp != DBCOMPAREOPS_IGNORE)
		{
			CCUBRIDRowsetRow *pRow;
			{
				bool bFound = m_rgRowHandles.Lookup((ULONG) * phRow, pRow);
				ATLASSERT(bFound && pRow != NULL);
			}

			DBBINDING &rBinding = pBinding->pBindings[0];
			hr = pRow->Compare(pFindValue, CompareOp, rBinding);
			if(hr == S_FALSE)
				bMatch = false;
			else if(hr != S_OK)
				goto error;
		}

		if(bMatch)
		{
			(*pcRowsObtained)++;
			break;
		}
		else
			ReleaseRows(1, phRow, NULL, NULL, NULL);

		continue;

error:
		if(rghRowsAllocated)
			*prghRows = 0;
		if(iRowsetTemp != -1)
			m_iRowset = iRowsetTemp;

		return hr;
	}

	if(cRows > 1)
	{
		// fetch last rows
		DBCOUNTITEM cTmp;
		HROW *phRow = &(*prghRows)[1];
		m_bExternalFetch = true;
		GetNextRows(hChapter, 0, (cRows - 1)*iDir, &cTmp, &phRow); // TODO: Interpret return value
		m_bExternalFetch = false;
		*pcRowsObtained += cTmp;
	}
	else if(cRows == 0)
	{
		HROW *phRow = &(*prghRows)[0];
		ReleaseRows(1, phRow, NULL, NULL, NULL);
		*pcRowsObtained = 0;
		if(rghRowsAllocated)
			*prghRows = 0;
	}

	if(iRowsetTemp != -1)
		m_iRowset = iRowsetTemp;

	rghRowsAllocated.Detach();

	return ( cRows == *pcRowsObtained ? S_OK : DB_S_ENDOFROWSET );
}

STDMETHODIMP CCUBRIDRowset::RefreshVisibleData(HCHAPTER hChapter, DBCOUNTITEM cRows,
																							 const HROW rghRows[], BOOL fOverWrite, DBCOUNTITEM *pcRowsRefreshed,
																							 HROW **prghRowsRefreshed, DBROWSTATUS **prgRowStatus)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::RefreshVisibleData\n");

	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetRefresh), "This object is in a zombie state");
	CHECK_RESTART(__uuidof(IRowsetRefresh));

	if(pcRowsRefreshed)
		*pcRowsRefreshed = 0;
	if(prghRowsRefreshed)
		*prghRowsRefreshed = 0;
	if(prgRowStatus)
		*prgRowStatus = 0;

	if(cRows != 0 && rghRows == NULL)
		return E_INVALIDARG;
	if(pcRowsRefreshed != NULL && prghRowsRefreshed == NULL)
		return E_INVALIDARG;

	DBCOUNTITEM cRowsLocal = ( cRows == 0 ? (DBCOUNTITEM)m_rgRowHandles.GetCount() : cRows );
	if(cRowsLocal == 0)
		return S_OK;

	if(pcRowsRefreshed)
	{
		*prghRowsRefreshed = (HROW *)CoTaskMemAlloc(cRowsLocal * sizeof(HROW));
		if(*prghRowsRefreshed == 0)
		{
			return E_OUTOFMEMORY;
		}

		if(prgRowStatus)
		{
			*prgRowStatus = (DBROWSTATUS *)CoTaskMemAlloc(cRowsLocal * sizeof(DBROWSTATUS));
			if(*prgRowStatus == 0)
			{
				CoTaskMemFree(*prghRowsRefreshed);
				*prghRowsRefreshed = 0;

				return E_OUTOFMEMORY;
			}
		}

		*pcRowsRefreshed = cRowsLocal;
	}

	DBROWSTATUS statTmp;
	bool bSucceeded = false, bFailed = false;
	POSITION pos = m_rgRowHandles.GetStartPosition();

	for(DBCOUNTITEM i = 0; i < cRowsLocal; i++)
	{
		ATLASSERT(pos != NULL);

		HROW hRow = ( cRows == 0 ? m_rgRowHandles.GetNextKey(pos) : rghRows[i] );
		if(pcRowsRefreshed)
			(*prghRowsRefreshed)[i] = hRow;
		DBROWSTATUS &rCurStat = ( (pcRowsRefreshed && prgRowStatus) ? (*prgRowStatus)[i] : statTmp );

		CCUBRIDRowsetRow *pRow;
		{
			bool bFound = m_rgRowHandles.Lookup((ULONG)hRow, pRow);
			if(!bFound || pRow == NULL)
			{
				rCurStat = DBROWSTATUS_E_INVALID;
				bFailed = true;
				continue;
			}
		}

		if(pRow->m_status == DBPENDINGSTATUS_NEW)
		{
			rCurStat = DBROWSTATUS_E_PENDINGINSERT;
			bFailed = true;
			continue;
		}

		if(!fOverWrite && (pRow->m_status == 0 || pRow->m_status == DBPENDINGSTATUS_UNCHANGED))
		{
			rCurStat = DBROWSTATUS_S_OK;
			bSucceeded = true;
			continue;
		}

		HRESULT hr = pRow->ReadData(GetRequestHandle(), false, true);
		if(hr == DB_E_DELETEDROW)
		{
			rCurStat = DBROWSTATUS_E_DELETED;
			bFailed = true;
			continue;
		}

		rCurStat = DBROWSTATUS_S_OK;
		bSucceeded = true;
	}

	if(!bFailed)
		return S_OK;
	else
		return bSucceeded ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED;
}

STDMETHODIMP CCUBRIDRowset::GetLastVisibleData(HROW hRow, HACCESSOR hAccessor, void *pData)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::GetLastVisibleData\n");

	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetRefresh), "This object is in a zombie state");
	CHECK_RESTART(__uuidof(IRowsetRefresh));

	ATLBINDINGS *pBinding;
	{
		bool bFound = m_rgBindings.Lookup((ULONG)hAccessor, pBinding);
		if(!bFound || pBinding == NULL)
			return DB_E_BADACCESSORHANDLE;
		if(!(pBinding->dwAccessorFlags & DBACCESSOR_ROWDATA))
			return DB_E_BADACCESSORTYPE; // row accessor
		if(pData == NULL && pBinding->cBindings != 0)
			return E_INVALIDARG;
	}

	// Attempt to locate the row in our map
	CCUBRIDRowsetRow *pRow;
	{
		bool bFound = m_rgRowHandles.Lookup((ULONG)hRow, pRow);
		if(!bFound || pRow == NULL)
			return DB_E_BADROWHANDLE;
	}

	if(pRow->m_status == DBPENDINGSTATUS_NEW)
		return DB_E_PENDINGINSERT;

	DBORDINAL cCols;
	ATLCOLUMNINFO *pInfo = GetColumnInfo(this, &cCols);
	DBROWCOUNT dwBookmark = pRow->m_iRowset + 3; //Util::FindBookmark(m_rgBookmarks, (LONG)pRow->m_iRowset+1);

	CCUBRIDRowsetRow OrigRow(pRow->m_iRowset, cCols, pInfo, m_spConvert, m_Columns.m_defaultVal);
	OrigRow.ReadData(GetRequestHandle(), false, true);

	return OrigRow.WriteData(pBinding, pData, dwBookmark);
}

STDMETHODIMP CCUBRIDRowset::Compare(HCHAPTER hReserved, DBBKMARK cbBookmark1, const BYTE *pBookmark1,
																		DBBKMARK cbBookmark2, const BYTE *pBookmark2, DBCOMPARE *pComparison)
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetLocate), "This object is in a zombie state");

	HRESULT hr = _RowsetBaseClass::Compare(hReserved, cbBookmark1, pBookmark1, cbBookmark2, pBookmark2, pComparison);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowsetLocate));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::GetRowsAt(HWATCHREGION hReserved1, HCHAPTER hReserved2,
																			DBBKMARK cbBookmark, const BYTE *pBookmark, DBROWOFFSET lRowsOffset,
																			DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW **prghRows)
{
	ClearError();

	if(pcRowsObtained)
		*pcRowsObtained = 0;
	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetLocate), "This object is in a zombie state");

	CHECK_RESTART(__uuidof(IRowsetLocate));

	CHECK_CANHOLDROWS(__uuidof(IRowsetLocate));

	HRESULT hr = _RowsetBaseClass::GetRowsAt(hReserved1, hReserved2, cbBookmark, pBookmark,
		lRowsOffset, cRows, pcRowsObtained, prghRows);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowsetLocate));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::GetRowsByBookmark(HCHAPTER hReserved, DBCOUNTITEM cRows,
																							const DBBKMARK rgcbBookmarks[], const BYTE *rgpBookmarks[],
																							HROW rghRows[], DBROWSTATUS rgRowStatus[])
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetLocate), "This object is in a zombie state");

	CHECK_RESTART(__uuidof(IRowsetLocate));

	CHECK_CANHOLDROWS(__uuidof(IRowsetLocate));

	HRESULT hr = _RowsetBaseClass::GetRowsByBookmark(hReserved, cRows, rgcbBookmarks, rgpBookmarks, rghRows, rgRowStatus);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowsetLocate));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::Hash(HCHAPTER hReserved, DBBKMARK cBookmarks,
																 const DBBKMARK rgcbBookmarks[], const BYTE *rgpBookmarks[],
																 DBHASHVALUE rgHashedValues[], DBROWSTATUS rgBookmarkStatus[])
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetLocate), "This object is in a zombie state");
	HRESULT hr = _RowsetBaseClass::Hash(hReserved, cBookmarks, rgcbBookmarks,
		rgpBookmarks, rgHashedValues, rgBookmarkStatus);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowsetLocate));
	else
		return hr;
}

STDMETHODIMP CCUBRIDRowset::GetApproximatePosition(HCHAPTER hReserved, DBBKMARK cbBookmark,
																									 const BYTE *pBookmark, DBCOUNTITEM *pulPosition, DBCOUNTITEM *pcRows)
{
	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetScroll), "This object is in a zombie state");
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::GetApproximatePosition\n");

	DBCOUNTITEM ulRowCount = (DBCOUNTITEM)m_rgRowData.GetCount();

	if(cbBookmark == 0 || pulPosition == NULL)
	{
		if(pcRows)
			*pcRows = ulRowCount;
		return S_OK; // return only total number of rows
	}

	HRESULT hr = ValidateBookmark(cbBookmark, pBookmark);
	if(hr != S_OK)
		return RaiseError(hr, 0, __uuidof(IRowsetScroll));

	if(cbBookmark != 1 && m_rgBookmarks[*pBookmark] == -1)
		return RaiseError(DB_E_BADBOOKMARK, 0, __uuidof(IRowsetScroll));

	if(pcRows)
		*pcRows = ulRowCount;

	if(m_rgRowData.GetCount() == 0)
	{
		*pulPosition = 0;
	}
	else if(cbBookmark == 1)
	{
		if(*pBookmark == DBBMK_FIRST)
			*pulPosition = 1;
		else // *pBookmark==DBBMK_LAST
			*pulPosition = ulRowCount;
	}
	else
	{
		*pulPosition = m_rgBookmarks[*pBookmark];
	}

	return S_OK;
}

STDMETHODIMP CCUBRIDRowset::GetRowsAtRatio(HWATCHREGION hReserved1, HCHAPTER hReserved2,
																					 DBCOUNTITEM ulNumerator, DBCOUNTITEM ulDenominator,
																					 DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW **prghRows)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::GetRowsAtRatio\n");

	ClearError();

	if(pcRowsObtained)
		*pcRowsObtained = 0;
	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetScroll), "This object is in a zombie state");

	CHECK_RESTART(__uuidof(IRowsetScroll));

	CHECK_CANHOLDROWS(__uuidof(IRowsetScroll));

	if(pcRowsObtained == NULL || prghRows == NULL)
		return RaiseError(E_INVALIDARG, 0, __uuidof(IRowsetScroll));
	if(ulDenominator == 0 || ulNumerator > ulDenominator)
		return RaiseError(DB_E_BADRATIO, 0, __uuidof(IRowsetScroll));

	if( (ulNumerator == 0 && cRows < 0) || (ulNumerator == ulDenominator && cRows > 0) )
		return RaiseError(DB_S_ENDOFROWSET, 0, __uuidof(IRowsetScroll));

	DBROWOFFSET iRowsetTemp = m_iRowset; // Cache the current rowset

	DBCOUNTITEM ulRowCount = (DBCOUNTITEM)m_rgRowData.GetCount();
	if(ulNumerator == 0)
		m_iRowset = 0;
	else if(ulNumerator == ulDenominator)
		m_iRowset = ulRowCount;
	else
		m_iRowset = ulNumerator * ulRowCount / ulDenominator;

	m_bExternalFetch = true;
	HRESULT hr = GetNextRows(hReserved2, 0, cRows, pcRowsObtained, prghRows);
	m_bExternalFetch = false;

	m_iRowset = iRowsetTemp;

	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowsetLocate));
	else
		return hr;
}
