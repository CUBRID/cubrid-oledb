////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Rowset.h"
#include "DataSource.h"
#include "CUBRIDStream.h"

static HRESULT DoCommit(CCUBRIDRowset *pRowset)
{
	// CHECK_UPDATABILITY
	ATLASSERT(pRowset->m_eType == CCUBRIDRowset::FromCommand || pRowset->m_eType == CCUBRIDRowset::FromSession);

	Util::ITxnCallback *pOwner = 0;
	if(pRowset->m_eType == CCUBRIDRowset::FromCommand)
		pOwner = pRowset->GetCommandPtr();
	else if(pRowset->m_eType == CCUBRIDRowset::FromSession)
		pOwner = pRowset;

	CCUBRIDSession *pSession = pRowset->GetSessionPtr();
	ATLASSERT(pSession);

	return pSession->AutoCommit(pOwner);
}

#define CHECK_UPDATABILITY(prop)													                       \
	do																				                                     \
{																				                                       \
	if(cci_is_updatable(GetRequestHandle())==0)									                 \
	return RaiseError(DB_SEC_E_PERMISSIONDENIED, 0, __uuidof(IRowsetChange));  \
	\
{																			                                       \
	CComVariant var;														                               \
	HRESULT hr = GetPropValue(&DBPROPSET_ROWSET, DBPROP_UPDATABILITY, &var);   \
	if(!(V_I4(&var) & (prop)))												                         \
	return RaiseError(DB_E_NOTSUPPORTED, 0, __uuidof(IRowsetChange));	       \
}																			                                       \
} while(0)

static void MakeRowInvalid(CCUBRIDRowset *pRowset, CCUBRIDRowsetRow *pRow)
{
	//Invalidate bookmark
	DBROWCOUNT dwBookmark = pRow->m_iRowset + 3; //Util::FindBookmark(pRowset->m_rgBookmarks, (LONG)pRow->m_iRowset+1);
	pRowset->m_rgBookmarks[dwBookmark] = -1;

	//Next fetch position
	//if(pRow->m_status!=DBPENDINGSTATUS_NEW && pRowset->m_iRowset > (LONG)pRow->m_iRowset)
	//	pRowset->m_iRowset--;

	pRow->m_status = DBPENDINGSTATUS_INVALIDROW;
}

static bool IsDeferred(CCUBRIDRowset *pRowset)
{
	CComVariant var;
	HRESULT hr = pRowset->GetPropValue(&DBPROPSET_ROWSET, DBPROP_IRowsetUpdate, &var);
	ATLASSERT(SUCCEEDED(hr));

	return (V_BOOL(&var) == ATL_VARIANT_TRUE);
}

STDMETHODIMP CCUBRIDRowset::DeleteRows(HCHAPTER hReserved, DBCOUNTITEM cRows,
																			 const HROW rghRows[], DBROWSTATUS rgRowStatus[])
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::DeleteRows\n");

	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetChange), CLocale::Msg(CLocale::ID_OBJECT_ZOMBIE_STATE));

	CHECK_UPDATABILITY(DBPROPVAL_UP_DELETE);

	if(cRows == 0)
		return S_OK;

	if(rghRows == NULL && cRows >= 1)
		return RaiseError(E_INVALIDARG, 0, __uuidof(IRowsetChange));

	// Determine if we're in immediate or deferred mode
	bool bDeferred = IsDeferred(this);

	int hConn;
	{
		HRESULT hr = GetConnectionHandle(&hConn);
		if(FAILED(hr))
			return RaiseError(E_FAIL, 0, __uuidof(IRowsetChange));
	}

	BOOL bSuccess = false;
	BOOL bFailed = false;
	for(DBCOUNTITEM i = 0; i < cRows; i++)
	{
		HROW hRow = rghRows[i];

		// Attempt to locate the row in our map
		CCUBRIDRowsetRow *pRow;
		{
			bool bFound = m_rgRowHandles.Lookup((ULONG)hRow, pRow);
			if(!bFound || pRow == NULL)
			{
				// invalid handle
				bFailed = true;
				if(rgRowStatus)
					rgRowStatus[i] = DBROWSTATUS_E_INVALID;
				continue;
			}
		}

		if(pRow->m_status == DBPENDINGSTATUS_DELETED)
		{
			// already deleted
			if(rgRowStatus)
				rgRowStatus[i] = DBROWSTATUS_E_DELETED;
			bFailed  = true;
			continue;
		}

		ATLASSERT( pRow->m_iRowset == (ULONG) - 1 || pRow->m_iRowset < m_rgRowData.GetCount() );

		DBROWSTATUS rowStat = DBROWSTATUS_S_OK;

		// mark the row as deleted
		if(pRow->m_status == DBPENDINGSTATUS_INVALIDROW)
		{
			bFailed = true;
			// unsigned high bit signified neg. number
			if(pRow->m_dwRef & 0x80000000)
				rowStat = DBROWSTATUS_E_INVALID;
			else
				rowStat = DBROWSTATUS_E_DELETED;
		}
		else if(pRow->m_iRowset == (ULONG) - 1 && pRow->m_status != DBPENDINGSTATUS_NEW)
		{
			bFailed = true;
			rowStat = DBROWSTATUS_E_NEWLYINSERTED;
		}
		else
		{
			bSuccess = true;
			rowStat = DBROWSTATUS_S_OK;
			if(pRow->m_status == DBPENDINGSTATUS_NEW)
				MakeRowInvalid(this, pRow);
			else
				pRow->m_status = DBPENDINGSTATUS_DELETED;
		}

		if(!bDeferred && pRow->m_status == DBPENDINGSTATUS_DELETED)
		{
			HRESULT hr = pRow->WriteData(hConn, GetRequestHandle(), m_strTableName, NULL, NULL);
			if(FAILED(hr))
				return hr;

			MakeRowInvalid(this, pRow);
		}

		if(rgRowStatus)
			rgRowStatus[i] = rowStat;
	}

	if(!bDeferred)
		DoCommit(this);

	if(bFailed)
	{
		if(bSuccess)
			return DB_S_ERRORSOCCURRED;
		else
			return RaiseError(DB_E_ERRORSOCCURRED, 0, __uuidof(IRowsetChange));
	}
	else
		return S_OK;
}

STDMETHODIMP CCUBRIDRowset::SetData(HROW hRow, HACCESSOR hAccessor, void *pData)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::SetData\n");

	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetChange), CLocale::Msg(CLocale::ID_OBJECT_ZOMBIE_STATE));

	//TODO Investigate the need for CHECK_RESTART
	CHECK_RESTART(__uuidof(IRowsetChange));
	CHECK_UPDATABILITY(DBPROPVAL_UP_CHANGE);

	//Verify we are in AUTOCOMMIT OFF
	int hConn;
	HRESULT hr = GetConnectionHandle(&hConn);
	if(FAILED(hr))
		return RaiseError(hr, 0, __uuidof(IRowsetChange));

	if(cci_get_autocommit(hConn) == CCI_AUTOCOMMIT_TRUE)
	{
		ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::SetData Error: AUTOCOMMIT mode is ON! For Update, it should be turned OFF.\n");
		return RaiseError(E_FAIL, 0, __uuidof(IRowsetChange));
	}

	// Determine if we're in immediate or deferred mode
	bool bDeferred = IsDeferred(this);

	// Attempt to locate the row in our map
	CCUBRIDRowsetRow *pRow;
	{
		bool bFound = m_rgRowHandles.Lookup((ULONG)hRow, pRow);
		if(!bFound || pRow == NULL)
			return RaiseError(DB_E_BADROWHANDLE, 0, __uuidof(IRowsetChange));
	}

	ATLASSERT( pRow->m_iRowset == (ULONG) - 1 || pRow->m_iRowset < m_rgRowData.GetCount() );

	if(pRow->m_status == DBPENDINGSTATUS_DELETED || pRow->m_status == DBPENDINGSTATUS_INVALIDROW)
		return RaiseError(DB_E_DELETEDROW, 0, __uuidof(IRowsetChange));

	if(pRow->m_iRowset == (ULONG) - 1 && pRow->m_status != DBPENDINGSTATUS_NEW)
		return DB_E_NEWLYINSERTED;

	ATLBINDINGS *pBinding;
	{
		bool bFound = m_rgBindings.Lookup((ULONG)hAccessor, pBinding);
		if(!bFound || pBinding == NULL)
			return RaiseError(DB_E_BADACCESSORHANDLE, 0, __uuidof(IRowsetChange));
		if(!(pBinding->dwAccessorFlags & DBACCESSOR_ROWDATA))
			return RaiseError(DB_E_BADACCESSORTYPE, 0, __uuidof(IRowsetChange)); // row accessor
		if(pData == NULL && pBinding->cBindings != 0)
			return RaiseError(E_INVALIDARG, 0, __uuidof(IRowsetChange));
	}

	hr = pRow->ReadData(pBinding, pData);
	if(FAILED(hr))
		return hr;

	if(pRow->m_status != DBPENDINGSTATUS_NEW)
		pRow->m_status = DBPENDINGSTATUS_CHANGED;

	if(!bDeferred)
	{
		int hConn;
		HRESULT hr = GetConnectionHandle(&hConn);
		if(FAILED(hr))
			return RaiseError(hr, 0, __uuidof(IRowsetChange));

		hr = pRow->WriteData(hConn, GetRequestHandle(), m_strTableName, pBinding, pData);

		if (hr == DB_E_INTEGRITYVIOLATION)
			return RaiseError(DB_E_INTEGRITYVIOLATION, 0, __uuidof(IRowsetChange));

		if(FAILED(hr))
			return RaiseError(DB_E_ERRORSOCCURRED, 0, __uuidof(IRowsetChange));

		pRow->m_status = 0; //TODO ...or UNCHANGED?
		DoCommit(this);
	}

	return hr; // S_OK or DB_S_ERRORSOCCURRED
}

STDMETHODIMP CCUBRIDRowset::InsertRow(HCHAPTER hReserved, HACCESSOR hAccessor, void *pData, HROW *phRow)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::InsertRow\n");

	if(phRow)
		*phRow = NULL;

	if(phRow)
		CHECK_CANHOLDROWS(__uuidof(IRowsetChange));

	ClearError();
	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetChange), CLocale::Msg(CLocale::ID_OBJECT_ZOMBIE_STATE));

	CHECK_UPDATABILITY(DBPROPVAL_UP_INSERT);

	// Determine if we're in immediate or deferred mode
	bool bDeferred = IsDeferred(this);

	ATLBINDINGS *pBinding;
	{
		bool bFound = m_rgBindings.Lookup((ULONG)hAccessor, pBinding);
		if(!bFound || pBinding == NULL)
			return RaiseError(DB_E_BADACCESSORHANDLE, 0, __uuidof(IRowsetChange));
		if(!(pBinding->dwAccessorFlags & DBACCESSOR_ROWDATA))
			return RaiseError(DB_E_BADACCESSORTYPE, 0, __uuidof(IRowsetChange)); // row accessor
		if(pData == NULL && pBinding->cBindings != 0)
			return RaiseError(E_INVALIDARG, 0, __uuidof(IRowsetChange));
	}

	CCUBRIDRowsetRow *pRow;
	CCUBRIDRowsetRow::KeyType key;
	{
		DBORDINAL cCols;
		ATLCOLUMNINFO *pColInfo = GetColumnInfo(this, &cCols);
		ATLTRY(pRow = new CCUBRIDRowsetRow(-1, cCols, pColInfo, m_spConvert, m_Columns.m_defaultVal));
		if(pRow == NULL)
			return RaiseError(E_OUTOFMEMORY, 0, __uuidof(IRowsetChange));

		key = (CCUBRIDRowsetRow::KeyType)m_rgRowData.GetCount();
		if (key == 0)
			key++;

		while(m_rgRowHandles.Lookup(key))
		{
			key++;
		}
		m_rgRowHandles.SetAt(key, pRow);

		//m_rgRowData.SetCount(m_rgRowData.GetCount() + 1);
	}

	HRESULT hrRead = pRow->ReadData(pBinding, pData);
	if(FAILED(hrRead))
	{
		//m_rgRowData.SetCount(m_rgRowData.GetCount()-1);
		m_rgRowHandles.RemoveKey(key);
		delete pRow;

		return hrRead;
	}

	pRow->m_status = DBPENDINGSTATUS_NEW;

	if(phRow)
	{
		pRow->AddRefRow();
		*phRow = (HROW)key;
	}

	//m_rgBookmarks.Add(pRow->m_iRowset+1);

	if(!bDeferred)
	{
		int hConn;
		HRESULT hr = GetConnectionHandle(&hConn);
		if(FAILED(hr))
			return E_FAIL;

		hr = pRow->WriteData(hConn, GetRequestHandle(), m_strTableName, pBinding, pData);
		if(FAILED(hr))
		{
			pRow->ReleaseRow();
			if (phRow != NULL)
				*phRow = NULL;
			return hr;
		}

		pRow->m_status = 0; //TODO ...or UNCHANGED?
		if(pRow->m_dwRef == 0)
		{
			m_rgRowHandles.RemoveKey(key);
			delete pRow;
		}

		DoCommit(this); // commit
		//pRow->ReadData(GetRequestHandle(), true);
	}

	return hrRead;
}

STDMETHODIMP CCUBRIDRowset::GetOriginalData(HROW hRow, HACCESSOR hAccessor, void *pData)
{
	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDRowset::GetOriginalData\n"));

	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetUpdate), CLocale::Msg(CLocale::ID_OBJECT_ZOMBIE_STATE));

	CHECK_RESTART(__uuidof(IRowsetUpdate));

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

	// If the status is DBPENDINGSTATUS_INVALIDROW, the row has been
	// deleted and the change transmitted to the data source.  In
	// this case, we can't get the original data so return
	// DB_E_DELETEDROW.
	if (pRow->m_status == DBPENDINGSTATUS_INVALIDROW)
		return DB_E_DELETEDROW;

	// Determine if we have a pending insert. In this case, the
	// spec says revert to default values, and if defaults,
	// are not available, then NULLs.

	if (pRow->m_status == DBPENDINGSTATUS_NEW)
	{
		for (ULONG lBind = 0; lBind < pBinding->cBindings; lBind++)
		{
			DBBINDING* pBindCur = &(pBinding->pBindings[lBind]);

			if (pBindCur->dwPart & DBPART_VALUE)
				*((BYTE *)pData + pBindCur->obValue) = NULL;
			if (pBindCur->dwPart & DBPART_STATUS)
				*((DBSTATUS*)((BYTE*)(pData) + pBindCur->obStatus)) = DBSTATUS_S_ISNULL;
		}

		return S_OK;
	}

	if(pRow->m_iRowset == (ULONG) - 1)
	{
		return pRow->WriteData(pBinding, pData, 0);
	}

	DBORDINAL cCols;
	ATLCOLUMNINFO *pInfo = GetColumnInfo(this, &cCols);
	DBROWCOUNT dwBookmark = pRow->m_iRowset + 3; //Util::FindBookmark(m_rgBookmarks, (LONG)pRow->m_iRowset+1);

	CCUBRIDRowsetRow OrigRow(pRow->m_iRowset, cCols, pInfo, m_spConvert, m_Columns.m_defaultVal);
	OrigRow.ReadData(GetRequestHandle());

	return OrigRow.WriteData(pBinding, pData, dwBookmark);
}

STDMETHODIMP CCUBRIDRowset::GetPendingRows(HCHAPTER hReserved, DBPENDINGSTATUS dwRowStatus,
																					 DBCOUNTITEM *pcPendingRows, HROW **prgPendingRows, DBPENDINGSTATUS **prgPendingStatus)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::GetPendingRows\n");

	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetUpdate), CLocale::Msg(CLocale::ID_OBJECT_ZOMBIE_STATE));

	bool bPending = false;
	CCUBRIDRowsetRow *pRow = NULL;

	if(pcPendingRows)
	{
		*pcPendingRows = 0;
		if(prgPendingRows)
			*prgPendingRows = NULL;
		if(prgPendingStatus)
			*prgPendingStatus = NULL;
	}

	// Validate input parameters
	if ((dwRowStatus & ~(DBPENDINGSTATUS_NEW | DBPENDINGSTATUS_CHANGED | DBPENDINGSTATUS_DELETED)) != 0)
		return E_INVALIDARG;

	// Determine how many rows we'll need to return
	POSITION pos = m_rgRowHandles.GetStartPosition();
	while( pos != NULL )
	{
		MapClass::CPair* pPair = m_rgRowHandles.GetNext( pos );
		ATLASSERT( pPair != NULL );

		// Check to see if a row has a pending status
		pRow = pPair->m_value;

		if (pRow->m_status & dwRowStatus)
		{
			if (pcPendingRows != NULL)
				(*pcPendingRows)++;
			bPending = true;
		}
	}

	// In this case, there are no pending rows that match, just exit out
	if (!bPending)
	{
		// There are no pending rows so exit immediately
		return S_FALSE;
	}
	else
	{
		// Here' the consumer just wants to see if there are pending rows
		// we know that so we can exit
		if (pcPendingRows == NULL)
			return S_OK;
	}

	// Allocate arrays for pending rows
	{
		if (prgPendingRows != NULL)
		{
			*prgPendingRows = (HROW*)CoTaskMemAlloc(*pcPendingRows * sizeof(HROW));
			if (*prgPendingRows == NULL)
			{
				*pcPendingRows = 0;

				return E_OUTOFMEMORY;
			}
		}

		if (prgPendingStatus != NULL)
		{
			*prgPendingStatus = (DBPENDINGSTATUS*)CoTaskMemAlloc(*pcPendingRows * sizeof(DBPENDINGSTATUS));
			if (*prgPendingStatus == NULL)
			{
				*pcPendingRows = 0;
				CoTaskMemFree(*prgPendingRows);
				*prgPendingRows = NULL;

				return E_OUTOFMEMORY;
			}
			memset(*prgPendingStatus, 0, *pcPendingRows * sizeof(DBPENDINGSTATUS));
		}
	}

	if (prgPendingRows || prgPendingStatus)
	{
		ULONG ulRows = 0;
		pos = m_rgRowHandles.GetStartPosition();
		while( pos != NULL )
		{
			MapClass::CPair* pPair = m_rgRowHandles.GetNext( pos );
			ATLASSERT( pPair != NULL );

			pRow = pPair->m_value;
			if (pRow->m_status & dwRowStatus)
			{
				// Add the output row
				pRow->AddRefRow();
				if (prgPendingRows)
					((*prgPendingRows)[ulRows]) = /*(HROW)*/pPair->m_key;
				if (prgPendingStatus)
					((*prgPendingStatus)[ulRows]) = (DBPENDINGSTATUS)pRow->m_status;
				ulRows++;
			}
		}

		if (pcPendingRows != NULL)
			*pcPendingRows = ulRows;
	}

	// Return code depending on
	return S_OK;
}

STDMETHODIMP CCUBRIDRowset::GetRowStatus(HCHAPTER hReserved, DBCOUNTITEM cRows,
																				 const HROW rghRows[], DBPENDINGSTATUS rgPendingStatus[])
{
	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDRowset::GetRowStatus\n"));

	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetUpdate), CLocale::Msg(CLocale::ID_OBJECT_ZOMBIE_STATE));

	bool bSucceeded = true;
	ULONG ulFetched = 0;

	if(cRows)
	{
		// check for correct pointers
		if(rghRows == NULL || rgPendingStatus == NULL)
			return E_INVALIDARG;

		for (ULONG ulRows = 0; ulRows < cRows; ulRows++)
		{
			CCUBRIDRowsetRow* pRow;
			bool bFound = m_rgRowHandles.Lookup((ULONG)rghRows[ulRows], pRow);
			if ((! bFound || pRow == NULL) || (pRow->m_status == DBPENDINGSTATUS_INVALIDROW))
			{
				rgPendingStatus[ulRows] = DBPENDINGSTATUS_INVALIDROW;
				bSucceeded = false;
				continue;
			}
			if (pRow->m_status != 0)
				rgPendingStatus[ulRows] = pRow->m_status;
			else
				rgPendingStatus[ulRows] = DBPENDINGSTATUS_UNCHANGED;

			ulFetched++;
		}
	}

	if (bSucceeded)
	{
		return S_OK;
	}
	else
	{
		if (ulFetched > 0)
			return DB_S_ERRORSOCCURRED;
		else
			return DB_E_ERRORSOCCURRED;
	}
}

STDMETHODIMP CCUBRIDRowset::Undo(HCHAPTER hReserved, DBCOUNTITEM cRows, const HROW rghRows[],
																 DBCOUNTITEM *pcRowsUndone, HROW **prgRowsUndone, DBROWSTATUS **prgRowStatus)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::Undo\n");

	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetUpdate), CLocale::Msg(CLocale::ID_OBJECT_ZOMBIE_STATE));

	CHECK_RESTART(__uuidof(IRowsetUpdate));

	DBCOUNTITEM ulRows = 0;
	bool bNotIgnore = true;

	// the following lines are used to fix the two _alloca calls below.  Those calls are risky
	// because we may be allocating huge amounts of data.  So instead I'll allocate that data on heap.
	// But if you use _alloca you don't have to worry about cleaning this memory.  So we will use these
	// temporary variables to allocate memory on heap.  As soon as we exit the function, the memory will
	// be cleaned up, just as if we were using alloca. So now, instead of calling alloca, I'll alloc
	// memory on heap using the two smnart pointers below, and then assing it to the actual pointers.
	CHeapPtr<HROW> spTempRowsUndone;
	CHeapPtr<DBROWSTATUS> spTempRowStatus;

	if(cRows || pcRowsUndone)
	{
		if(prgRowsUndone)
			*prgRowsUndone = NULL;
		if(prgRowStatus)
			*prgRowStatus = NULL;
	}
	else
	{
		bNotIgnore = false;		// Don't do status or row arrays
	}

	// Check to see how many changes we'll undo
	if(pcRowsUndone)
	{
		*pcRowsUndone = NULL;
		if(prgRowsUndone == NULL)
			return E_INVALIDARG;
	}

	if(cRows)
	{
		if(rghRows == NULL)
			return E_INVALIDARG;
		ulRows = cRows;
	}
	else
		ulRows = (DBCOUNTITEM)m_rgRowHandles.GetCount();

	// NULL out pointers
	{
		if(prgRowsUndone && ulRows && bNotIgnore)
		{
			// Make a temporary buffer as we may not fill up everything
			// in the case where cRows == 0
			if(cRows)
				*prgRowsUndone = (HROW*)CoTaskMemAlloc(ulRows * sizeof(HROW));
			else
			{
				spTempRowsUndone.Allocate(ulRows);
				*prgRowsUndone = spTempRowsUndone;
			}
			if (*prgRowsUndone == NULL)
				return E_OUTOFMEMORY;
		}

		if(prgRowStatus && ulRows && bNotIgnore)
		{
			if(cRows)
				*prgRowStatus = (DBROWSTATUS*)CoTaskMemAlloc(ulRows * sizeof(DBROWSTATUS));
			else
			{
				spTempRowStatus.Allocate(ulRows);
				*prgRowStatus = spTempRowStatus;
			}
			if(*prgRowStatus == NULL)
			{
				if(cRows)
					CoTaskMemFree(*prgRowsUndone);
				*prgRowsUndone = NULL;

				return E_OUTOFMEMORY;
			}
		}
	}

	bool bSucceeded = false;
	bool bFailed = false;
	ULONG ulUndone = 0;
	POSITION pos = m_rgRowHandles.GetStartPosition();
	for (ULONG ulUndoRow = 0; ulUndoRow < ulRows; ulUndoRow++)
	{
		ULONG ulCurrentRow = ulUndone;

		HROW hRowUndo = NULL;
		{
			if(cRows)
			{
				hRowUndo = rghRows[ulUndoRow];
			}
			else
			{
				// ATLASSERT(ulUndoRow < (ULONG)m_rgRowHandles.GetCount());
				ATLASSERT( pos != NULL );
				MapClass::CPair* pPair = m_rgRowHandles.GetNext(pos);
				ATLASSERT( pPair != NULL );
				hRowUndo = pPair->m_key;
			}
		}

		if(prgRowsUndone && bNotIgnore)
			(*prgRowsUndone)[ulCurrentRow] = hRowUndo;

		// Fetch the RowClass and determine if it is valid
		CCUBRIDRowsetRow *pRow;
		{
			bool bFound = m_rgRowHandles.Lookup((ULONG)hRowUndo, pRow);
			if (!bFound || pRow == NULL)
			{
				if (prgRowStatus && bNotIgnore)
					(*prgRowStatus)[ulCurrentRow] = DBROWSTATUS_E_INVALID;
				bFailed = true;
				ulUndone++;
				continue;
			}
		}

		// If cRows is zero we'll go through all rows fetched.  We shouldn't
		// increment the count for rows that haven't been modified.
		if (cRows != 0 || (pRow != NULL &&
			pRow->m_status != 0 && pRow->m_status != DBPENDINGSTATUS_UNCHANGED
			&& pRow->m_status != DBPENDINGSTATUS_INVALIDROW))
			ulUndone++;
		else
			continue;

		if(cRows == 0)
			pRow->AddRefRow();

		switch (pRow->m_status)
		{
		case DBPENDINGSTATUS_INVALIDROW:
			{
				if(prgRowStatus && bNotIgnore)
					(*prgRowStatus)[ulCurrentRow] = DBROWSTATUS_E_DELETED;
				bFailed = true;
			}
			break;
		case DBPENDINGSTATUS_NEW:
			{
				// If the row is newly inserted, go ahead and mark its
				// row as INVALID (according to the specification).
				if(prgRowStatus && bNotIgnore)
					(*prgRowStatus)[ulCurrentRow] = DBROWSTATUS_S_OK;

				MakeRowInvalid(this, pRow);

				bSucceeded = true;
			}
			break;
		case DBPENDINGSTATUS_CHANGED:
		case DBPENDINGSTATUS_DELETED:
			{
				// read data back
				pRow->ReadData(GetRequestHandle());
				pRow->m_status = DBPENDINGSTATUS_UNCHANGED;

				if(prgRowStatus && bNotIgnore)
					(*prgRowStatus)[ulCurrentRow] = DBROWSTATUS_S_OK;
				bSucceeded = true;
			}
			break;
		default:
			{
				pRow->m_status = DBPENDINGSTATUS_UNCHANGED;

				if(prgRowStatus && bNotIgnore)
					(*prgRowStatus)[ulCurrentRow] = DBROWSTATUS_S_OK;
				bSucceeded = true;
			}
			break;
		}

		// Check if we need to release the row because it's ref was 0
		// See the IRowset::ReleaseRows section in the spec for more
		// information
		if (pRow->m_dwRef == 0)
		{
			pRow->AddRefRow();	// Artifically bump this to remove it
			if( FAILED( RefRows(1, &hRowUndo, NULL, NULL, false) ) )
				return E_FAIL;
		}
	}

	// Set the output for rows undone.
	if(pcRowsUndone) *pcRowsUndone = ulUndone;

	if(ulUndone == 0)
	{
		if(prgRowsUndone)
		{
			CoTaskMemFree(*prgRowsUndone);
			*prgRowsUndone = NULL;
		}

		if(prgRowStatus)
		{
			CoTaskMemFree(*prgRowStatus);
			*prgRowStatus = NULL;
		}
	}
	else if(cRows == 0)
	{
		// In the case where cRows == 0, we need to allocate the final
		// array of data.
		if(prgRowsUndone && bNotIgnore)
		{
			HROW *prgRowsTemp = (HROW *)CoTaskMemAlloc(ulUndone * sizeof(HROW));
			if(prgRowsTemp == NULL)
				return E_OUTOFMEMORY;
			memcpy(prgRowsTemp, *prgRowsUndone, ulUndone * sizeof(HROW));
			*prgRowsUndone = prgRowsTemp;
		}

		if(prgRowStatus && bNotIgnore)
		{
			DBROWSTATUS *prgRowStatusTemp = (DBROWSTATUS *)CoTaskMemAlloc(ulUndone * sizeof(DBROWSTATUS));
			if(prgRowStatusTemp == NULL)
			{
				CoTaskMemFree(*prgRowsUndone);
				*prgRowsUndone = NULL;
				return E_OUTOFMEMORY;
			}
			memcpy(prgRowStatusTemp, *prgRowStatus, ulUndone * sizeof(DBROWSTATUS));
			*prgRowStatus = prgRowStatusTemp;
		}
	}

	// Send the return value
	if(!bFailed)
		return S_OK;
	else
	{
		return bSucceeded ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED;
	}
}

STDMETHODIMP CCUBRIDRowset::Update(HCHAPTER hReserved, DBCOUNTITEM cRows, const HROW rghRows[],
																	 DBCOUNTITEM *pcRows, HROW **prgRows, DBROWSTATUS **prgRowStatus)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowset::Update\n");

	ClearError();

	if(m_nStatus == 1)
		return RaiseError(E_UNEXPECTED, 1, __uuidof(IRowsetUpdate), CLocale::Msg(CLocale::ID_OBJECT_ZOMBIE_STATE));

	CHECK_RESTART(__uuidof(IRowsetUpdate));

	DBCOUNTITEM ulRows = 0;
	bool bNotIgnore = true;

	// the following lines are used to fix the two _alloca calls below.  Those calls are risky
	// because we may be allocating huge amounts of data.  So instead I'll allocate that data on heap.
	// But if you use _alloca you don't have to worry about cleaning this memory.  So we will use these
	// temporary variables to allocate memory on heap.  As soon as we exit the function, the memory will
	// be cleaned up, just as if we were using alloca. So now, instead of calling alloca, I'll alloc
	// memory on heap using the two smnart pointers below, and then assing it to the actual pointers.
	CHeapPtr<HROW> spTempRows;
	CHeapPtr<DBROWSTATUS> spTempRowStatus;

	if(cRows || pcRows)
	{
		if(prgRows)
			*prgRows = NULL;
		if(prgRowStatus)
			*prgRowStatus = NULL;
	}
	else
	{
		bNotIgnore = false;	//Don't do status or row arrays
	}

	// Check to see how many changes we'll undo
	if(pcRows)
	{
		*pcRows = NULL;
		if(prgRows == NULL)
			return E_INVALIDARG;
	}

	if(cRows)
	{
		if(rghRows == NULL)
			return E_INVALIDARG;
		ulRows = cRows;
	}
	else
		ulRows = (DBCOUNTITEM)m_rgRowHandles.GetCount();

	int hConn;
	HRESULT hr = GetConnectionHandle(&hConn);
	if(FAILED(hr))
		return E_FAIL;

	// NULL out pointers
	{
		if(prgRows && ulRows && bNotIgnore)
		{
			// Make a temporary buffer as we may not fill up everything
			// in the case where cRows == 0
			if(cRows)
				*prgRows = (HROW*)CoTaskMemAlloc(ulRows * sizeof(HROW));
			else
			{
				spTempRows.Allocate(ulRows);
				*prgRows = spTempRows;
			}
			if (*prgRows == NULL)
				return E_OUTOFMEMORY;
		}

		if(prgRowStatus && ulRows && bNotIgnore)
		{
			if(cRows)
				*prgRowStatus = (DBROWSTATUS*)CoTaskMemAlloc(ulRows * sizeof(DBROWSTATUS));
			else
			{
				spTempRowStatus.Allocate(ulRows);
				*prgRowStatus = spTempRowStatus;
			}
			if(*prgRowStatus == NULL)
			{
				if(cRows) CoTaskMemFree(*prgRows);
				*prgRows = NULL;

				return E_OUTOFMEMORY;
			}
		}
	}

	bool bSucceeded = false;
	bool bFailed = false;
	ULONG ulCount = 0;
	POSITION pos = m_rgRowHandles.GetStartPosition();
	for (ULONG ulRow = 0; ulRow < ulRows; ulRow++)
	{
		ULONG ulCurrentRow = ulCount;
		bool bDupRow = false;
		ULONG ulAlreadyProcessed = 0;

		HROW hRowUpdate = NULL;
		{
			if(cRows)
			{
				hRowUpdate = rghRows[ulRow];
				for (ULONG ulCheckDup = 0; ulCheckDup < ulRow; ulCheckDup++)
				{
					if (hRowUpdate == rghRows[ulCheckDup] ||
						IsSameRow(hRowUpdate, rghRows[ulCheckDup]) == S_OK)
					{
						ulAlreadyProcessed = ulCheckDup;
						bDupRow = true;
						break;
					}
				}
			}
			else
			{
				ATLASSERT( pos != NULL );
				MapClass::CPair* pPair = m_rgRowHandles.GetNext(pos);
				ATLASSERT( pPair != NULL );
				hRowUpdate = pPair->m_key;
			}
		}

		if(prgRows && bNotIgnore)
			(*prgRows)[ulCurrentRow] = hRowUpdate;

		if(bDupRow)
		{
			// We've already set the row before, just copy status and
			// continue processing
			if(prgRowStatus && bNotIgnore)
				(*prgRowStatus)[ulCurrentRow] = (*prgRowStatus)[ulAlreadyProcessed];
			ulCount++;
			continue;
		}

		// Fetch the RowClass and determine if it is valid
		CCUBRIDRowsetRow *pRow;
		{
			bool bFound = m_rgRowHandles.Lookup((ULONG)hRowUpdate, pRow);
			if (!bFound || pRow == NULL)
			{
				if (prgRowStatus && bNotIgnore)
					(*prgRowStatus)[ulCurrentRow] = DBROWSTATUS_E_INVALID;
				bFailed = true;
				ulCount++;
				continue;
			}
		}

		// If cRows is zero we'll go through all rows fetched.  We
		// shouldn't increment the attempted count for rows that are
		// not changed
		if (cRows != 0 || (pRow != NULL &&
			pRow->m_status != 0 && pRow->m_status != DBPENDINGSTATUS_UNCHANGED
			&& pRow->m_status != DBPENDINGSTATUS_INVALIDROW))
			ulCount++;
		else
			continue;

		if(cRows == 0)
			pRow->AddRefRow();

		switch (pRow->m_status)
		{
		case DBPENDINGSTATUS_INVALIDROW:	// Row is bad or deleted
			{
				if(prgRowStatus && bNotIgnore)
					(*prgRowStatus)[ulCurrentRow] = DBROWSTATUS_E_DELETED;
				bFailed = true;
			}
			break;
		case DBPENDINGSTATUS_UNCHANGED:
		case 0:
			{
				// If the row's status is not changed, then just put S_OK
				// and continue.  The spec says we should not transmit the
				// request to the data source (as nothing would change).
				if(prgRowStatus && bNotIgnore)
					(*prgRowStatus)[ulCurrentRow] = DBROWSTATUS_S_OK;
				bSucceeded = true;
			}
			break;
		default:
			{
				DBORDINAL cCols;
				ATLCOLUMNINFO *pColInfo = GetColumnInfo(this, &cCols);
				HRESULT hr = pRow->WriteData(hConn, GetRequestHandle(), m_strTableName, NULL, NULL);
				if(FAILED(hr))
				{
					DBROWSTATUS stat = DBROWSTATUS_E_FAIL;
					if(hr == DB_E_INTEGRITYVIOLATION)
						stat = DBROWSTATUS_E_INTEGRITYVIOLATION;
					if(prgRowStatus && bNotIgnore)
						(*prgRowStatus)[ulCurrentRow] = stat;
					bFailed = true;
				}
				else
				{
					//if(pRow->m_status==DBPENDINGSTATUS_NEW)
					//{
					//	CCUBRIDRowsetRow::KeyType key = pRow->m_iRowset;
					//	POSITION pos = m_rgRowHandles.GetStartPosition();
					//	while(pos)
					//	{
					//		CCUBRIDRowset::MapClass::CPair *pPair = m_rgRowHandles.GetNext(pos);
					//		ATLASSERT(pPair);
					//		CCUBRIDRowsetRow *pCheckRow = pPair->m_value;
					//		if( pCheckRow && pCheckRow->m_iRowset < key )
					//		{
					//			if(pCheckRow->m_iRowset<pRow->m_iRowset)
					//				pRow->m_iRowset = pCheckRow->m_iRowset;
					//			pCheckRow->m_iRowset++;
					//		}
					//	}
					//	pRow->ReadData(GetRequestHandle(), true);
					//}

					if(pRow->m_status == DBPENDINGSTATUS_DELETED)
						MakeRowInvalid(this, pRow);
					else
						pRow->m_status = DBPENDINGSTATUS_UNCHANGED;

					if(prgRowStatus && bNotIgnore)
						(*prgRowStatus)[ulCurrentRow] = DBROWSTATUS_S_OK;
					bSucceeded = true;

					// Check if we need to release the row because it's ref was 0
					// See the IRowset::ReleaseRows section in the spec for more
					// information
					if (pRow->m_dwRef == 0)
					{
						pRow->AddRefRow();	// Artifically bump this to remove it
						if( FAILED( RefRows(1, &hRowUpdate, NULL, NULL, false) ) )
							return E_FAIL;
					}
				}
			}
			break;
		}
	}

	// Set the output for rows undone.
	if(pcRows)
		*pcRows = ulCount;

	if(ulCount == 0)
	{
		if(prgRows)
		{
			CoTaskMemFree(*prgRows);
			*prgRows = NULL;
		}

		if(prgRowStatus)
		{
			CoTaskMemFree(*prgRowStatus);
			*prgRowStatus = NULL;
		}
	}
	else if(cRows == 0)
	{
		// In the case where cRows == 0, we need to allocate the final array of data.
		if(prgRows && bNotIgnore)
		{
			HROW *prgRowsTemp = (HROW *)CoTaskMemAlloc(ulCount * sizeof(HROW));
			if(prgRowsTemp == NULL)
				return E_OUTOFMEMORY;
			memcpy(prgRowsTemp, *prgRows, ulCount * sizeof(HROW));
			*prgRows = prgRowsTemp;
		}

		if(prgRowStatus && bNotIgnore)
		{
			DBROWSTATUS *prgRowStatusTemp = (DBROWSTATUS *)CoTaskMemAlloc(ulCount * sizeof(DBROWSTATUS));
			if(prgRowStatusTemp == NULL)
			{
				CoTaskMemFree(*prgRows);
				*prgRows = NULL;

				return E_OUTOFMEMORY;
			}
			memcpy(prgRowStatusTemp, *prgRowStatus, ulCount * sizeof(DBROWSTATUS));
			*prgRowStatus = prgRowStatusTemp;
		}
	}

	DoCommit(this);

	// Send the return value
	if(!bFailed)
		return S_OK;
	else
	{
		return bSucceeded ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED;
	}
}
