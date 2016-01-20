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

// Session.h : Declaration of the CCUBRIDSession

#pragma once

#include "CUBRIDProvider.h"
#include "resource.h"       // main symbols
#include "Command.h"
#include "util.h"

class CCUBRIDDataSource;

// SR = Schema Rowset
class CSRTables;
class CSRColumns;
class CSRProviderTypes;
class CSRTablePrivileges;
class CSRColumnPrivileges;
class CSRTableConstraints;
class CSRTablesInfo;
class CSRStatistics;
class CSRIndexes;
class CSRViewColumnUsage;
class CSRViews;
class CConnectionProperties;

// CCUBRIDSession
[
	coclass,
	noncreatable,
	uuid("F4CD8484-A670-4511-8DF5-F77B2942B985"),
	threading(apartment),
	registration_script("none")
]
class ATL_NO_VTABLE CCUBRIDSession : 
	public IGetDataSourceImpl<CCUBRIDSession>,
	public IOpenRowsetImpl<CCUBRIDSession>,
	public ISessionPropertiesImpl<CCUBRIDSession>,
	public IObjectWithSiteSessionImpl<CCUBRIDSession>,
	public IDBSchemaRowsetImpl<CCUBRIDSession>,
	public IDBCreateCommandImpl<CCUBRIDSession, CCUBRIDCommand>,
	public ITransactionLocal,
	public ITableDefinition,
	public IIndexDefinition,
	public ICUBRIDSession
{
public:
	CCUBRIDDataSource *GetDataSourcePtr();
	static CCUBRIDSession *GetSessionPtr(IObjectWithSite *pSite);

private:
	int m_hConn;
	UINT m_uCodepage;
public:
	int GetConnection();
	UINT GetCodepage();

	CCUBRIDSession() : m_hConn(0), m_uCodepage(_AtlGetConversionACP())
	{
		ATLTRACE2(atlTraceDBProvider, 3, "CCUBRIDSession::CCUBRIDSession\n");

		m_bAutoCommit = true;
	}

	~CCUBRIDSession()
	{
		ATLASSERT(m_grpTxnCallbacks.GetCount()==0);
		ATLTRACE2(atlTraceDBProvider, 3, "CCUBRIDSession::~CCUBRIDSession\n");
	}

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct();
	void FinalRelease();

	// ITransactionLocal
private:
	ISOLEVEL m_isoLevel;
	bool m_bAutoCommit;
	CAtlList<Util::ITxnCallback *> m_grpTxnCallbacks;

	HRESULT DoCASCCICommit(bool bCommit);
	HRESULT SetCASCCIIsoLevel(ISOLEVEL isoLevel, bool bCheckOnly=false);
	void EnterAutoCommitMode();
public:
	HRESULT AutoCommit(const Util::ITxnCallback *pOwner);
	HRESULT RowsetCommit();
	HRESULT RegisterTxnCallback(Util::ITxnCallback *pTxnCallback, bool bRegister);
	STDMETHOD(Connect)();
	STDMETHOD(GetOptionsObject)(ITransactionOptions **ppOptions);
	STDMETHOD(StartTransaction)(ISOLEVEL isoLevel, ULONG isoFlags,
				ITransactionOptions *pOtherOptions, ULONG *pulTransactionLevel);
    STDMETHOD(Commit)(BOOL fRetaining, DWORD grfTC, DWORD grfRM);
	STDMETHOD(Abort)(BOID *pboidReason, BOOL fRetaining, BOOL fAsync);
	STDMETHOD(GetTransactionInfo)(XACTTRANSINFO *pinfo);

	// IOpenRowset
	STDMETHOD(OpenRowset)(IUnknown *pUnk, DBID *pTID, DBID *pInID, REFIID riid,
						ULONG cSets, DBPROPSET rgSets[], IUnknown **ppRowset);

	// ISessionProperties
	virtual HRESULT	IsValidValue(ULONG /*iCurSet*/, DBPROP* pDBProp);
	virtual HRESULT OnPropertyChanged(ULONG /*iCurSet*/, DBPROP *pDBProp);


	//ITableDefinition
	STDMETHOD(CreateTable)(IUnknown *pUnkOuter,
            DBID *pTableID,
            DBORDINAL cColumnDescs,
            const DBCOLUMNDESC rgColumnDescs[],
            REFIID riid,
            ULONG cPropertySets,
            DBPROPSET rgPropertySets[],
            DBID **ppTableID,
            IUnknown **ppRowset);
    STDMETHOD(DropTable)(DBID *pTableID);
    STDMETHOD(AddColumn)(DBID *pTableID, DBCOLUMNDESC *pColumnDesc, DBID **ppColumnID);
    STDMETHOD(DropColumn)(DBID *pTableID, DBID *pColumnID);

	//IIndexDefinition
	STDMETHOD(CreateIndex)( 
            DBID *pTableID,
            DBID *pIndexID,
            DBORDINAL cIndexColumnDescs,
            const DBINDEXCOLUMNDESC rgIndexColumnDescs[],
            ULONG cPropertySets,
            DBPROPSET rgPropertySets[],
            DBID **ppIndexID);
    STDMETHOD(DropIndex)( 
            DBID *pTableID,
            DBID *pIndexID);


BEGIN_PROPSET_MAP(CCUBRIDSession)
	BEGIN_PROPERTY_SET(DBPROPSET_SESSION)
		PROPERTY_INFO_ENTRY_VALUE(SESS_AUTOCOMMITISOLEVELS, DBPROPVAL_TI_READCOMMITTED)
	END_PROPERTY_SET(DBPROPSET_SESSION)
END_PROPSET_MAP()

	// IDBSchemaRowset
	void SetRestrictions(ULONG cRestrictions, GUID* rguidSchema, ULONG* rgRestrictions);
	HRESULT CheckRestrictions(REFGUID rguidSchema, ULONG cRestrictions, 
				const VARIANT rgRestrictions[]);

BEGIN_SCHEMA_MAP(CCUBRIDSession)
	SCHEMA_ENTRY(DBSCHEMA_TABLES, CSRTables)
	SCHEMA_ENTRY(DBSCHEMA_COLUMNS, CSRColumns)
	SCHEMA_ENTRY(DBSCHEMA_PROVIDER_TYPES, CSRProviderTypes)
	SCHEMA_ENTRY(DBSCHEMA_TABLE_PRIVILEGES, CSRTablePrivileges)
	SCHEMA_ENTRY(DBSCHEMA_COLUMN_PRIVILEGES, CSRColumnPrivileges)
	SCHEMA_ENTRY(DBSCHEMA_TABLE_CONSTRAINTS, CSRTableConstraints)
	SCHEMA_ENTRY(DBSCHEMA_TABLES_INFO, CSRTablesInfo)
	SCHEMA_ENTRY(DBSCHEMA_STATISTICS, CSRStatistics)
	SCHEMA_ENTRY(DBSCHEMA_INDEXES, CSRIndexes)
	SCHEMA_ENTRY(DBSCHEMA_VIEW_COLUMN_USAGE, CSRViewColumnUsage)
	SCHEMA_ENTRY(DBSCHEMA_VIEWS, CSRViews)
END_SCHEMA_MAP()
};

#define SR_PROPSET_MAP(Class)						\
BEGIN_PROPSET_MAP(Class)							\
	BEGIN_PROPERTY_SET(DBPROPSET_ROWSET)			\
		PROPERTY_INFO_ENTRY(IAccessor)				\
		PROPERTY_INFO_ENTRY(IColumnsInfo)			\
		PROPERTY_INFO_ENTRY(IConvertType)			\
		PROPERTY_INFO_ENTRY(IRowset)				\
		PROPERTY_INFO_ENTRY(IRowsetIdentity)		\
		PROPERTY_INFO_ENTRY(IRowsetInfo)			\
		PROPERTY_INFO_ENTRY(CANFETCHBACKWARDS)		\
		PROPERTY_INFO_ENTRY(CANHOLDROWS)			\
		PROPERTY_INFO_ENTRY(CANSCROLLBACKWARDS)		\
		PROPERTY_INFO_ENTRY_VALUE(MAXOPENROWS, 0)	\
		PROPERTY_INFO_ENTRY_VALUE(MAXROWS, 0)		\
		/* �� �ڴ� LTM�� ���� �Ӽ� */								\
		/* LTM�� CCUBRIDDataSource�� ���� �Ӽ��� ��´� */				\
		/* ���� Schema Rowset�� �ƴ� */							\
		/* CCUBRIDCommand�� ���ǵ� �Ӽ��� ���� �׽�Ʈ�� �����Ѵ� */	\
		PROPERTY_INFO_ENTRY_VALUE      (ABORTPRESERVE, ATL_VARIANT_FALSE) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(ACCESSORDER, DBPROPVAL_AO_RANDOM, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(BOOKMARKSKIPPED, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE      (CHANGEINSERTEDROWS, ATL_VARIANT_TRUE) \
		PROPERTY_INFO_ENTRY            (COLUMNRESTRICT) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(COMMITPRESERVE, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE      (IColumnsRowset, ATL_VARIANT_FALSE) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(IGetRow, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(IGetSession, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(IMMOBILEROWS, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(IMultipleResults, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(IRowsetFind, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(IRowsetRefresh, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE      (ISequentialStream, ATL_VARIANT_FALSE) \
		PROPERTY_INFO_ENTRY_VALUE	   (ISupportErrorInfo, ATL_VARIANT_FALSE) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(LITERALBOOKMARKS, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(LITERALIDENTITY, ATL_VARIANT_TRUE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(ORDEREDBOOKMARKS, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(OTHERINSERT, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(OTHERUPDATEDELETE, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(OWNINSERT, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(REMOVEDELETED, ATL_VARIANT_FALSE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(SERVERDATAONINSERT, ATL_VARIANT_TRUE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
		PROPERTY_INFO_ENTRY            (STRONGIDENTITY) \
		PROPERTY_INFO_ENTRY_VALUE_FLAGS(UPDATABILITY, DBPROPVAL_UP_CHANGE|DBPROPVAL_UP_INSERT|DBPROPVAL_UP_DELETE, DBPROPFLAGS_ROWSET | DBPROPFLAGS_READ) \
	END_PROPERTY_SET(DBPROPSET_ROWSET)				\
END_PROPSET_MAP()

#include "SRTables.h"
#include "SRColumns.h"
#include "SRProviderTypes.h"
#include "SRTablePrivileges.h"
#include "SRColumnPrivileges.h"
#include "SRTableConstraints.h"
#include "SRTablesInfo.h"
#include "SRStatistics.h"
#include "SRIndexes.h"
#include "SRViewColumnUsage.h"
#include "SRViews.h"
#include "SRPrimaryKeys.h"
#include "SRKeyColumnUsage.h"
