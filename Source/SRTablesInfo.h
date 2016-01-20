////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

class CTablesInfoRow
{
public:
	WCHAR m_szTableCatalog[1];
	WCHAR m_szTableSchema[1];
	WCHAR m_szTableName[256];
	WCHAR m_szTableType[16];
	GUID m_tableGUID;
	VARIANT_BOOL m_bBookmarks;
	int m_bBookmarkType;
	short m_bBookmarkDatatype;
	ULONG m_bBookmarkMaximumLength;
	ULONG m_bBookmarkInformation;
	ULARGE_INTEGER m_tableVersion;
	ULARGE_INTEGER m_ulCardinality;
	WCHAR m_szDescription[1];
	ULONG m_bTablePropID;

	CTablesInfoRow()
	{
		memset(this, 0, sizeof(*this));	
	}

	BEGIN_PROVIDER_COLUMN_MAP(CTablesInfoRow)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_CATALOG", 1, m_szTableCatalog)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_SCHEMA", 2, m_szTableSchema)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_NAME", 3, m_szTableName)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_TYPE", 4, m_szTableType)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("TABLE_GUID", 5, DBTYPE_GUID, 0xFF, 0xFF, m_tableGUID)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("BOOKMARKS", 6, DBTYPE_BOOL, 0xFF, 0xFF, m_bBookmarks)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("BOOKMARK_TYPE", 7, DBTYPE_I4, 10, ~0, m_bBookmarkType)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("BOOKMARK_DATATYPE", 8, DBTYPE_UI2, 5, ~0, m_bBookmarkDatatype)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("BOOKMARK_MAXIMUM_LENGTH", 9, DBTYPE_UI4, 10, ~0, m_bBookmarkMaximumLength)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("BOOKMARK_INFORMATION", 10, DBTYPE_UI4, 10, ~0, m_bBookmarkInformation)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("TABLE_VERSION", 11, DBTYPE_I8, 19, ~0, m_tableVersion)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("CARDINALITY", 12, DBTYPE_UI8, 20, ~0, m_ulCardinality)
		PROVIDER_COLUMN_ENTRY_WSTR("DESCRIPTION", 13, m_szDescription)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("TABLE_PROPID", 14, DBTYPE_UI4, 10, ~0, m_bTablePropID)
	END_PROVIDER_COLUMN_MAP()
};

class CSRTablesInfo :
	public CSchemaRowsetImpl<CSRTablesInfo, CTablesInfoRow, CCUBRIDSession>
{
public:
	~CSRTablesInfo();

	SR_PROPSET_MAP(CSRTablesInfo);

	HRESULT Execute(LONG* pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions);
	DBSTATUS GetDBStatus(CSimpleRow*, ATLCOLUMNINFO* pInfo);
};
