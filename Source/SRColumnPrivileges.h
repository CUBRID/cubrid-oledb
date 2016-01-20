////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

class CColumnPrivilegesRow
{
public:
	WCHAR m_szGrantor[129];
	WCHAR m_szGrantee[129];
	WCHAR m_szTableCatalog[1];
	WCHAR m_szTableSchema[1];
	WCHAR m_szTableName[256];
	WCHAR m_szColumnName[129];
	GUID m_szColumnGUID;
	ULONG m_ColumnPropID;
	WCHAR m_szPrivilegeType[11];
	VARIANT_BOOL m_bIsGrantable;

	CColumnPrivilegesRow()
	{
		memset(this, 0, sizeof(*this));	
	}

	BEGIN_PROVIDER_COLUMN_MAP(CColumnPrivilegesRow)
		PROVIDER_COLUMN_ENTRY_WSTR("GRANTOR", 1, m_szGrantor)
		PROVIDER_COLUMN_ENTRY_WSTR("GRANTEE", 2, m_szGrantee)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_CATALOG", 3, m_szTableCatalog)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_SCHEMA", 4, m_szTableSchema)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_NAME", 5, m_szTableName)
		PROVIDER_COLUMN_ENTRY_WSTR("COLUMN_NAME", 6, m_szColumnName)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("COLUMN_GUID", 7, DBTYPE_GUID, 255, 255, m_szColumnGUID)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("COLUMN_PROPID", 8, DBTYPE_UI4, 10, ~0, m_ColumnPropID)
		PROVIDER_COLUMN_ENTRY_WSTR("PRIVILEGE_TYPE", 9, m_szPrivilegeType)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("IS_GRANTABLE", 10, DBTYPE_BOOL, 0xFF, 0xFF, m_bIsGrantable)
	END_PROVIDER_COLUMN_MAP()
};

class CSRColumnPrivileges :
	public CSchemaRowsetImpl<CSRColumnPrivileges, CColumnPrivilegesRow, CCUBRIDSession>
{
public:
	~CSRColumnPrivileges();

	SR_PROPSET_MAP(CSRColumnPrivileges);

	HRESULT Execute(LONG* pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions);
	DBSTATUS GetDBStatus(CSimpleRow*, ATLCOLUMNINFO* pInfo);
	HRESULT FillRowData(int hReq, LONG *pcRowsAffected, const CComVariant &grantee,
		const char *table_name, const char *column_name);
};
