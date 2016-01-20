////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

class CForeignKeysRow
{
private:
	WCHAR   PKTableCatalog[1];
	WCHAR   PKTableSchema[1];
	WCHAR   PKTableName[128];
	WCHAR   PKColumnName[128];
	GUID    PKColumnGUID;
	ULONG   PKColumnPropID;
	WCHAR   FKTableCatalog[1];
	WCHAR   FKTableSchema[1];
	WCHAR   FKTableName[128];
	WCHAR   FKColumnName[128];
	GUID    FKColumnGUID;
	ULONG   FKColumnPropID;
	ULONG   ColumnOrdinal;
	WCHAR   UpdateRule[128];
	WCHAR   DeleteRule[128];
	WCHAR   foreignKeyName[128];
	WCHAR   primaryKeyName[128];
	ULONG   KeySeq;

public:
	CForeignKeysRow();

	const WCHAR *GetTableNameColumn(void);
	void SetPKTableNameColumn(const WCHAR *newTableName);
	void SetFKTableNameColumn(const WCHAR *newTableName);
	void SetPKColumnNameColumn(const WCHAR *columnName);
	void SetFKColumnNameColumn(const WCHAR *columnName);
	void SetUpdateRuleColumn(const WCHAR *rule);
	void SetDeleteRuleColumn(const WCHAR *rule);
	void SetKeySeqColumn(const int &seq);
	void SetColumnOrdinalColumn(const int &ordinal);
	void SetForeignKeyNameColumn(const WCHAR *foreignKeyName);
	void SetPrimaryKeyNameColumn(const WCHAR *primaryKeyName);

	BEGIN_PROVIDER_COLUMN_MAP(CForeignKeysRow)
		PROVIDER_COLUMN_ENTRY_WSTR("PK_TABLE_CATALOG", 1, PKTableCatalog)
		PROVIDER_COLUMN_ENTRY_WSTR("PK_TABLE_SCHEMA", 2, PKTableSchema)
		PROVIDER_COLUMN_ENTRY_WSTR("PK_TABLE_NAME", 3, PKTableName)
		PROVIDER_COLUMN_ENTRY_WSTR("PK_COLUMN_NAME", 4, PKColumnName)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("PK_COLUMN_GUID", 5, DBTYPE_GUID, 0xFF, 0xFF, PKColumnGUID)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("PK_COLUMN_PROPID", 6, DBTYPE_UI4, 10, ~0, PKColumnPropID)
		PROVIDER_COLUMN_ENTRY_WSTR("FK_TABLE_CATALOG", 7, FKTableCatalog)
		PROVIDER_COLUMN_ENTRY_WSTR("FK_TABLE_SCHEMA", 8, FKTableSchema)
		PROVIDER_COLUMN_ENTRY_WSTR("FK_TABLE_NAME", 9, FKTableName)
		PROVIDER_COLUMN_ENTRY_WSTR("FK_COLUMN_NAME", 10, FKColumnName)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("FK_COLUMN_GUID", 11, DBTYPE_GUID, 0xFF, 0xFF, FKColumnGUID)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("FK_COLUMN_PROPID", 12, DBTYPE_UI4, 10, ~0, FKColumnPropID)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("ORDINAL", 13, DBTYPE_UI4, 10, ~0, ColumnOrdinal)
		PROVIDER_COLUMN_ENTRY_WSTR("UPDATE_RULE", 14, UpdateRule)
		PROVIDER_COLUMN_ENTRY_WSTR("DELETE_RULE", 15, DeleteRule)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("KEY_SEQ", 16, DBTYPE_UI4, 10, ~0, KeySeq)
		PROVIDER_COLUMN_ENTRY_WSTR("FK_NAME", 17, foreignKeyName)
		PROVIDER_COLUMN_ENTRY_WSTR("PK_NAME", 18, primaryKeyName)
	END_PROVIDER_COLUMN_MAP()
};

class CSRForeignKeys : public CSchemaRowsetImpl<CSRForeignKeys, CForeignKeysRow, CCUBRIDSession>
{
public:
	~CSRForeignKeys();

	SR_PROPSET_MAP(CSRForeignKeys);

	HRESULT Execute(LONG* pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions);
	DBSTATUS GetDBStatus(CSimpleRow*, ATLCOLUMNINFO* pInfo);

private:
	void ClearCurrentErrorObject(void);
	HRESULT GetConnectionHandleFromSessionObject(int &connectionHandle);
	HRESULT FetchData(int cci_request_handle, CForeignKeysRow &newRow);
};
