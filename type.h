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

#pragma once

namespace Type {

HRESULT GetColumnDefinitionString(DBCOLUMNDESC colDesc, CComBSTR &strType);

T_CCI_A_TYPE GetCASTypeA(DBTYPE wType);
T_CCI_U_TYPE GetCASTypeU(DBTYPE wType);
// wType�� pValue�� GetCASTypeA���� ��ȯ�� Ÿ�Կ� �´� ������ ��ȯ�Ѵ�.
// ��ȯ�� ���� free�� ���־�� �Ѵ�.
HRESULT OLEDBValueToCCIValue(DBTYPE wType, T_CCI_A_TYPE* aType, T_CCI_U_TYPE* uType, 
			void *pValue, DBLENGTH dwLength, void** dstValue, DBSTATUS* status, const CComPtr<IDataConvert> &spConvert, UINT uCodepage);

DBTYPE GetOledbTypeFromName(LPCWSTR pwszName);

// szRowValue�� pFindValue�� ���谡 CompareOp�� �����ϴ��� �˻��Ѵ�.
// szRowValue�� c string, pFindValue�� rBinding.wType�� �����ϴ� ���̴�.
HRESULT Compare(DBCOMPAREOP CompareOp, DBTYPE wType, const char *szRowValue, const char *szFindValue);

// �� Ÿ�Կ� ���� CompareOp�� �� �� �ִ� ���� ��ȯ�Ѵ�.
long GetFindCompareOps(DBTYPE wType);

/* StaticTypeInfo
 *    column ���ǿ� ������� ������ ����
 *    ProviderTypes Schema Rowset���� ���� ����
 */
struct StaticTypeInfo
{
	T_CCI_U_TYPE nCCIUType;
	USHORT nOLEDBType;
	ULONG ulSize;
	LPWSTR szPrefix;
	LPWSTR szSuffix;
	VARIANT_BOOL bCaseSensitive;
	ULONG ulSearchable;
	VARIANT_BOOL bUnsignedAttribute;
	VARIANT_BOOL bIsFixedLength;
	VARIANT_BOOL bIsLong;
	VARIANT_BOOL bFixedPrecScale;
};

// CCI CUBRID type�� �����ϴ� StaticTypeInfo�� ������ ��ȯ�Ѵ�.
const StaticTypeInfo &GetStaticTypeInfo(T_CCI_U_TYPE nCCIUType);

inline const StaticTypeInfo &GetStaticTypeInfo(int nCCIUType)
{
	return GetStaticTypeInfo((T_CCI_U_TYPE)CCI_GET_COLLECTION_DOMAIN(nCCIUType));
}

inline const StaticTypeInfo &GetStaticTypeInfo(const T_CCI_COL_INFO *info, int iOrdinal)
{
	return GetStaticTypeInfo((int)CCI_GET_RESULT_INFO_TYPE(info, iOrdinal));
}

/* DynamicTypeInfo
 *    column ���ǿ� ���� �ٲ�� ����
 *    IColumnsInfo��� ���� ����
 */
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
	//SELECT "ABC" FROM �� ���� ����� selection�ϴ� ��� precision���� -1�� ���´�.
	//�� ���� �÷� �̸��� ���̸� precision���� �Ѱ� �ش�.
	int nPrecision = CCI_GET_RESULT_INFO_PRECISION(info, iOrdinal);
	if(nPrecision==-1) nPrecision = (int)strlen(CCI_GET_RESULT_INFO_NAME(info, iOrdinal));
	return GetDynamicTypeInfo(
				(int)CCI_GET_RESULT_INFO_TYPE(info, iOrdinal),
				nPrecision,
				CCI_GET_RESULT_INFO_SCALE(info, iOrdinal),
				CCI_GET_RESULT_INFO_IS_NON_NULL(info, iOrdinal)==0);
}

struct TableInfo
{
	DBORDINAL iOrdinal;
	CComVariant varDefault;
	CComBSTR strSourceClass;
	bool bIsUnique;
};

// ���� ���� ���ڿ��̹Ƿ� ����ü ���� ����� ũ��.
// �׷��� ��ȯ�� ��� ���ڷ� �Ѱ��ֵ��� �ߴ�.
HRESULT GetTableInfo(int hConn, UINT uCodepage, PCWSTR szTableName, CAtlArray<TableInfo> &infos);

} // end of namespace Type
