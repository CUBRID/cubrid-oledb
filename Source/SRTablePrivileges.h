////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

class CTablePrivilegesRow
{
public:
	WCHAR m_szGrantor[129];
	WCHAR m_szGrantee[129];
	WCHAR m_szTableCatalog[129];
	WCHAR m_szTableSchema[129];
	WCHAR m_szTableName[129];
	WCHAR m_szPrivilegeType[129];
	VARIANT_BOOL m_bIsGrantable;

	CTablePrivilegesRow()
	{
		memset(this, 0, sizeof(*this));	
	}

	BEGIN_PROVIDER_COLUMN_MAP(CTablePrivilegesRow)
		PROVIDER_COLUMN_ENTRY_WSTR("GRANTOR", 1, m_szGrantor)
		PROVIDER_COLUMN_ENTRY_WSTR("GRANTEE", 2, m_szGrantee)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_CATALOG", 3, m_szTableCatalog)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_SCHEMA", 4, m_szTableSchema)
		PROVIDER_COLUMN_ENTRY_WSTR("TABLE_NAME", 5, m_szTableName)
		PROVIDER_COLUMN_ENTRY_WSTR("PRIVILEGE_TYPE", 6, m_szPrivilegeType)
		PROVIDER_COLUMN_ENTRY_TYPE_PS("IS_GRANTABLE", 7, DBTYPE_BOOL, 0xFF, 0xFF, m_bIsGrantable)
	END_PROVIDER_COLUMN_MAP()
};

class CSRTablePrivileges :
	public CSchemaRowsetImpl<CSRTablePrivileges, CTablePrivilegesRow, CCUBRIDSession>
{
public:
	SR_PROPSET_MAP(CSRTablePrivileges);

	HRESULT Execute(LONG* pcRowsAffected, ULONG cRestrictions, const VARIANT *rgRestrictions);
	DBSTATUS GetDBStatus(CSimpleRow*, ATLCOLUMNINFO* pInfo);
};
