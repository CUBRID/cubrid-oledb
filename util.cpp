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

#include "stdafx.h"
#include "util.h"
#include "type.h"
#include "DataSource.h"
#include <atltime.h>

void show_error(LPSTR msg, int code, T_CCI_ERROR *error)
{
	ATLTRACE(atlTraceDBProvider, 2, "Error : %s [code : %d]", msg, code);
    if (code == CCI_ER_DBMS)
	{
		ATLTRACE(atlTraceDBProvider, 2, "DBMS Error : %s [code : %d]",
			(LPSTR)error->err_msg, error->err_code);
    }
}

namespace Util {

struct CharsetInfo
{
	int codepage;
	const char* charset;
};

struct CharsetInfo CHARSET_INFOS[] = {
	{ 037, "IBM037" },						// IBM EBCDIC US-Canada
	{ 437, "IBM437" },						// OEM United States
	{ 500, "IBM500" },						// IBM EBCDIC International
	{ 708, "ASMO-708" },					// Arabic (ASMO 708)
	{ 709, "Arabic" },						// (ASMO-449+, BCON V4)
	{ 710, "Arabic" },						// - Transparent Arabic
	{ 720, "DOS-720" },						// Arabic (Transparent ASMO); Arabic (DOS)
	{ 737, "ibm737" },						// OEM Greek (formerly 437G); Greek (DOS)
	{ 775, "ibm775" },						// OEM Baltic; Baltic (DOS)
	{ 850, "ibm850" },						// OEM Multilingual Latin 1; Western European (DOS)
	{ 852, "ibm852" },						// OEM Latin 2; Central European (DOS)
	{ 855, "IBM855" },						// OEM Cyrillic (primarily Russian)
	{ 857, "ibm857" },						// OEM Turkish; Turkish (DOS)
	{ 858, "IBM00858" },					// OEM Multilingual Latin 1 + Euro symbol
	{ 860, "IBM860" },						// OEM Portuguese; Portuguese (DOS)
	{ 861, "ibm861" },						// OEM Icelandic; Icelandic (DOS)
	{ 862, "DOS-862" },						// OEM Hebrew; Hebrew (DOS)
	{ 863, "IBM863" },						// OEM French Canadian; French Canadian (DOS)
	{ 864, "IBM864" },						// OEM Arabic; Arabic (864)
	{ 865, "IBM865" },						// OEM Nordic; Nordic (DOS)
	{ 866, "cp866" },						// OEM Russian; Cyrillic (DOS)
	{ 869, "ibm869" },						// OEM Modern Greek; Greek, Modern (DOS)
	{ 870, "IBM870" },						// IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2
	{ 874, "windows-874" },					// ANSI/OEM Thai (same as 28605, ISO 8859-15); Thai (Windows)
	{ 875, "cp875" },						// IBM EBCDIC Greek Modern
	{ 932, "shift_jis" },					// ANSI/OEM Japanese; Japanese (Shift-JIS)
	{ 936, "gb2312" },						// ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312)
	{ 949, "ks_c_5601-1987" },				// ANSI/OEM Korean (Unified Hangul Code)
	{ 950, "big5" },						// ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5)
	{ 1026, "IBM1026" },					// IBM EBCDIC Turkish (Latin 5)
	{ 1047, "IBM01047" },					// IBM EBCDIC Latin 1/Open System
	{ 1140, "IBM01140" },					// IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro)
	{ 1141, "IBM01141" },					// IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro)
	{ 1142, "IBM01142" },					// IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro)
	{ 1143, "IBM01143" },					// IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro)
	{ 1144, "IBM01144" },					// IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro)
	{ 1145, "IBM01145" },					// IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro)
	{ 1146, "IBM01146" },					// IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro)
	{ 1147, "IBM01147" },					// IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro)
	{ 1148, "IBM01148" },					// IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro)
	{ 1149, "IBM01149" },					// IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro)
	{ 1250, "windows-1250" },				// ANSI Central European; Central European (Windows)
	{ 1251, "windows-1251" },				// ANSI Cyrillic; Cyrillic (Windows)
	{ 1252, "windows-1252" },				// ANSI Latin 1; Western European (Windows)
	{ 1253, "windows-1253" },				// ANSI Greek; Greek (Windows)
	{ 1254, "windows-1254" },				// ANSI Turkish; Turkish (Windows)
	{ 1255, "windows-1255" },				// ANSI Hebrew; Hebrew (Windows)
	{ 1256, "windows-1256" },				// ANSI Arabic; Arabic (Windows)
	{ 1257, "windows-1257" },				// ANSI Baltic; Baltic (Windows)
	{ 1258, "windows-1258" },				// ANSI/OEM Vietnamese; Vietnamese (Windows)
	{ 1361, "Johab" },						// Korean (Johab)
	{ 10000, "macintosh" },					// MAC Roman; Western European (Mac)
	{ 10001, "x-mac-japanese" },			// Japanese (Mac)
	{ 10002, "x-mac-chinesetrad" },			// MAC Traditional Chinese (Big5); Chinese Traditional (Mac)
	{ 10003, "x-mac-korean" },				// Korean (Mac)
	{ 10004, "x-mac-arabic" },				// Arabic (Mac)
	{ 10005, "x-mac-hebrew" },				// Hebrew (Mac)
	{ 10006, "x-mac-greek" },				// Greek (Mac)
	{ 10007, "x-mac-cyrillic" },			// Cyrillic (Mac)
	{ 10008, "x-mac-chinesesimp" },			// MAC Simplified Chinese (GB 2312); Chinese Simplified (Mac)
	{ 10010, "x-mac-romanian" },			// Romanian (Mac)
	{ 10017, "x-mac-ukrainian" },			// Ukrainian (Mac)
	{ 10021, "x-mac-thai" },				// Thai (Mac)
	{ 10029, "x-mac-ce" },					// MAC Latin 2; Central European (Mac)
	{ 10079, "x-mac-icelandic" },			// Icelandic (Mac)
	{ 10081, "x-mac-turkish" },				// Turkish (Mac)
	{ 10082, "x-mac-croatian" },			// Croatian (Mac)
	{ 20000, "x-Chinese_CNS" },				// CNS Taiwan; Chinese Traditional (CNS)
	{ 20001, "x-cp20001" },					// TCA Taiwan
	{ 20002, "x_Chinese-Eten" },			// Eten Taiwan; Chinese Traditional (Eten)
	{ 20003, "x-cp20003" },					// IBM5550 Taiwan
	{ 20004, "x-cp20004" },					// TeleText Taiwan
	{ 20005, "x-cp20005" },					// Wang Taiwan
	{ 20105, "x-IA5" },						// IA5 (IRV International Alphabet No. 5, 7-bit); Western European (IA5)
	{ 20106, "x-IA5-German" },				// IA5 German (7-bit)
	{ 20107, "x-IA5-Swedish" },				// IA5 Swedish (7-bit)
	{ 20108, "x-IA5-Norwegian" },			// IA5 Norwegian (7-bit)
	{ 20127, "US-ASCII" },					// US-ASCII (7-bit)
	{ 20261, "x-cp20261" },					// T.61
	{ 20269, "x-cp20269" },					// ISO 6937 Non-Spacing Accent
	{ 20273, "IBM273" },					// IBM EBCDIC Germany
	{ 20277, "IBM277" },					// IBM EBCDIC Denmark-Norway
	{ 20278, "IBM278" },					// IBM EBCDIC Finland-Sweden
	{ 20280, "IBM280" },					// IBM EBCDIC Italy
	{ 20284, "IBM284" },					// IBM EBCDIC Latin America-Spain
	{ 20285, "IBM285" },					// IBM EBCDIC United Kingdom
	{ 20290, "IBM290" },					// IBM EBCDIC Japanese Katakana Extended
	{ 20297, "IBM297" },					// IBM EBCDIC France
	{ 20420, "IBM420" },					// IBM EBCDIC Arabic
	{ 20423, "IBM423" },					// IBM EBCDIC Greek
	{ 20424, "IBM424" },					// IBM EBCDIC Hebrew
	{ 20833, "x-EBCDIC-KoreanExtended," },	// IBM EBCDIC Korean Extended
	{ 20838, "IBM-Thai" },					// IBM EBCDIC Thai
	{ 20866, "koi8-r" },					// Russian (KOI8-R); Cyrillic (KOI8-R)
	{ 20871, "IBM871" },					// IBM EBCDIC Icelandic
	{ 20880, "IBM880" },					// IBM EBCDIC Cyrillic Russian
	{ 20905, "IBM905" },					// IBM EBCDIC Turkish
	{ 20924, "IBM00924" },					// IBM EBCDIC Latin 1/Open System (1047 + Euro symbol)
	{ 20932, "EUC-JP" },					// Japanese (JIS 0208-1990 and 0121-1990)
	{ 20936, "x-cp20936" },					// Simplified Chinese (GB2312); Chinese Simplified (GB2312-80)
	{ 20949, "x-cp20949" },					// Korean Wansung
	{ 21025, "cp1025" },					// IBM EBCDIC Cyrillic Serbian-Bulgarian
	{ 21866, "koi8-u" },					// Ukrainian (KOI8-U); Cyrillic (KOI8-U)
	{ 28591, "iso-8859-1" },				// ISO 8859-1 Latin 1; Western European (ISO)
	{ 28592, "iso-8859-2" },				// ISO 8859-2 Central European; Central European (ISO)
	{ 28593, "iso-8859-3" },				// ISO 8859-3 Latin 3
	{ 28594, "iso-8859-4" },				// ISO 8859-4 Baltic
	{ 28595, "iso-8859-5" },				// ISO 8859-5 Cyrillic
	{ 28596, "iso-8859-6" },				// ISO 8859-6 Arabic
	{ 28597, "iso-8859-7" },				// ISO 8859-7 Greek
	{ 28598, "iso-8859-8" },				// ISO 8859-8 Hebrew; Hebrew (ISO-Visual)
	{ 28599, "iso-8859-9" },				// ISO 8859-9 Turkish
	{ 28603, "iso-8859-13" },				// ISO 8859-13 Estonian
	{ 28605, "iso-8859-15" },				// ISO 8859-15 Latin 9
	{ 29001, "x-Europa" },					// Europa 3
	{ 38598, "iso-8859-8-i" },				// ISO 8859-8 Hebrew; Hebrew (ISO-Logical)
	{ 50220, "iso-2022-jp" },				// ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS)
	{ 50221, "csISO2022JP" },				// ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana)
	{ 50222, "iso-2022-jp" },				// ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI)
	{ 50225, "iso-2022-kr" },				// ISO 2022 Korean
	{ 50227, "x-cp50227" },					// ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)
	{ 50229, "ISO" },						// 2022 Traditional Chinese
	{ 50930, "EBCDIC" },					// Japanese (Katakana) Extended
	{ 50931, "EBCDIC" },					// US-Canada and Japanese
	{ 50933, "EBCDIC" },					// Korean Extended and Korean
	{ 50935, "EBCDIC" },					// Simplified Chinese Extended and Simplified Chinese
	{ 50936, "EBCDIC" },					// Simplified Chinese
	{ 50937, "EBCDIC" },					// US-Canada and Traditional Chinese
	{ 50939, "EBCDIC" },					// Japanese (Latin) Extended and Japanese
	{ 51932, "euc-jp" },					// EUC Japanese
	{ 51936, "EUC-CN" },					// EUC Simplified Chinese; Chinese Simplified (EUC)
	{ 51949, "euc-kr" },					// EUC Korean
	{ 51950, "EUC" },						// Traditional Chinese
	{ 52936, "hz-gb-2312" },				// HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)
	{ 54936, "GB18030" },					// Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)
	{ 57002, "x-iscii-de" },				// ISCII Devanagari
	{ 57003, "x-iscii-be" },				// ISCII Bengali
	{ 57004, "x-iscii-ta" },				// ISCII Tamil
	{ 57005, "x-iscii-te" },				// ISCII Telugu
	{ 57006, "x-iscii-as" },				// ISCII Assamese
	{ 57007, "x-iscii-or" },				// ISCII Oriya
	{ 57008, "x-iscii-ka" },				// ISCII Kannada
	{ 57009, "x-iscii-ma" },				// ISCII Malayalam
	{ 57010, "x-iscii-gu" },				// ISCII Gujarati
	{ 57011, "x-iscii-pa" },				// ISCII Punjabi
	{ 65000, "utf-7" },						// Unicode (UTF-7)
	{ 65001, "utf-8" } 						// Unicode (UTF-8)
};

int CharsetToCodepage(const char* charset)
{
	if (charset == NULL || charset[0] == '\0')
		return _AtlGetConversionACP();	// default code page for CW2A

	for (int i = 0; i < sizeof(CHARSET_INFOS) / sizeof(CHARSET_INFOS[0]); i++)
	{
		if (_stricmp(charset, CHARSET_INFOS[i].charset) == 0)
			return CHARSET_INFOS[i].codepage;
	}

	return -1;
}

HRESULT GetConnectionProperties(IDBProperties* pDBProps, CConnectionProperties& props)
{
	CDBPropIDSet set[2];
	set[0].SetGUID(DBPROPSET_UNIPROVIDER_DBINIT);
	set[0].AddPropertyID(DBPROP_UNIPROVIDER_UNICAS_PORT);
	set[0].AddPropertyID(DBPROP_UNIPROVIDER_CHARSET);

	set[1].SetGUID(DBPROPSET_DBINIT);
	set[1].AddPropertyID(DBPROP_INIT_LOCATION);
	set[1].AddPropertyID(DBPROP_INIT_DATASOURCE);
	set[1].AddPropertyID(DBPROP_AUTH_USERID);
	set[1].AddPropertyID(DBPROP_AUTH_PASSWORD);

	ULONG cPropSet = 0;
	DBPROPSET* pPropSet = NULL;
	HRESULT hr = pDBProps->GetProperties(2, set, &cPropSet, &pPropSet);
	if (FAILED(hr)) return hr;

	props.nPort = V_I4(&(pPropSet[0].rgProperties[0].vValue));
	int cp = CharsetToCodepage(CW2A(V_BSTR(&(pPropSet[0].rgProperties[1].vValue))));
	
	if (cp != -1)
	{
		props.uCodepage = cp;
	}
	else
	{
		RaiseError(DB_E_BADPROPERTYVALUE, 1, IID_NULL, L"Charset property value is invalid");
		hr = DB_E_BADPROPERTYVALUE;
		goto clear;
	}

	props.strAddr = CW2A(V_BSTR(&(pPropSet[1].rgProperties[0].vValue)));
	props.strDB = CW2A(V_BSTR(&(pPropSet[1].rgProperties[1].vValue)));
	props.strUser = CW2A(V_BSTR(&(pPropSet[1].rgProperties[2].vValue)));
	props.strPass = CW2A(V_BSTR(&(pPropSet[1].rgProperties[3].vValue)));
	hr = S_OK;

clear:
	for (ULONG i = 0; i < cPropSet; i++)
	{
		DBPROPSET& ps = pPropSet[i];
		for (ULONG j = 0; j < pPropSet[i].cProperties; j++)
			VariantClear(&(ps.rgProperties[j].vValue));
		CoTaskMemFree(ps.rgProperties);
	}
	CoTaskMemFree(pPropSet);
	return hr;
}


//DBROWCOUNT FindBookmark(const CAtlArray<DBROWCOUNT> &rgBookmarks, DBROWCOUNT iRowset)
//{
//	for(DBROWCOUNT i=0;i<(DBROWCOUNT)rgBookmarks.GetCount();i++)
//	{
//		if(rgBookmarks[i]==iRowset)
//			return i;
//	}
//	return 0;
//
//	// TODO: 북마크가 오름차순으로 정렬되어 있지 않으면 문제가 발생할 수 있음. see Rowset.h
//	//DBROWCOUNT bm = iRowset+2;
//	//DBROWCOUNT cnt = (DBROWCOUNT)rgBookmarks.GetCount();
//	//while(bm<cnt && rgBookmarks[bm]!=iRowset)
//	//{
//	//	if(rgBookmarks[bm]<iRowset)
//	//		bm++;
//	//	else
//	//		bm--;
//	//}
//	//if(bm>=cnt) return 0;
//	//return bm;
//}


HRESULT DoesTableExist(int hConn, char *szTableName)
{
	T_CCI_ERROR err_buf;
	int hReq = cci_schema_info(hConn, CCI_SCH_CLASS, szTableName, NULL,
							CCI_CLASS_NAME_PATTERN_MATCH, &err_buf);
	if(hReq<0)
		return E_FAIL;
	int rc = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
	cci_close_req_handle(hReq);
	if(rc==CCI_ER_NO_MORE_DATA)
		return S_FALSE;
	else if(rc<0)
		return E_FAIL;
	else
		return S_OK;
}

HRESULT OpenTable(int hConn, UINT uCodepage, const CComBSTR &strTableName, int *phReq, int *pcResult, char flag, bool bAsynch, int maxrows)
{
	ATLASSERT(phReq && pcResult);

	if (!strTableName || wcslen(strTableName.m_str) == 0)
		return DB_E_NOTABLE;

	CComBSTR query = "select * from ";
	int len = strTableName.Length();

	//이미 quotation이 있으면 추가하지 않고 없으면 추가한다.
	if (strTableName.m_str[0] == '\"' && strTableName.m_str[len - 1] == '\"')
	{
		query.Append(strTableName);
	} else
	{
		query.Append("[");
		query.Append(strTableName);
		query.Append("]");
	}
	
	T_CCI_ERROR err_buf;
	*phReq = cci_prepare(hConn, CW2A(query, uCodepage), flag, &err_buf);
	if(*phReq<0)
	{
		*phReq = 0;

		HRESULT hr = DoesTableExist(hConn, CW2A(strTableName, uCodepage));
		if(hr==S_FALSE)
			return DB_E_NOTABLE;
		else
			return RaiseError(E_FAIL, 1, __uuidof(IOpenRowset), CA2W(err_buf.err_msg, uCodepage));
	}

	if(maxrows!=0)
	{
//		cci_set_max_row(*phReq, maxrows);
	}

	*pcResult = cci_execute(*phReq, (bAsynch?CCI_EXEC_ASYNC:0), 0, &err_buf);
	if(*pcResult<0)
	{
		*pcResult = 0;

		// TODO: 에러를 좀 더 자세히 분류
		cci_close_req_handle(*phReq);
		return E_FAIL;
		// return RaiseError(E_FAIL, ?, ?, err_buf.err_msg);
	}

	ATLTRACE(atlTraceDBProvider, 1, "Util::OpenTable success : return hReq=%d, cResult=%d\n", *phReq, *pcResult);

	return S_OK;
}

HRESULT GetUniqueTableName(CComBSTR& strTableName)
{
	int i;

	// Table 이름이 주어지지 않았으면 임의로 생성한다.
	WCHAR rand_buf[31];

	srand( (unsigned)time( NULL ));

	/* Display 30 numbers. */
	for(i = 0; i < 15;i++ )
		rand_buf[i] = 'A' + (rand() % 26);

	srand( (unsigned)time( NULL ) + rand());
	for(; i < 30;i++ )
		rand_buf[i] = 'A' + (rand() % 26);
	rand_buf[30] = '\0';
	strTableName = rand_buf;

	return S_OK;
}

HRESULT GetTableNames(int hConn, CAtlArray<CStringA> &rgTableNames)
{
	int hReq, res;
	T_CCI_ERROR err_buf;
	HRESULT hr = S_OK;

	hReq = cci_schema_info(hConn, CCI_SCH_CLASS, NULL, NULL,
						CCI_CLASS_NAME_PATTERN_MATCH, &err_buf);
	if(hReq<0)
	{
		ATLTRACE2("GetTableNames: cci_schema_info fail\n");
		return E_FAIL;
	}

	res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
	if(res==CCI_ER_NO_MORE_DATA) goto done;
	if(res<0) { hr = E_FAIL; goto done; }

	while(1)
	{
		char *value;
		int ind;

		res = cci_fetch(hReq, &err_buf);
		if(res<0) { hr = E_FAIL; goto done; }

		res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &value, &ind);
		if(res<0) { hr = E_FAIL; goto done; }

		rgTableNames.Add(value);

		res = cci_cursor(hReq, 1, CCI_CURSOR_CURRENT, &err_buf);
		if(res==CCI_ER_NO_MORE_DATA) goto done;
		if(res<0) { hr = E_FAIL; goto done; }
	}

done:
	cci_close_req_handle(hReq);
	return hr;
}

//특정 테이블의 distinct한 인덱스 이름 리스트를 가져온다.
HRESULT GetIndexNamesInTable(int hConn, char* table_name, CAtlArray<CStringA> &rgIndexNames, CAtlArray<int> &rgIndexTypes)
{
	int hReq, res;
	T_CCI_ERROR err_buf;
	HRESULT hr = S_OK;

	hReq = cci_schema_info(hConn, CCI_SCH_CONSTRAINT, table_name, NULL,
						CCI_ATTR_NAME_PATTERN_MATCH, &err_buf);
	if(hReq<0)
	{
		ATLTRACE2("GetIndexNamesInTable: cci_schema_info fail\n");
		return E_FAIL;
	}

	res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
	if(res==CCI_ER_NO_MORE_DATA) goto done;
	if(res<0) { hr = E_FAIL; goto done; }

	while(1)
	{
		char *value;
		int ind, isUnique;

		res = cci_fetch(hReq, &err_buf);
		if(res<0) { hr = E_FAIL; goto done; }

		res = cci_get_data(hReq, 1, CCI_A_TYPE_INT, &isUnique, &ind);
		if(res<0) { hr = E_FAIL; goto done; }
		res = cci_get_data(hReq, 2, CCI_A_TYPE_STR, &value, &ind);
		if(res<0) { hr = E_FAIL; goto done; }

		bool isAlreadyExist = false;
		for (size_t i = 0; i < rgIndexNames.GetCount(); i++)
		{
			if (!strcmp(value, rgIndexNames[i].GetBuffer()))
			{
				isAlreadyExist = true;
				break;
			}
		}
		if (!isAlreadyExist)
		{
			rgIndexNames.Add(value);
			rgIndexTypes.Add(isUnique);
		}

		res = cci_cursor(hReq, 1, CCI_CURSOR_CURRENT, &err_buf);
		if(res==CCI_ER_NO_MORE_DATA) goto done;
		if(res<0) { hr = E_FAIL; goto done; }
	}

done:
	cci_close_req_handle(hReq);
	return hr;
}

void ExtractTableName(const CComBSTR &strCommandText, CComBSTR &strTableName)
{
	// Update 경우를 위해 테이블 이름을 저장해 둔다
	// 여러개의 테이블이 from 뒤에 오는 경우 updatable query는 false가 될 것이므로
	// 이 이름을 사용하지 않는다. 물론 뽑아낸 테이블 이름은 Valid하지 않다.
	CStringW str = strCommandText;
	str.MakeLower();
	strTableName = "";
	
	wchar_t *tmp = wcsstr(str.GetBuffer(), L"from ");
	if(tmp) //select, delete의 경우
	{
		wchar_t *tmp2 = wcschr(tmp, L'\"');
		if(tmp2) // double quotation이 있으면
		{
			wchar_t *tmp3 = wcschr(tmp2+1, L'\"');
			if(tmp3)
			{
				*tmp3 = 0;
				strTableName = tmp2 + 1;
			}
			// else : 닫는 quot가 없음. 에러
		}
		else
		{
			tmp = tmp+5;
			while(*tmp==' ') tmp++;
			wchar_t *tmp3 = wcschr(tmp, ' ');
			if(tmp3)
				*tmp3 = 0;
			strTableName = tmp; // 빈칸 혹은 SQL문 끝까지가 테이블 이름
		}
	} else
	{
		tmp = wcsstr(str.GetBuffer(), L"insert ");
		wchar_t* tmp1 = wcsstr(str.GetBuffer(), L"into ");

		if (tmp && tmp1) //insert의 경우
		{
			wchar_t *tmp2 = wcschr(tmp1 + 5, L'\"');
			if(tmp2) // double quotation이 있으면
			{
				wchar_t *tmp3 = wcschr(tmp2+1, L'\"');
				if(tmp3)
				{
					*tmp3 = 0;
					strTableName = tmp2 + 1;
				}
				// else : 닫는 quot가 없음. 에러
			}
			else
			{
				tmp = tmp1+5;
				while(*tmp==' ') tmp++;
				wchar_t *tmp3 = wcspbrk(tmp, L" (");
				if(tmp3)
					*tmp3 = 0;
				strTableName = tmp; // 빈칸 혹은 SQL문 끝까지가 테이블 이름
			}
		} else
		{
			tmp = wcsstr(str.GetBuffer(), L"update ");
			wchar_t* tmp1 = wcsstr(str.GetBuffer(), L"set ");

			if (tmp && tmp1) //update의 경우
			{
				wchar_t *tmp2 = wcschr(tmp + 7, L'\"');
				if(tmp2) // double quotation이 있으면
				{
					wchar_t *tmp3 = wcschr(tmp2+1, L'\"');
					if(tmp3)
					{
						*tmp3 = 0;
						strTableName = tmp2 + 1;
					}
					// else : 닫는 quot가 없음. 에러
				}
				else
				{
					tmp = tmp+7;
					while(*tmp==' ') tmp++;
					wchar_t *tmp3 = wcspbrk(tmp, L" ");
					if(tmp3)
						*tmp3 = 0;
					strTableName = tmp; // 빈칸 혹은 SQL문 끝까지가 테이블 이름
				}
			}
		}

	}
}

bool RequestedRIIDNeedsUpdatability(REFIID riid)
{
	if (riid == IID_IRowsetUpdate ||
		riid == IID_IRowsetChange ||
		riid == IID_IRowsetRefresh ||
		riid == IID_ISequentialStream ||
		riid == IID_IRow ||
		riid == IID_IGetRow ||
		riid == IID_IGetSession)
		return true;

	return false;
}

//bool RequestedRIIDNeedsOID(REFIID riid)
//{
//	if (riid == IID_IRow ||
//		riid == IID_IGetRow ||
//		riid == IID_IGetSession)
//		return true;
//
//	return false;
//}

//bool CheckOIDFromProperties(ULONG cSets, const DBPROPSET rgSets[])
//{
//	for (ULONG i = 0; i < cSets; i++)
//	{
//		if (rgSets[i].guidPropertySet == DBPROPSET_ROWSET)
//		{
//			ULONG cProp = rgSets[i].cProperties;
//			
//			for (ULONG j=0; j < cProp; j++)
//			{
//				DBPROP* rgProp = &rgSets[i].rgProperties[j];
//				if (!rgProp) return false;
//
//				if (rgProp->dwPropertyID == DBPROP_IRow ||
//					rgProp->dwPropertyID == DBPROP_IGetRow ||
//					rgProp->dwPropertyID == DBPROP_IGetSession)
//				{
//					if (V_BOOL(&(rgProp->vValue)) == ATL_VARIANT_TRUE)
//						return true;
//				}
//			}
//		}
//	}
//	return false;
//}

bool CheckUpdatabilityFromProperties(ULONG cSets, const DBPROPSET rgSets[])
{
	for (ULONG i = 0; i < cSets; i++)
	{
		if (rgSets[i].guidPropertySet == DBPROPSET_ROWSET)
		{
			ULONG cProp = rgSets[i].cProperties;
			
			for (ULONG j=0; j < cProp; j++)
			{
				DBPROP* rgProp = &rgSets[i].rgProperties[j];
				if (!rgProp) return false;

				if (rgProp->dwPropertyID == DBPROP_IRowsetChange ||
					rgProp->dwPropertyID == DBPROP_IRowsetUpdate ||
					rgProp->dwPropertyID == DBPROP_ISequentialStream ||
					rgProp->dwPropertyID == DBPROP_IRow ||
					rgProp->dwPropertyID == DBPROP_IGetRow ||
					rgProp->dwPropertyID == DBPROP_IGetSession ||
					rgProp->dwPropertyID == DBPROP_OTHERUPDATEDELETE)
				{
					if (V_BOOL(&rgProp->vValue) == ATL_VARIANT_TRUE)
						return true;
				}
			}
		}
	}
	return false;
}

//Row에서 호출되는 경우
HRESULT CColumnsInfo::GetColumnInfo(UINT uCodepage, T_CCI_COL_INFO* info, T_CCI_CUBRID_STMT cmd_type, int cCol, bool bBookmarks, ULONG ulMaxLen)
{
	m_cColumns = cCol;

	return GetColumnInfoCommon(uCodepage, info, cmd_type, bBookmarks, ulMaxLen);
}

HRESULT CColumnsInfo::GetColumnInfo(int hReq, UINT uCodepage, bool bBookmarks, ULONG ulMaxLen)
{
	T_CCI_CUBRID_STMT cmd_type;
	T_CCI_COL_INFO *info = cci_get_result_info(hReq, &cmd_type, &m_cColumns);
	if(info==NULL) return E_FAIL;

	return GetColumnInfoCommon(uCodepage, info, cmd_type, bBookmarks, ulMaxLen);
}

HRESULT CColumnsInfo::GetColumnInfoCommon(UINT uCodepage, T_CCI_COL_INFO* info, T_CCI_CUBRID_STMT cmd_type, bool bBookmarks, ULONG ulMaxLen)
{
	// 북마크 컬럼 추가
	if(bBookmarks) m_cColumns++;

	m_pInfo = new ATLCOLUMNINFO[m_cColumns];
	if(m_pInfo==NULL)
	{
		m_cColumns = 0;
		return E_OUTOFMEMORY;
	}

	//ulMaxLen값은 cci_db_paramter로 얻어온 MAX_STRING_LENGTH값에 해당
	//컬럼 사이즈가 이 값보다 큰 경우 대신 이 값을 컬럼 length로 넘겨준다
	//MAX length값이 주어지지 않았으면 1024로 세팅
	if (!ulMaxLen)
		ulMaxLen = 1024;

	// 북마크 컬럼용 Type Info
	const Type::StaticTypeInfo bmk_sta_info = { CCI_U_TYPE_UNKNOWN, DBTYPE_BYTES };
	const Type::DynamicTypeInfo bmk_dyn_info = { sizeof(ULONG), DBCOLUMNFLAGS_ISBOOKMARK|DBCOLUMNFLAGS_ISFIXEDLENGTH, 0, 0 };

	for(int i=0;i<m_cColumns;i++)
	{
		int iOrdinal = i;
		if(!bBookmarks) iOrdinal++;

		//multiset등의 info type이 -1이 나오는 경우
		//임시로 string에 해당하는 static info, dynamic info를 반환한다.
		//modified by risensh1ne
		//2003.06.17
		int type;
		if (iOrdinal > 0)
			type = CCI_GET_RESULT_INFO_TYPE(info, iOrdinal);

		const Type::StaticTypeInfo &sta_info =
			( iOrdinal==0 ? bmk_sta_info : 
			  type==-1    ? Type::GetStaticTypeInfo(CCI_U_TYPE_STRING) :
							Type::GetStaticTypeInfo(info, iOrdinal) );
		Type::DynamicTypeInfo dyn_info =
			( iOrdinal==0 ? bmk_dyn_info : 
			  type==-1    ? Type::GetDynamicTypeInfo(CCI_U_TYPE_STRING, 0, 0, true) :
							Type::GetDynamicTypeInfo(info, iOrdinal) );

		// method를 통해 생성된 result-set은 그 column-name을 "METHOD_RESULT"
		// 로 설정하고 column-type은 varchar로 지정한다.
		if (cmd_type == CUBRID_STMT_CALL || cmd_type == CUBRID_STMT_EVALUATE || cmd_type == CUBRID_STMT_GET_STATS)
		{
			m_pInfo[i].pwszName = _wcsdup(
				iOrdinal==0 ? L"Bookmark" : L"METHOD_RESULT");
		} else
		{
			m_pInfo[i].pwszName = _wcsdup(
				iOrdinal==0 ? L"Bookmark" : CA2W(CCI_GET_RESULT_INFO_NAME(info, iOrdinal), uCodepage) );
		}

		if(m_pInfo[i].pwszName==NULL)
		{
			m_cColumns = i;
			FreeColumnInfo();
			return E_OUTOFMEMORY;
		}
		m_pInfo[i].pTypeInfo = NULL;
		m_pInfo[i].iOrdinal = iOrdinal;

		// method를 통해 생성된 result-set은 그 column-name을 "METHOD_RESULT"
		// 로 설정하고 column-type은 varchar로 지정한다.
		if (cmd_type == CUBRID_STMT_CALL || cmd_type == CUBRID_STMT_EVALUATE || cmd_type == CUBRID_STMT_GET_STATS)
			m_pInfo[i].wType = DBTYPE_STR;
		else
			m_pInfo[i].wType = sta_info.nOLEDBType;

		//MAX_LENGTH 이상인 경우 ulMaxLen으로 컬럼사이즈 세팅
		//modified by risensh1ne 20030609
		if (dyn_info.ulColumnSize > ulMaxLen)
			m_pInfo[i].ulColumnSize = ulMaxLen;
		else
			m_pInfo[i].ulColumnSize = dyn_info.ulColumnSize;

		m_pInfo[i].bPrecision = dyn_info.bPrecision;
		m_pInfo[i].bScale = dyn_info.bScale;
		m_pInfo[i].dwFlags = dyn_info.ulFlags;
		m_pInfo[i].cbOffset = 0;

		if(iOrdinal==0)
		{
			m_pInfo[i].columnid.eKind = DBKIND_GUID_PROPID;
			memcpy(&m_pInfo[i].columnid.uGuid.guid, &DBCOL_SPECIALCOL, sizeof(GUID));
			m_pInfo[i].columnid.uName.ulPropid = 2;
		}
		else
		{
			m_pInfo[i].columnid.eKind = DBKIND_GUID_NAME;
			m_pInfo[i].columnid.uName.pwszName = m_pInfo[i].pwszName;
			_wcsupr(m_pInfo[i].pwszName);
			memset(&m_pInfo[i].columnid.uGuid.guid, 0, sizeof(GUID));
			m_pInfo[i].columnid.uGuid.guid.Data1 = iOrdinal; 
		}
	}
	
	return S_OK;
}

void CColumnsInfo::FreeColumnInfo()
{
	for(int i=0;i<m_cColumns;i++)
	{
		if(m_pInfo[i].pwszName)
			free(m_pInfo[i].pwszName);
	}
	delete [] m_pInfo;
			
	m_cColumns = 0;
	m_pInfo = 0;
}


}

