////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

class CIndexesRow
{
public:
	WCHAR m_szTableCatalog[1];
	WCHAR m_szTableSchema[1];
	WCHAR m_szTableName[256];
	WCHAR m_szIndexCatalog[1];
	WCHAR m_szIndexSchema[1];
	WCHAR m_szIndexName[256];
	VARIANT_BOOL m_bPrimaryKey;
	VARIANT_BOOL m_bUnique;
	VARIANT_BOOL m_bClustered;
	USHORT m_uType;
	int m_FillFactor;
	int m_InitialSize;
	int m_Nulls;
	VARIANT_BOOL m_bSortBookmarks;
	VARIANT_BOOL m_bAutoUpdate;
	int m_NullCollation;
	UINT m_uOrdinalPosition;
	WCHAR m_szColumnName[256];
	GUID m_ColumnGUID;
	UINT m_uColumnPropID;
	short m_Collation;
	ULARGE_INTEGER m_ulCardinality;
	int m_Pages;
	WCHAR m_szFilterCondition[1];
	VARIANT_BOOL m_bIntegrated;

	CIndexesRow()
	{
		memset(this, 0, sizeof(*this));	
	}

	BEGIN_PROVIDER_COLUMN_MAP(CIndexesRow)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_CATALOG", 1, m_szTableCatalog)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_SCHEMA", 2, m_szTableSchema)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_NAME", 3, m_szTableName)
		PROVIDER_COLUMN_ENTRY_WSTR("INDEX_CATALOG", 4, m_szIndexCatalog)
		PROVIDER_COLUMN_ENTRY_WSTR("INDEX_SCHEMA", 5, m_szIndexSchema)
		PROVIDER_COLUMN_ENTRY_WSTR("INDEX_NAME", 6, m_szIndexName)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("PRIMARY_KEY", 7, DBTYPE_BOOL, 0xFF, 0xFF, m_bPrimaryKey)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("UNIQUE", 8, DBTYPE_BOOL, 0xFF, 0xFF, m_bUnique)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("CLUSTERED", 9, DBTYPE_BOOL, 0xFF, 0xFF, m_bClustered)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("TYPE", 10, DBTYPE_UI2, 5, ~0, m_uType)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("FILL_FACTOR", 11, DBTYPE_I4, 10, ~0, m_FillFactor)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("INITIAL_SIZE", 12, DBTYPE_I4, 10, ~0, m_InitialSize)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("NULLS", 13, DBTYPE_I4, 10, ~0, m_Nulls)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("SORT_BOOKMARKS", 14, DBTYPE_BOOL, 0xFF, 0xFF, m_bSortBookmarks)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("AUTO_UPDATE", 15, DBTYPE_BOOL, 0xFF, 0xFF, m_bAutoUpdate)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("NULL_COLLATION", 16, DBTYPE_I4, 10, ~0, m_NullCollation)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("ORDINAL_POSITION", 17, DBTYPE_UI4, 10, ~0, m_uOrdinalPosition)
		PROVIDER_COLUMN_ENTRY_WSTR("COLUMN_NAME", 18, m_szColumnName)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("COLUMN_GUID", 19, DBTYPE_GUID, 0xFF, 0xFF, m_ColumnGUID)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("COLUMN_PROPID", 20, DBTYPE_UI4, 10, ~0, m_uColumnPropID)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("COLLATION", 21, DBTYPE_I2, 5, ~0, m_Collation)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("CARDINALITY", 22, DBTYPE_UI8, 20, ~0, m_ulCardinality)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("PAGES", 23, DBTYPE_I4, 10, ~0, m_Pages)
		PROVIDER_COLUMN_ENTRY_WSTR("FILTER_CONDITION", 24, m_szFilterCondition)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("INTEGRATED", 25, DBTYPE_BOOL, 0xFF, 0xFF, m_bIntegrated)
	END_PROVIDER_COLUMN_MAP()
};

class CSRIndexes :
	public CSchemaRowsetImpl<CSRIndexes, CIndexesRow, CCUBRIDSession>
{
public:
	~CSRIndexes();

	SR_PROPSET_MAP(CSRIndexes);

	HRESULT FillRowData(int hConn, int hReq, LONG* pcRowsAffected, const char *table_name, const char *index_name);
	HRESULT Execute(LONG* pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions);
	DBSTATUS GetDBStatus(CSimpleRow*, ATLCOLUMNINFO* pInfo);
};
