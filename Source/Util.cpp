////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "util.h"
#include "type.h"
#include "DataSource.h"
#include <atltime.h>
#include "CUBRIDStream.h"

void show_error(LPSTR msg, int code, T_CCI_ERROR *error)
{
	ATLTRACE(atlTraceDBProvider, 2, "Error : %s [code : %d]", msg, code);
	if (code == CCI_ER_DBMS)
	{
		ATLTRACE(atlTraceDBProvider, 2, "DBMS Error : %s [code : %d]", (LPSTR)error->err_msg, error->err_code);
	}
}

namespace Util
{
	//DBROWCOUNT FindBookmark(const CAtlArray<DBROWCOUNT> &rgBookmarks, DBROWCOUNT iRowset)
	//{
	//	for(DBROWCOUNT i=0;i<(DBROWCOUNT)rgBookmarks.GetCount();i++)
	//	{
	//		if(rgBookmarks[i]==iRowset)
	//			return i;
	//	}
	//	return 0;
	//
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

	HRESULT Connect(IDBProperties *pDBProps, int *phConn)
	{
		ATLASSERT(phConn != NULL);
		*phConn = 0;

		int broker_port;
		{
			CDBPropIDSet set(DBPROPSET_CUBRIDPROVIDER_DBINIT);
			set.AddPropertyID(DBPROP_CUBRIDPROVIDER_BROKER_PORT);

			ULONG cPropSet = 0;
			CComHeapPtr<DBPROPSET> rgPropSet;
			HRESULT hr = pDBProps->GetProperties(1, &set, &cPropSet, &rgPropSet);
			if(FAILED(hr))
				return hr;

			ATLASSERT(cPropSet == 1);

			broker_port = V_I4(&rgPropSet->rgProperties[0].vValue);

			//Cleanup
			VariantClear(&(rgPropSet->rgProperties[0].vValue));
			CoTaskMemFree(rgPropSet->rgProperties);
		}

		{
			// Properties to read
			CDBPropIDSet set(DBPROPSET_CUBRIDPROVIDER_DBINIT);
			set.AddPropertyID(DBPROP_CUBRIDPROVIDER_AUTOCOMMIT);
			set.AddPropertyID(DBPROP_CUBRIDPROVIDER_LOGIN_TIMEOUT);
			set.AddPropertyID(DBPROP_CUBRIDPROVIDER_QUERY_TIMEOUT);
			set.AddPropertyID(DBPROP_CUBRIDPROVIDER_CHARSET);

			// Read properties
			ULONG cPropSet = 0;
			CComHeapPtr<DBPROPSET> rgPropSet;
			HRESULT hr = pDBProps->GetProperties(1, &set, &cPropSet, &rgPropSet);
			if(FAILED(hr))
				return hr;

			ATLASSERT(cPropSet == 1);

			g_autocommit    = V_I4(&rgPropSet->rgProperties[0].vValue) == 1 ? true : false;
			g_login_timeout = V_I4(&rgPropSet->rgProperties[1].vValue);
			g_query_timeout = V_I4(&rgPropSet->rgProperties[2].vValue);
			strncpy(g_charset, CW2A(V_BSTR(&rgPropSet->rgProperties[3].vValue)), 127);
			g_charset[127] = 0;

			//Cleanup
			for(int i = 0; i < 4; i++)
			{
				VariantClear(&(rgPropSet->rgProperties[i].vValue));
			}
			CoTaskMemFree(rgPropSet->rgProperties);
		}

		char lo[512], ds[512], id[512], pw[4096];
		{
			// Properties to read
			CDBPropIDSet set(DBPROPSET_DBINIT);
			set.AddPropertyID(DBPROP_INIT_LOCATION);
			set.AddPropertyID(DBPROP_INIT_DATASOURCE);
			set.AddPropertyID(DBPROP_AUTH_USERID);
			set.AddPropertyID(DBPROP_AUTH_PASSWORD);

			// Read properties
			ULONG cPropSet = 0;
			CComHeapPtr<DBPROPSET> rgPropSet;
			HRESULT hr = pDBProps->GetProperties(1, &set, &cPropSet, &rgPropSet);
			if(FAILED(hr))
				return hr;

			ATLASSERT(cPropSet == 1);

			strncpy(lo, CW2A(V_BSTR(&rgPropSet->rgProperties[0].vValue)), 511);
			lo[511] = 0;
			strncpy(ds, CW2A(V_BSTR(&rgPropSet->rgProperties[1].vValue)), 511);
			ds[511] = 0;
			strncpy(id, CW2A(V_BSTR(&rgPropSet->rgProperties[2].vValue)), 511);
			id[511] = 0;
			strncpy(pw, CW2A(V_BSTR(&rgPropSet->rgProperties[3].vValue)), 4095);
			pw[4095] = 0;

			//Cleanup
			for(int i = 0; i < 4; i++)
			{
				VariantClear(&(rgPropSet->rgProperties[i].vValue));
			}
			CoTaskMemFree(rgPropSet->rgProperties);
		}

		ATLTRACE2(atlTraceDBProvider, 2, "Location=%s;DataSource=%s;UserID=%s;Password=%s;Port=%d;\n", lo, ds, id, pw, broker_port);

		int rc = DB_SEC_E_AUTH_FAILED;
		try
		{
			//rc = cci_connect(lo, cas_port, ds, id, pw);

			//Example: URL=cci:CUBRID:192.168.0.1:33000:demodb:::?althosts=192.168.0.2:33000,192.168.0.3:33000
			char *url = (char *)malloc(512 * sizeof(char));
			_strnset(url, '\0', 512);
			strncat(url, "cci:CUBRID", strlen("cci:CUBRID"));
			strncat(url, ":", strlen(":"));
			strncat(url, lo, strlen(lo));
			strncat(url, ":", strlen(":")); //host
			char buffer[16];
			_itoa(broker_port, buffer, 10);
			strncat(url, buffer, strlen(buffer));
			strncat(url, ":", strlen(":")); //port
			strncat(url, ds, strlen(ds));
			strncat(url, ":", strlen(":")); //database
			strncat(url, "", strlen(""));
			strncat(url, ":", strlen(":")); //user forced set to empty
			strncat(url, "", strlen(""));
			strncat(url, ":", strlen(":")); //password forced set to empty
			if(g_autocommit)
			{
				CString str = "?autocommit=true";
				strncat(url, str, str.GetLength());
			}
			else
			{
				CString str = "?autocommit=false";
				strncat(url, str, str.GetLength());
			}
			if(g_login_timeout > 1000)
			{
				CString str = "";
				str.AppendFormat("&login_timeout=%d", g_login_timeout);
				strncat(url, str, str.GetLength());
			}
			else
			{
				CString str = "&login_timeout=10000";
				strncat(url, str, str.GetLength());
			}
			if(g_query_timeout > 1000)
			{
				CString str = "";
				str.AppendFormat("&query_timeout=%d", g_query_timeout);
				strncat(url, str, str.GetLength());
			}
			else
			{
				CString str = "&query_timeout=10000";
				strncat(url, str, str.GetLength());
			}

			//TODO Add charset support in the next versions of CUBRID, once it will be supported

			ATLTRACE2(atlTraceDBProvider, 2, "cci_connect string: %s\n", url);

			rc = cci_connect_with_url(url, id, pw);
		}
		catch(...)
		{
			ATLTRACE2(atlTraceDBProvider, 3, "Exception in cci_connect!\n");
		}

		ATLTRACE2(atlTraceDBProvider, 3, "cci_connect returned: %d\n", rc);

		if(rc < 0)
		{
			if(rc == CCI_ER_NO_MORE_MEMORY)
				return E_OUTOFMEMORY;

			//Wrong hostname
			return DB_SEC_E_AUTH_FAILED;
		}

		*phConn = rc;

		return S_OK;
	}

	HRESULT Disconnect(int *phConn)
	{
		ATLASSERT(phConn != NULL);

		T_CCI_ERROR err_buf;
		int rc = cci_disconnect(*phConn, &err_buf);
		//FIXME Check return value of cci_disconnect...?

		*phConn = 0;

		return S_OK;
	}

	HRESULT DoesTableExist(int hConn, char *szTableName)
	{
		T_CCI_ERROR err_buf;
		int hReq = cci_schema_info(hConn, CCI_SCH_CLASS, szTableName, NULL, CCI_CLASS_NAME_PATTERN_MATCH, &err_buf);
		if(hReq < 0)
			return E_FAIL;

		int rc = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);

		cci_close_req_handle(hReq);

		if(rc == CCI_ER_NO_MORE_DATA)
			return S_FALSE;
		else if(rc < 0)
			return E_FAIL;
		else
			return S_OK;
	}

	HRESULT OpenTable(int hConn, const CComBSTR &strTableName, int *phReq, int *pcResult, char flag, bool bAsynch, int maxrows)
	{
		ATLASSERT(phReq && pcResult);

		if (!strTableName || wcslen(strTableName.m_str) == 0)
			return DB_E_NOTABLE;

		CComBSTR query = "select * from ";
		int len = strTableName.Length();

		if (strTableName.m_str[0] == '\"' && strTableName.m_str[len - 1] == '\"')
		{
			query.Append(strTableName);
		}
		else
		{
			query.Append("[");
			query.Append(strTableName);
			query.Append("]");
		}

		T_CCI_ERROR err_buf;
		*phReq = cci_prepare(hConn, CW2A(query), flag, &err_buf);
		if(*phReq < 0)
		{
			*phReq = 0;

			HRESULT hr = DoesTableExist(hConn, CW2A(strTableName));
			if(hr == S_FALSE)
				return DB_E_NOTABLE;
			else
				return RaiseError(E_FAIL, 1, __uuidof(IOpenRowset), err_buf.err_msg);
		}

		if(maxrows != 0) //TODO Investigate this
		{
			//cci_set_max_row(*phReq, maxrows);
		}

		*pcResult = cci_execute(*phReq, (bAsynch ? CCI_EXEC_ASYNC : 0), 0, &err_buf);
		if(*pcResult < 0)
		{
			*pcResult = 0;
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
		WCHAR rand_buf[31];

		srand((unsigned)time( NULL ));
		for(i = 0; i < 15; i++ )
			rand_buf[i] = 'A' + (rand() % 26);

		srand((unsigned)time( NULL ) + rand());
		for(; i < 30; i++ )
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

		hReq = cci_schema_info(hConn, CCI_SCH_CLASS, NULL, NULL, CCI_CLASS_NAME_PATTERN_MATCH, &err_buf);
		if(hReq < 0)
		{
			ATLTRACE2("GetTableNames: cci_schema_info fail\n");
			return E_FAIL;
		}

		res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
		if(res == CCI_ER_NO_MORE_DATA)
			goto done;
		if(res < 0)
		{
			hr = E_FAIL;
			goto done;
		}

		while(true)
		{
			char *value;
			int ind;

			res = cci_fetch(hReq, &err_buf);
			if(res < 0)
			{
				hr = E_FAIL;
				goto done;
			}

			res = cci_get_data(hReq, 1, CCI_A_TYPE_STR, &value, &ind);
			if(res < 0)
			{
				hr = E_FAIL;
				goto done;
			}

			rgTableNames.Add(value);

			res = cci_cursor(hReq, 1, CCI_CURSOR_CURRENT, &err_buf);
			if(res == CCI_ER_NO_MORE_DATA)
				goto done;
			if(res < 0)
			{
				hr = E_FAIL;
				goto done;
			}
		}

done:
		cci_close_req_handle(hReq);

		return hr;
	}

	HRESULT GetIndexNamesInTable(int hConn, char* table_name, CAtlArray<CStringA> &rgIndexNames, CAtlArray<int> &rgIndexTypes)
	{
		int hReq, res;
		T_CCI_ERROR err_buf;
		HRESULT hr = S_OK;

		hReq = cci_schema_info(hConn, CCI_SCH_CONSTRAINT, table_name, NULL, CCI_ATTR_NAME_PATTERN_MATCH, &err_buf);
		if(hReq < 0)
		{
			ATLTRACE2("GetIndexNamesInTable: cci_schema_info fail\n");
			return E_FAIL;
		}

		res = cci_cursor(hReq, 1, CCI_CURSOR_FIRST, &err_buf);
		if(res == CCI_ER_NO_MORE_DATA)
			goto done;
		if(res < 0)
		{
			hr = E_FAIL;
			goto done;
		}

		while(true)
		{
			char *value;
			int ind, isUnique;

			res = cci_fetch(hReq, &err_buf);
			if(res < 0)
			{
				hr = E_FAIL;
				goto done;
			}

			res = cci_get_data(hReq, 1, CCI_A_TYPE_INT, &isUnique, &ind);
			if(res < 0)
			{
				hr = E_FAIL;
				goto done;
			}

			res = cci_get_data(hReq, 2, CCI_A_TYPE_STR, &value, &ind);
			if(res < 0)
			{
				hr = E_FAIL;
				goto done;
			}

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
			if(res == CCI_ER_NO_MORE_DATA)
				goto done;
			if(res < 0)
			{
				hr = E_FAIL;
				goto done;
			}
		}

done:
		cci_close_req_handle(hReq);

		return hr;
	}

	void ExtractTableName(const CComBSTR &strCommandText, CComBSTR &strTableName)
	{
		CW2A str(strCommandText);
		if(str == NULL)
			return;
		_strlwr(str);
		strTableName = "";

		char *tmp = strstr(str, "from ");
		if(tmp)
		{
			char *tmp2 = strchr(tmp, '\"');
			if(tmp2)
			{
				char *tmp3 = strchr(tmp2 + 1, '\"');
				if(tmp3)
				{
					*tmp3 = 0;
					strTableName = tmp2 + 1;
				}
			}
			else
			{
				tmp = tmp + 5;
				while(*tmp == ' ')
					tmp++;
				char *tmp3 = strchr(tmp, ' ');
				if(tmp3)
					*tmp3 = 0;
				strTableName = tmp;
			}
		}
		else
		{
			tmp = strstr(str, "insert ");
			char* tmp1 = strstr(str, "into ");

			if (tmp && tmp1)
			{
				char *tmp2 = strchr(tmp1 + 5, '\"');
				if(tmp2)
				{
					char *tmp3 = strchr(tmp2 + 1, '\"');
					if(tmp3)
					{
						*tmp3 = 0;
						strTableName = tmp2 + 1;
					}
				}
				else
				{
					tmp = tmp1 + 5;
					while(*tmp == ' ') tmp++;
					char *tmp3 = strpbrk(tmp, " (");
					if(tmp3)
						*tmp3 = 0;
					strTableName = tmp;
				}
			}
			else
			{
				tmp = strstr(str, "update ");
				char* tmp1 = strstr(str, "set ");

				if (tmp && tmp1)
				{
					char *tmp2 = strchr(tmp + 7, '\"');
					if(tmp2)
					{
						char *tmp3 = strchr(tmp2 + 1, '\"');
						if(tmp3)
						{
							*tmp3 = 0;
							strTableName = tmp2 + 1;
						}
					}
					else
					{
						tmp = tmp + 7;
						while(*tmp == ' ')
							tmp++;
						char *tmp3 = strpbrk(tmp, " ");
						if(tmp3)
							*tmp3 = 0;
						strTableName = tmp;
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

				for (ULONG j = 0; j < cProp; j++)
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

	HRESULT CColumnsInfo::GetColumnInfo(T_CCI_COL_INFO* info, T_CCI_CUBRID_STMT cmd_type, int cCol, bool bBookmarks, ULONG ulMaxLen)
	{
		m_cColumns = cCol;

		return GetColumnInfoCommon(info, cmd_type, bBookmarks, ulMaxLen);
	}

	HRESULT CColumnsInfo::GetColumnInfo(int hReq, bool bBookmarks, ULONG ulMaxLen)
	{
		T_CCI_CUBRID_STMT cmd_type;
		T_CCI_COL_INFO *info = cci_get_result_info(hReq, &cmd_type, &m_cColumns);
		if(info == NULL)
			return E_FAIL;
		else
		{
			//TODO Fix the scale whenever the type is CCI_U_TYPE_STRING??
			;
		}

		return GetColumnInfoCommon(info, cmd_type, bBookmarks, ulMaxLen);
	}

	HRESULT CColumnsInfo::GetColumnInfoCommon(T_CCI_COL_INFO* info, T_CCI_CUBRID_STMT cmd_type, bool bBookmarks, ULONG ulMaxLen)
	{
		if(bBookmarks) m_cColumns++;

		m_pInfo = new ATLCOLUMNINFO[m_cColumns];
		if(m_pInfo == NULL)
		{
			m_cColumns = 0;
			return E_OUTOFMEMORY;
		}

		if (!ulMaxLen)
			ulMaxLen = 1024;

		const Type::StaticTypeInfo bmk_sta_info = { CCI_U_TYPE_UNKNOWN, DBTYPE_BYTES };
		const Type::DynamicTypeInfo bmk_dyn_info = { sizeof(ULONG), DBCOLUMNFLAGS_ISBOOKMARK | DBCOLUMNFLAGS_ISFIXEDLENGTH, 0, 0 };

		for(int i = 0; i < m_cColumns; i++)
		{
			int iOrdinal = i;
			if(!bBookmarks)
				iOrdinal++;

			int type;
			if (iOrdinal > 0)
				type = CCI_GET_RESULT_INFO_TYPE(info, iOrdinal);

			const Type::StaticTypeInfo &sta_info =
				( iOrdinal == 0 ? bmk_sta_info :
				type == -1    ? Type::GetStaticTypeInfo(CCI_U_TYPE_STRING) :
				Type::GetStaticTypeInfo(info, iOrdinal) );
			Type::DynamicTypeInfo dyn_info =
				( iOrdinal == 0 ? bmk_dyn_info :
				type == -1    ? Type::GetDynamicTypeInfo(CCI_U_TYPE_STRING, 0, 0, true) :
				Type::GetDynamicTypeInfo(info, iOrdinal) );

			if (cmd_type == CUBRID_STMT_CALL || cmd_type == CUBRID_STMT_EVALUATE || cmd_type == CUBRID_STMT_GET_STATS)
			{
				m_pInfo[i].pwszName = _wcsdup(iOrdinal == 0 ? L"Bookmark" : L"METHOD_RESULT");
			}
			else
			{
				m_pInfo[i].pwszName = _wcsdup(iOrdinal == 0 ? L"Bookmark" : CA2W(CCI_GET_RESULT_INFO_NAME(info, iOrdinal)) );
			}

			if(m_pInfo[i].pwszName == NULL)
			{
				m_cColumns = i;
				FreeColumnInfo();
				return E_OUTOFMEMORY;
			}

			m_pInfo[i].pTypeInfo = NULL;
			m_pInfo[i].iOrdinal = iOrdinal;

			if (cmd_type == CUBRID_STMT_CALL || cmd_type == CUBRID_STMT_EVALUATE || cmd_type == CUBRID_STMT_GET_STATS)
				m_pInfo[i].wType = DBTYPE_STR;
			else
				m_pInfo[i].wType = sta_info.nOLEDBType;

			if (dyn_info.ulColumnSize > ulMaxLen)
				m_pInfo[i].ulColumnSize = ulMaxLen;
			else
				m_pInfo[i].ulColumnSize = dyn_info.ulColumnSize;

			m_pInfo[i].bPrecision = dyn_info.bPrecision;
			m_pInfo[i].bScale = dyn_info.bScale;
			m_pInfo[i].dwFlags = dyn_info.ulFlags;
			m_pInfo[i].cbOffset = 0;

			if(iOrdinal == 0)
			{
				m_pInfo[i].columnid.eKind = DBKIND_GUID_PROPID;
				memcpy(&m_pInfo[i].columnid.uGuid.guid, &DBCOL_SPECIALCOL, sizeof(GUID));
				m_pInfo[i].columnid.uName.ulPropid = 2;
			}
			else
			{
				m_pInfo[i].columnid.eKind = DBKIND_GUID_NAME;
				m_pInfo[i].columnid.uName.pwszName = m_pInfo[i].pwszName;
				//_wcsupr(m_pInfo[i].pwszName);
				memset(&m_pInfo[i].columnid.uGuid.guid, 0, sizeof(GUID));
				m_pInfo[i].columnid.uGuid.guid.Data1 = iOrdinal;
			}
		}

		return S_OK;
	}

	void CColumnsInfo::FreeColumnInfo()
	{
		for(int i = 0; i < m_cColumns; i++)
		{
			if(m_pInfo[i].pwszName)
				free(m_pInfo[i].pwszName);
		}
		delete [] m_pInfo;

		m_cColumns = 0;
		m_pInfo = 0;
	}

} //namespace Util

