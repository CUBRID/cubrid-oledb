////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace Type
{
	HRESULT GetColumnDefinitionString(DBCOLUMNDESC colDesc, CComBSTR &strType);

	T_CCI_A_TYPE GetCASTypeA(DBTYPE wType);
	T_CCI_U_TYPE GetCASTypeU(DBTYPE wType);

	HRESULT OLEDBValueToCCIValue(DBTYPE wType, T_CCI_A_TYPE* aType, T_CCI_U_TYPE* uType,
		void *pValue, DBLENGTH dwLength, void** dstValue, DBSTATUS* status, const CComPtr<IDataConvert> &spConvert);

	DBTYPE GetOledbTypeFromName(LPCWSTR pwszName);

	HRESULT Compare(DBCOMPAREOP CompareOp, DBTYPE wType, const char *szRowValue, const char *szFindValue);

	long GetFindCompareOps(DBTYPE wType);

	struct StaticTypeInfo
	{
		T_CCI_U_TYPE nCCIUType;
		USHORT nOLEDBType;
		ULONG ulSize;
		LPWSTR szPrefix;
		LPWSTR szSuffix;
		VARIANT_BOOL	bCaseSensitive;
		ULONG					ulSearchable;
		VARIANT_BOOL	bUnsignedAttribute;
		VARIANT_BOOL	bIsFixedLength;
		VARIANT_BOOL	bIsLong;
		VARIANT_BOOL	bFixedPrecScale;
	};

	const StaticTypeInfo &GetStaticTypeInfo(T_CCI_U_TYPE nCCIUType);

	inline const StaticTypeInfo &GetStaticTypeInfo(int nCCIUType)
	{
		return GetStaticTypeInfo((T_CCI_U_TYPE)CCI_GET_COLLECTION_DOMAIN(nCCIUType));
	}

	inline const StaticTypeInfo &GetStaticTypeInfo(const T_CCI_COL_INFO *info, int iOrdinal)
	{
		return GetStaticTypeInfo((int)CCI_GET_RESULT_INFO_TYPE(info, iOrdinal));
	}

	struct DynamicTypeInfo
	{
		ULONG ulColumnSize;
		ULONG ulFlags;
		BYTE bPrecision;
		BYTE bScale;
		ULONG ulCharMaxLength;
		ULONG ulCharOctetLength;
		USHORT nNumericPrecision;
		short nNumericScale;
		ULONG ulDateTimePrecision;
	};

	DynamicTypeInfo GetDynamicTypeInfo(T_CCI_U_TYPE nCCIUType, int nPrecision, int nScale, bool bNullable);

	inline DynamicTypeInfo GetDynamicTypeInfo(int nCCIUType, int nPrecision, int nScale, bool bNullable)
	{
		return GetDynamicTypeInfo((T_CCI_U_TYPE)CCI_GET_COLLECTION_DOMAIN(nCCIUType),
			nPrecision, nScale, bNullable);
	}

	inline DynamicTypeInfo GetDynamicTypeInfo(const T_CCI_COL_INFO *info, int iOrdinal)
	{
		int nPrecision = CCI_GET_RESULT_INFO_PRECISION(info, iOrdinal);
		return GetDynamicTypeInfo(
			(int)CCI_GET_RESULT_INFO_TYPE(info, iOrdinal),
			nPrecision,
			CCI_GET_RESULT_INFO_SCALE(info, iOrdinal),
			CCI_GET_RESULT_INFO_IS_NON_NULL(info, iOrdinal) == 0);
	}

	struct TableInfo
	{
		DBORDINAL iOrdinal;
		CComVariant varDefault;
		CComBSTR strSourceClass;
		bool bIsUnique;
	};

	HRESULT GetTableInfo(int hConn, char *szTableName, CAtlArray<TableInfo> &infos);

} // end of namespace Type
