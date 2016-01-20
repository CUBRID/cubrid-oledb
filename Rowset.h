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

// Rowset.h : Declaration of the CCUBRIDRowset

#pragma once

#include "resource.h"       // main symbols
#include "command.h"
#include "util.h"
#include "ColumnsRowset.h" // IColumnsRowset ����
#include "RowsetRow.h" // RowClass�� �����Ѵ�.

class CCUBRIDDataSource;
class CCUBRIDSession;
class CCUBRIDCommand;

/*
 * CRowsetImpl�� ���� ����������
 * IOpenRowset::OpenRowset(CCUBRIDSession) Ȥ�� ICommand::Execute(CCUBRIDCommand)����
 * CreateRowset�� ȣ���ϰ� �� �ȿ��� CCUBRIDRowset::Execute�� �ҷ�����. �׷���,
 * CCUBRIDRowset::Execute������ Storage Type�� �迭�� m_rgRowData�� ��� �����͸� �ִ´�.
 * �׷��� Accessor�� �����ϰ� �����͸� Consumer���� �ѱ�� ���� ���� CRowsetImpl��
 * �� �����͸� ���� �ڵ����� �Ѵ�.
 * Schema Rowset�� ���������� �� ������ �״�� ����Ѵ�.
 *
 * Rowset�� ��쿡�� Schema Rowset�� ���� ������ Storage�� ����
 * ������ �Ѵٰ� �ص� ��� �����͸� �޸𸮿� ���� ���� �� ���� ��쵵 �ֱ� ������
 * �״�� ����� �� ����(Schema Rowset�� ������ ���� ���� ��찡 ���ٰ� �����Ѵ�).
 * �׷��� ������ ���� �����ؼ� �����ߴ�.
 *
 * Stroage�� CDummy�� ������ �ʴ´�.
 * Array of Storage(m_rgRowData)�� �ܼ��� �������� ������ ���ؼ��� �����Ѵ�.
 * ���� �Լ����� ���ȴ�(�������� m_rgRowData�� ������� �ʵ��� �籸��)
 *		IRowsetImpl::GetNextRowsSkipDeleted
 *		IRowsetImpl::GetNextRows
 *		IRowsetLocateImpl::GetRowsAt
 * ���� �����ʹ� GetNextRows�� �����ͼ� RowClass�� �����ϰ�
 * ReleaseRows���� ��������.
 */

// ���� ��ȯ�ϰ� �ٽ� üũ�ϴ� ���� ���ϱ� ����
// Ŭ������ ����� �ƴ� ��ũ�η� �������.
// TODO: IRowsetImpl::RestartPosition������ row handle�� ���µ�
// �˻��� CHANGED�� ���� ��ȯ�ϴµ� ������� �´� ���� �𸣰ڴ�.
#define CHECK_CANHOLDROWS(iid)														\
	do																				\
	{																				\
		CComVariant varHoldRows;													\
		GetPropValue(&DBPROPSET_ROWSET, DBPROP_CANHOLDROWS, &varHoldRows);			\
		if(V_BOOL(&varHoldRows)==ATL_VARIANT_FALSE && m_rgRowHandles.GetCount()>0)	\
			return RaiseError(DB_E_ROWSNOTRELEASED, 0, iid);						\
	} while(0)

#define CHECK_RESTART(iid)															\
	do																				\
	{	/* ���� �߻� or ���� ����� */												\
		/*if(m_nStatus==2) Reexecute();*/											\
		/*if(m_nStatus==2) return RaiseError(E_FAIL, 1, iid, "You must call IRowset::RestartPosition before this");*/ \
	} while(0)

/*
 * 1. IRowsetChange�� CANHOLDROWS�� exclusive �ϰ�
 *
 * 2. OWNUPDATEDELETE�� OTHERUPDATEDELETE�� ���� ���� ������
 *
 *		CCI ������ ��¿ �� ����. Consumer�� �� �� set �� ���
 *		���߿� set �Ǵ� ���� �ȴ�. CONFLICT�� ���� �Ϸ���
 *		�⺻������ Consumer�� ���� set �� ������ ������ �� �־�� �ϴµ�
 *		�����ϰԴ� �� �� �� �ϴ�.
 *
 *		1�� �⺻���� FALSE�̹Ƿ� TRUE�� ����ڰ� set �� �ɷ� �ؼ� CONFLICT��
 *		�� �� �ִ�.
 *
 * 3. BOOKMARKS�� TRUE�� CANSCROLLBACKWARDS, CANFETCHBACKWARDS�� TRUE��
 *
 * 4. IRowsetLocate�� TRUE�� IRowsetScroll�� TRUE��
 *
 *		�ӽ�. ADO���� IRowsetScroll�� �Լ��� ȣ���ϸ鼭
 *		DBPROP_IRowsetScroll�� ��û���� �ʴ´�.
 *
 * 5. DBPROP_UPDATABILITY, DBPROP_BOOKMARKS�� DBPROP_IRowsetChange, DBPROP_IRowsetLocate��
 *	 CONFLICT���� �ʵ���
 *
 * 6. IRowsetFind�� TRUE�� BOOKMARKS�� TRUE��
 *
 *		LTM�� ���� IRowsetFind �Ӽ��� ������ ��û�ϴ� ���� ���� �� ����. �ϴ� �ּ�ó��.
 */
template <class T>
HRESULT CUBRIDOnPropertyChanged(T *pT, ULONG iCurSet, DBPROP* pDBProp)
{
	ATLASSERT(pDBProp);

	switch(pDBProp->dwPropertyID)
	{
	//case DBPROP_IRowsetChange:
	//case DBPROP_CANHOLDROWS:
	//	{
	//		CComVariant var;
	//		pT->GetPropValue(&DBPROPSET_ROWSET, (pDBProp->dwPropertyID==DBPROP_CANHOLDROWS?
	//						DBPROP_IRowsetChange:DBPROP_CANHOLDROWS), &var);
	//		if(V_BOOL(&pDBProp->vValue)==ATL_VARIANT_TRUE && V_BOOL(&var)==ATL_VARIANT_TRUE)
	//		{
	//			pDBProp->dwStatus = DBPROPSTATUS_CONFLICTING;
	//			var = false;
	//			pT->SetPropValue(&DBPROPSET_ROWSET, pDBProp->dwPropertyID, &var);
	//			return E_FAIL;
	//		}
	//	}
	//	break;

	case DBPROP_OWNUPDATEDELETE:
	case DBPROP_OTHERUPDATEDELETE:
		{
			pT->SetPropValue(&DBPROPSET_ROWSET, (pDBProp->dwPropertyID==DBPROP_OWNUPDATEDELETE?
						DBPROP_OTHERUPDATEDELETE:DBPROP_OWNUPDATEDELETE), &pDBProp->vValue);
		}
		break;

	case DBPROP_UPDATABILITY:
		if(V_I4(&pDBProp->vValue)!=0)
		{	// IRowsetChange�� default FALSE����, set FALSE���� �����ؼ� TRUE ����, ������ �����Ѵ�.
			UPROPVAL* pUPropVal = &(pT->m_pUProp[iCurSet].pUPropVal[pT->GetUPropValIndex(iCurSet, DBPROP_IRowsetChange)]);
			if(V_BOOL(&pUPropVal->vValue)==ATL_VARIANT_FALSE)
			{
				if(pUPropVal->dwFlags & DBINTERNFLAGS_CHANGED)
				{
					pDBProp->dwStatus = DBPROPSTATUS_CONFLICTING;
					CComVariant var = 0;
					pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_UPDATABILITY, &var);
					return E_FAIL;
				}
				else
				{
					CComVariant var = true;
					pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_IRowsetChange, &var);
				}
			}
		}
		return S_OK;

	case DBPROP_BOOKMARKS:
		if(V_BOOL(&pDBProp->vValue)==ATL_VARIANT_TRUE)
		{
			pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_CANFETCHBACKWARDS, &pDBProp->vValue);
			pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_CANSCROLLBACKWARDS, &pDBProp->vValue);
		}
		return S_OK;

	case DBPROP_IRowsetChange:
	case DBPROP_IRowsetUpdate:
		if(V_BOOL(&pDBProp->vValue)==ATL_VARIANT_TRUE)
		{
			VARIANT var;
			var.vt = VT_I4;
			var.lVal = DBPROPVAL_UP_CHANGE|DBPROPVAL_UP_DELETE|DBPROPVAL_UP_INSERT;
			pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_UPDATABILITY, &var);

			if (pDBProp->dwPropertyID == DBPROP_IRowsetUpdate)
			{
				VARIANT tmpVar;
				::VariantInit(&tmpVar);
				tmpVar.vt = VT_BOOL;
				tmpVar.boolVal = VARIANT_TRUE;

				pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_IRowsetChange, &tmpVar);
				::VariantClear(&tmpVar);
			}
		} 
		return S_OK;
	case DBPROP_IRowsetLocate:
		if(V_BOOL(&pDBProp->vValue)==ATL_VARIANT_TRUE)
		{
			// DBPROP_CANSCROLLBACKWARDS�� default FALSE����, set FALSE���� �����ؼ� TRUE ����, ����� �����Ѵ�.
			UPROPVAL* pUPropVal = &(pT->m_pUProp[iCurSet].pUPropVal[pT->GetUPropValIndex(iCurSet, DBPROP_CANSCROLLBACKWARDS)]);
			if(V_BOOL(&pUPropVal->vValue)==ATL_VARIANT_FALSE)
			{
				if(!(pUPropVal->dwFlags & DBINTERNFLAGS_CHANGED))
					pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_CANSCROLLBACKWARDS, &pDBProp->vValue);
			}

			// DBPROP_CANFETCHBACKWARDS�� default FALSE����, set FALSE���� �����ؼ� TRUE ����, ����� �����Ѵ�.
			pUPropVal = &(pT->m_pUProp[iCurSet].pUPropVal[pT->GetUPropValIndex(iCurSet, DBPROP_CANFETCHBACKWARDS)]);
			if(V_BOOL(&pUPropVal->vValue)==ATL_VARIANT_FALSE)
			{
				if(!(pUPropVal->dwFlags & DBINTERNFLAGS_CHANGED))
					pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_CANFETCHBACKWARDS, &pDBProp->vValue);
			}

			pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_IRowsetScroll, &pDBProp->vValue);
		}
		pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_BOOKMARKS, &pDBProp->vValue);
		return S_OK;

	//case DBPROP_IRowsetFind:
	//	if(V_BOOL(&pDBProp->vValue)==ATL_VARIANT_TRUE)
	//	{
	//		pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_BOOKMARKS, &pDBProp->vValue);
	//		pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_CANFETCHBACKWARDS, &pDBProp->vValue);
	//		pT->SetPropValue(&DBPROPSET_ROWSET, DBPROP_CANSCROLLBACKWARDS, &pDBProp->vValue);
	//	}
	//	break;
	}

	return S_FALSE;
}

class CDummy
{
};

class CCUBRIDRowset :
	public CRowsetImpl<CCUBRIDRowset, CDummy, CCUBRIDCommand, CAtlArray<CDummy>, CCUBRIDRowsetRow,
							IRowsetLocateImpl<CCUBRIDRowset, IRowsetScroll, CCUBRIDRowsetRow> >,
	public IColumnsRowsetImpl<CCUBRIDRowset, CCUBRIDCommand>,
	public IGetRow,
	public IRowsetUpdate, // IRowsetChange�� �����Ѵ�.
	public ISupportErrorInfo,
	public IRowsetFind,
	public IRowsetRefresh,
	public Util::ITxnCallback
{
	//===== Rowset Ÿ��
public:
	enum Type { Invalid, FromSession, FromCommand, FromRow } m_eType;

	//===== Helper
public:
	typedef CAtlMap<_HRowClass::KeyType, _HRowClass *> MapClass;

	CCUBRIDDataSource *GetDataSourcePtr();
	CCUBRIDSession *GetSessionPtr();
	CCUBRIDCommand *GetCommandPtr();
	static CCUBRIDRowset *GetRowsetPtr(IObjectWithSite *pSite);

	//===== �ڵ�
private:
	int m_hReq; // FromCommand�� ���� Invalid
	UINT m_uCodepage;

public:
	int GetRequestHandle();

	//===== �������� �ǰ� �ϱ� ���� ����, �Լ�. �ƹ� �ϵ� ���� �ʴ´�.
public:
	bool m_isPrepared; // for IColumnsRowsetImpl
	HRESULT Execute(DBPARAMS *pParams, DBROWCOUNT *pcRowsAffected)
	{
		return S_OK;
	}

	//===== Initialize, Finalize
private:
	HRESULT InitCommon(int cResult, bool bRegist=true);
public:
	CCUBRIDRowset();
	~CCUBRIDRowset();

	// TODO_REMOVE: hReq
	HRESULT ValidateCommandID(DBID* pTableID, DBID* pIndexID); // called by IOpenRowset::OpenRowset
	HRESULT InitFromSession(DBID *pTID, char flag); // called by IOpenRowset::OpenRowset
	HRESULT InitFromCommand(int hReq, UINT uCodepage, int cResult, bool bAsynch=false); // called by ICommand::Execute
//	HRESULT InitFromRow(int hReq, int cResult); // called by IRow::Open

	//===== ��Ÿ ���
private:
	bool m_bAsynch;
	HRESULT Reexecute();
public:
	int m_nStatus; // 0: normal, 1: zombie, 2: need reexecute
	CComBSTR m_strTableName;
	virtual void TxnCallback(const ITxnCallback *pOwner);

	//===== IUnknown
	//   �Ӽ��� FALSE�� �������̽��� ���� QI�� �����ϰ� �Ѵ�.
	static HRESULT WINAPI InternalQueryInterface(void *pThis,
		const _ATL_INTMAP_ENTRY *pEntries, REFIID iid, void **ppvObject)
	{
		struct
		{
			const GUID *iid;
			DBPROPID dwPropId;
		} list[] = {
			{ &__uuidof(IRowsetChange), DBPROP_IRowsetChange },
			{ &__uuidof(IRowsetUpdate), DBPROP_IRowsetUpdate },
			{ &__uuidof(IRowsetLocate), DBPROP_IRowsetLocate },
			{ &__uuidof(IRowsetScroll), DBPROP_IRowsetScroll },
		};
		for(int i=0;i<sizeof(list)/sizeof(*list);i++)
		{
			if( InlineIsEqualGUID(iid, *list[i].iid) )
			{
				CComVariant var;
				((CCUBRIDRowset *)pThis)->GetPropValue(&DBPROPSET_ROWSET, list[i].dwPropId, &var);
				if(V_BOOL(&var)==ATL_VARIANT_FALSE) return E_NOINTERFACE;
			}
		}
		return _RowsetBaseClass::InternalQueryInterface(pThis, pEntries, iid, ppvObject);
	}

	//===== ISupportErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		if( InlineIsEqualGUID(riid, __uuidof(IAccessor)) ||
			InlineIsEqualGUID(riid, __uuidof(IColumnsInfo)) ||
			InlineIsEqualGUID(riid, __uuidof(IColumnsRowset)) ||
			InlineIsEqualGUID(riid, __uuidof(IConvertType)) ||
			InlineIsEqualGUID(riid, __uuidof(IGetRow)) ||
			InlineIsEqualGUID(riid, __uuidof(IRowset)) ||
			InlineIsEqualGUID(riid, __uuidof(IRowsetChange)) ||
			InlineIsEqualGUID(riid, __uuidof(IRowsetIdentity)) ||
			InlineIsEqualGUID(riid, __uuidof(IRowsetInfo)) ||
			InlineIsEqualGUID(riid, __uuidof(IRowsetFind)) ||
			InlineIsEqualGUID(riid, __uuidof(IRowsetLocate)) ||
			InlineIsEqualGUID(riid, __uuidof(IRowsetRefresh)) ||
			InlineIsEqualGUID(riid, __uuidof(IRowsetScroll)) ||
			InlineIsEqualGUID(riid, __uuidof(IRowsetUpdate)) )
			return S_OK;
		else
			return S_FALSE;
	}

	//===== IAccessor
	STDMETHOD(AddRefAccessor)(HACCESSOR hAccessor, DBREFCOUNT *pcRefCount);
	STDMETHOD(CreateAccessor)(DBACCESSORFLAGS dwAccessorFlags, DBCOUNTITEM cBindings,
			const DBBINDING rgBindings[], DBLENGTH cbRowSize,
			HACCESSOR *phAccessor, DBBINDSTATUS rgStatus[]);
	STDMETHOD(GetBindings)(HACCESSOR hAccessor, DBACCESSORFLAGS *pdwAccessorFlags,
			DBCOUNTITEM *pcBindings, DBBINDING **prgBindings);
	STDMETHOD(ReleaseAccessor)(HACCESSOR hAccessor, DBREFCOUNT *pcRefCount);

	//===== IColumnsInfoImpl
private:
	Util::CColumnsInfo m_Columns;
public:
	static ATLCOLUMNINFO* GetColumnInfo(CCUBRIDRowset *pv, DBORDINAL *pcCols);
	STDMETHOD(GetColumnDefaultValue)(CCUBRIDRowset* pv);

	STDMETHOD(GetColumnInfo)(DBORDINAL *pcColumns, DBCOLUMNINFO **prgInfo,
							OLECHAR **ppStringsBuffer);
	STDMETHOD(MapColumnIDs)(DBORDINAL cColumnIDs, const DBID rgColumnIDs[],
							DBORDINAL rgColumns[]);

	//===== IConvertType
	STDMETHOD(CanConvert)(DBTYPE wFromType, DBTYPE wToType, DBCONVERTFLAGS dwConvertFlags);

	//===== IRowset
private:
	HRESULT GetNextRowsAsynch(HCHAPTER hReserved, DBROWOFFSET lRowsOffset,
						DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW **prghRows);
public:
	HRESULT CreateRow(DBROWOFFSET lRowsOffset, DBCOUNTITEM &cRowsObtained, HROW *rgRows);
	STDMETHOD(AddRefRows)(DBCOUNTITEM cRows, const HROW rghRows[],
						DBREFCOUNT rgRefCounts[], DBROWSTATUS rgRowStatus[]);
	STDMETHOD(GetData)(HROW hRow, HACCESSOR hAccessor, void *pDstData);
	STDMETHOD(GetNextRows)(HCHAPTER hReserved, DBROWOFFSET lRowsOffset,
						DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW **prghRows);
	STDMETHOD(ReleaseRows)(DBCOUNTITEM cRows, const HROW rghRows[],
						DBROWOPTIONS rgRowOptions[], DBREFCOUNT rgRefCounts[],
						DBROWSTATUS rgRowStatus[]);
	STDMETHOD(RestartPosition)(HCHAPTER /*hReserved*/);

	//===== IRowsetChange : defined in RowsetChange.cpp
	STDMETHOD(DeleteRows)(HCHAPTER hReserved, DBCOUNTITEM cRows,
				const HROW rghRows[], DBROWSTATUS rgRowStatus[]);
	STDMETHOD(SetData)(HROW hRow, HACCESSOR hAccessor, void *pData);
	STDMETHOD(InsertRow)(HCHAPTER hReserved, HACCESSOR hAccessor, void *pData, HROW *phRow);

	//===== IRowsetUpdate : defined in RowsetChange.cpp
	STDMETHOD(GetOriginalData)(HROW hRow, HACCESSOR hAccessor, void *pData);
	STDMETHOD(GetPendingRows)(HCHAPTER hReserved, DBPENDINGSTATUS dwRowStatus,
				DBCOUNTITEM *pcPendingRows, HROW **prgPendingRows,
				DBPENDINGSTATUS **prgPendingStatus);
	STDMETHOD(GetRowStatus)(HCHAPTER hReserved, DBCOUNTITEM cRows,
				const HROW rghRows[], DBPENDINGSTATUS rgPendingStatus[]);
	STDMETHOD(Undo)(HCHAPTER hReserved, DBCOUNTITEM cRows, const HROW rghRows[],
				DBCOUNTITEM *pcRowsUndone, HROW **prgRowsUndone, DBROWSTATUS **prgRowStatus);
	STDMETHOD(Update)(HCHAPTER hReserved, DBCOUNTITEM cRows, const HROW rghRows[],
				DBCOUNTITEM *pcRows, HROW **prgRows, DBROWSTATUS **prgRowStatus);

	//===== IRowsetIdentity
	STDMETHOD(IsSameRow)(HROW hThisRow, HROW hThatRow);

	//===== IRowsetInfo
	virtual HRESULT OnPropertyChanged(ULONG iCurSet, DBPROP* pDBProp);
	virtual HRESULT IsValidValue(ULONG iCurSet, DBPROP* pDBProp);
	STDMETHOD(GetProperties)(const ULONG cPropertyIDSets, const DBPROPIDSET rgPropertyIDSets[],
					ULONG *pcPropertySets, DBPROPSET **prgPropertySets);
	STDMETHOD(GetReferencedRowset)(DBORDINAL iOrdinal, REFIID riid,
					IUnknown **ppReferencedRowset);
	STDMETHOD(GetSpecification)(REFIID riid, IUnknown **ppSpecification);

	//===== IGetRow
	STDMETHOD(GetRowFromHROW)(IUnknown *pUnkOuter, HROW hRow, REFIID riid, IUnknown **ppUnk);
	STDMETHOD(GetURLFromHROW)(HROW hRow, LPOLESTR *ppwszURL);

	//===== IRowsetFind
private:
	// GetNextRows�� cRows==0�̸� �׳� ������ ������
	// FindNextRow�� ���� ã�Ҵ� �������� ã�ƺ��� �Ѵ�.
	bool m_bFindForward; // true: forward, false: backward
public:
	STDMETHOD(FindNextRow)(HCHAPTER hChapter, HACCESSOR hAccessor, void *pFindValue,
				DBCOMPAREOP CompareOp, DBBKMARK cbBookmark, const BYTE *pBookmark,
				DBROWOFFSET lRowsOffset, DBROWCOUNT cRows,
				DBCOUNTITEM *pcRowsObtained, HROW **prghRows);

	//===== IRowsetRefresh
	STDMETHOD(RefreshVisibleData)(HCHAPTER hChapter, DBCOUNTITEM cRows,
				const HROW rghRows[], BOOL fOverWrite, DBCOUNTITEM *pcRowsRefreshed,
				HROW **prghRowsRefreshed, DBROWSTATUS **prgRowStatus);
	STDMETHOD(GetLastVisibleData)(HROW hRow, HACCESSOR hAccessor, void *pData);

	//===== IRowsetLocate
	STDMETHOD(Compare)(HCHAPTER hReserved, DBBKMARK cbBookmark1, const BYTE *pBookmark1,
				DBBKMARK cbBookmark2, const BYTE *pBookmark2, DBCOMPARE *pComparison);
	STDMETHOD(GetRowsAt)(HWATCHREGION hReserved1, HCHAPTER hReserved2,
				DBBKMARK cbBookmark, const BYTE *pBookmark, DBROWOFFSET lRowsOffset,
				DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW **prghRows);
	STDMETHOD(GetRowsByBookmark)(HCHAPTER hReserved, DBCOUNTITEM cRows,
				const DBBKMARK rgcbBookmarks[], const BYTE *rgpBookmarks[],
				HROW rghRows[], DBROWSTATUS rgRowStatus[]);
	STDMETHOD(Hash)(HCHAPTER hReserved, DBBKMARK cBookmarks,
				const DBBKMARK rgcbBookmarks[], const BYTE *rgpBookmarks[],
				DBHASHVALUE rgHashedValues[], DBROWSTATUS rgBookmarkStatus[]);

	//===== IRowsetScroll
	STDMETHOD(GetApproximatePosition)(HCHAPTER hReserved, DBBKMARK cbBookmark,
				const BYTE *pBookmark, DBCOUNTITEM *pulPosition, DBCOUNTITEM *pcRows);
	STDMETHOD(GetRowsAtRatio)(HWATCHREGION hReserved1, HCHAPTER hReserved2,
				DBCOUNTITEM ulNumerator, DBCOUNTITEM ulDenominator,
				DBROWCOUNT cRows, DBCOUNTITEM *pcRowsObtained, HROW **prghRows);

BEGIN_COM_MAP(CCUBRIDRowset)
	COM_INTERFACE_ENTRY(IGetRow)
	COM_INTERFACE_ENTRY(IColumnsRowset)
	COM_INTERFACE_ENTRY(IRowsetLocate)
	COM_INTERFACE_ENTRY(IRowsetScroll)
	COM_INTERFACE_ENTRY(IRowsetChange)
	COM_INTERFACE_ENTRY(IRowsetUpdate)
	COM_INTERFACE_ENTRY(IRowsetFind)
	COM_INTERFACE_ENTRY(IRowsetRefresh)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_CHAIN(_RowsetBaseClass)
END_COM_MAP()
};
