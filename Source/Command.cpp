////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Command.cpp : Implementation of CCUBRIDCommand

#include "stdafx.h"
#include "DataSource.h"
#include "Command.h"
#include "Rowset.h"
#include "MultipleResult.h"
#include "Session.h"
#include "util.h"
#include "type.h"
#include "row.h"
#include "ProviderInfo.h"
#include "error.h"
#include "CUBRIDStream.h"

CCUBRIDDataSource *CCUBRIDCommand::GetDataSourcePtr()
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDCommand::GetDataSourcePtr\n");

	return GetSessionPtr()->GetDataSourcePtr();
}

CCUBRIDSession *CCUBRIDCommand::GetSessionPtr()
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDCommand::GetSessionPtr\n");

	return CCUBRIDSession::GetSessionPtr(this);
}

CCUBRIDCommand *CCUBRIDCommand::GetCommandPtr(IObjectWithSite *pSite)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDCommand::GetCommandPtr\n");

	CComPtr<ICommand> spCom;

	HRESULT hr = pSite->GetSite(__uuidof(ICommand), (void **)&spCom);
	ATLASSERT(SUCCEEDED(hr));

	//return static_cast<CCUBRIDCommand *>((ICommand *)spCom);
	CCUBRIDCommand *pCom = static_cast<CCUBRIDCommand *>((ICommand *)spCom);

	return pCom;
}

CCUBRIDCommand::CCUBRIDCommand()
	: m_hReq(0), m_cExpectedRuns(0), m_prepareIndex(0), m_isPrepared(false),
	m_cParams(0), m_cParamsInQuery(0), m_pParamInfo(NULL)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDCommand::CCUBRIDCommand\n");

	m_phAccessor = NULL;
	m_pBindings = NULL;
	m_cBindings = 0;
}

CCUBRIDCommand::~CCUBRIDCommand()
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDCommand::~CCUBRIDCommand\n");

	if(m_hReq > 0)
	{
		cci_close_req_handle(m_hReq);
		m_hReq = 0;
	}

	if(m_pParamInfo)
	{
		for(DBCOUNTITEM i = 0; i < m_cParams; i++)
		{
			SysFreeString(m_pParamInfo[i].pwszName);
		}
		CoTaskMemFree(m_pParamInfo);
	}

	if(m_spUnkSite)
		GetSessionPtr()->RegisterTxnCallback(this, false);
}

STDMETHODIMP CCUBRIDCommand::SetSite(IUnknown *pUnkSite)
{
	HRESULT hr = IObjectWithSiteImpl<CCUBRIDCommand>::SetSite(pUnkSite);
	GetSessionPtr()->RegisterTxnCallback(this, true);

	return hr;
}

void CCUBRIDCommand::TxnCallback(const ITxnCallback *pOwner)
{
	if(pOwner != this)
	{
		cci_close_req_handle(m_hReq);
		m_hReq = 0;
		m_isPrepared = false;
	}
}

HRESULT CCUBRIDCommand::GetConnectionHandle(int *phConn)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDCommand::GetConnectionHandle\n");

	return GetSessionPtr()->GetConnectionHandle(phConn);
}

HRESULT CCUBRIDCommand::OnPropertyChanged(ULONG iCurSet, DBPROP* pDBProp)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDCommand::OnPropertyChanged\n");

	HRESULT hr = CUBRIDOnPropertyChanged(this, iCurSet, pDBProp);

	if(hr == S_FALSE)
		return ICommandPropertiesImpl<CCUBRIDCommand>::OnPropertyChanged(iCurSet, pDBProp);
	else
		return hr;
}

HRESULT CCUBRIDCommand::IsValidValue(ULONG iCurSet, DBPROP* pDBProp)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDCommand::IsValidValue\n");

	ATLASSERT(pDBProp);
	if(pDBProp->dwPropertyID == DBPROP_ROWSET_ASYNCH)
	{
		LONG val = V_I4(&pDBProp->vValue);
		if(val == 0 || val == DBPROPVAL_ASYNCH_PREPOPULATE // --> synchronous
			|| val == DBPROPVAL_ASYNCH_POPULATEONDEMAND) // --> asynchronous
			return S_OK;

		return S_FALSE;
	}

	if (pDBProp->dwPropertyID == DBPROP_MAXROWS)
	{
		if ( V_I4(&pDBProp->vValue) < 0 )
			return S_FALSE;
	}

	return ICommandPropertiesImpl<CCUBRIDCommand>::IsValidValue(iCurSet, pDBProp);
}

STDMETHODIMP CCUBRIDCommand::SetCommandText(REFGUID rguidDialect, LPCOLESTR pwszCommand)
{
	ATLTRACE(atlTraceDBProvider, 0, "CCUBRIDCommand::SetCommandText with SQL '%s'\n", pwszCommand ? CW2A(pwszCommand) : "");

	HRESULT hr;

	ClearError();

	hr = ICommandTextImpl<CCUBRIDCommand>::SetCommandText(rguidDialect, pwszCommand);

	Util::ExtractTableName(m_strCommandText, m_strTableName);

	char buf[256];
	WideCharToMultiByte(CP_UTF8,
		0,
		m_strTableName.m_str,
		SysStringLen(m_strTableName.m_str),
		buf,
		sizeof(buf),
		NULL,
		NULL);
	buf[SysStringLen(m_strTableName.m_str)] = '\0';
	ATLTRACE(atlTraceDBProvider, 0, "CCUBRIDCommand::SetCommandText extracted table name: %s\n", buf);

	m_isPrepared = false;

	return hr;
}

STDMETHODIMP CCUBRIDCommand::PrepareCommand(int hConn, REFIID riid)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDCommand::PrepareCommand\n");

	{
		BSTR strNewCMD = ::SysAllocStringLen(NULL, m_strCommandText.Length());
		int idxNewCMD = 0;
		int idxOID = 1;
		char in_str = 0;
		char in_comment = 0;
		char line_comment = 0;

		wchar_t *pos = m_strCommandText;
		while(*pos)
		{
			if (in_str)
			{
				if (*pos == '\'')
					in_str = 0;
			}
			else if (in_comment)
			{
				if (line_comment && *pos == '\n')
				{
					in_comment = 0;
				}
				else if (!line_comment && wcsncmp(pos, L"*/", 2) == 0)
				{
					in_comment = 0;
					strNewCMD[idxNewCMD++] = *pos++;
				}
			}
			else
			{
				if (*pos == '\'')
					in_str = 1;

				if (wcsncmp(pos, L"//", 2) == 0 || wcsncmp(pos, L"--", 2) == 0)
				{
					in_comment = 1;
					line_comment = 1;
					strNewCMD[idxNewCMD++] = *pos++;
				}
				else if (wcsncmp(pos, L"/*", 2) == 0)
				{
					in_comment = 1;
					line_comment = 0;
					strNewCMD[idxNewCMD++] = *pos++;
				}
			}

			if(!in_comment && *pos == '@')
			{
				if(*(pos - 1) == '\'')
				{
					wchar_t *end = wcschr(pos, '\'');
					wchar_t *delimiter = wcschr(pos, '|');
					if (delimiter)
					{
						delimiter = wcschr(delimiter, '|');
						if (delimiter && delimiter < end)
						{
							if(end == NULL)
								break;

							*end = 0;

							strNewCMD[idxNewCMD - 1] = '?';

							m_gOID.Add(CW2A(pos));
							m_gOIDIndex.Add(idxOID);
							idxOID++;
							pos = end + 1;
							continue;
						}
					}
				}
			}

			if(!in_str && !in_comment && *pos == '?')
				idxOID++;

			strNewCMD[idxNewCMD] = *pos;
			idxNewCMD++;
			pos++;
		}

		strNewCMD[idxNewCMD] = 0;
		m_strCommandText.Attach(strNewCMD);

		m_cParamsInQuery = idxOID - (DBCOUNTITEM)m_gOID.GetCount() - 1;
	}

	int return_code;
	T_CCI_ERROR error;
	char prepareFlag = 0;
	CComVariant propVal;

	if (riid != GUID_NULL)
	{
		if (Util::RequestedRIIDNeedsUpdatability(riid))
			prepareFlag |= (CCI_PREPARE_INCLUDE_OID | CCI_PREPARE_UPDATABLE);
	}

#ifndef FIXME
	GetPropValue(&DBPROPSET_ROWSET, DBPROP_IRowsetInfo, &propVal);
	if (propVal.boolVal == ATL_VARIANT_TRUE)
		prepareFlag = CCI_PREPARE_INCLUDE_OID | CCI_PREPARE_UPDATABLE;
#endif
	GetPropValue(&DBPROPSET_ROWSET, DBPROP_IRowsetChange, &propVal);
	if (propVal.boolVal == ATL_VARIANT_TRUE)
		prepareFlag = CCI_PREPARE_INCLUDE_OID | CCI_PREPARE_UPDATABLE;

	GetPropValue(&DBPROPSET_ROWSET, DBPROP_IRowsetUpdate, &propVal);
	if (propVal.boolVal == ATL_VARIANT_TRUE)
		prepareFlag = CCI_PREPARE_INCLUDE_OID | CCI_PREPARE_UPDATABLE;

	GetPropValue(&DBPROPSET_ROWSET, DBPROP_IRowsetRefresh, &propVal);
	if (propVal.boolVal == ATL_VARIANT_TRUE)
		prepareFlag = CCI_PREPARE_INCLUDE_OID | CCI_PREPARE_UPDATABLE;

	GetPropValue(&DBPROPSET_ROWSET, DBPROP_OTHERUPDATEDELETE, &propVal);
	if (propVal.boolVal == ATL_VARIANT_TRUE)
		prepareFlag = CCI_PREPARE_INCLUDE_OID | CCI_PREPARE_UPDATABLE;

	GetPropValue(&DBPROPSET_ROWSET, DBPROP_IRow, &propVal);
	if (propVal.boolVal == ATL_VARIANT_TRUE)
		prepareFlag = CCI_PREPARE_INCLUDE_OID | CCI_PREPARE_UPDATABLE;

	if ((return_code = cci_prepare(hConn, CW2A(m_strCommandText.m_str), prepareFlag, &error)) < 0)
	{
		if (return_code == CCI_ER_DBMS)
		{
			int error_code = error.err_code;
			return RaiseError(DB_E_ERRORSINCOMMAND, 1, __uuidof(ICommandPrepare), error.err_msg);
		}

		show_error("cci_prepare failed", return_code, &error);
		return E_FAIL;
	}

	m_hReq = return_code;
	m_prepareIndex = 0;

	return S_OK;
}

STDMETHODIMP CCUBRIDCommand::Execute(IUnknown * pUnkOuter, REFIID riid, DBPARAMS * pParams, DBROWCOUNT * pcRowsAffected, IUnknown ** ppRowset)
{
	ATLTRACE2(atlTraceDBProvider, 2, _T("CCUBRIDCommand::Execute\n"));

	HRESULT hr = S_OK;
	T_CCI_ERROR error;
	int return_code;
	int hConn;
	int numQuery;
	CCUBRIDRowset* pRowset = NULL;
	T_CCI_QUERY_RESULT	*qr = NULL;
	int result_count;
	CComVariant var;
	bool bIsAsyncExec = false;
	bool bCreateRow = false;
	bool bMultiple = false;

	ClearError();

	m_bIsExecuting = TRUE;

	if (ppRowset)
		*ppRowset = NULL;

	if (riid != IID_NULL && !ppRowset)
	{
		hr = E_INVALIDARG;
		goto ExecuteError2;
	}

	if (pUnkOuter && riid != IID_IUnknown)
		return DB_E_NOAGGREGATION;

	if (m_strCommandText.Length() == 0)
	{
		hr = DB_E_NOCOMMAND;
		goto ExecuteError2;
	}

	hr = GetConnectionHandle(&hConn);
	if (FAILED(hr))
	{
		ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDCommand::Execute - GetConnectionHandle failed\n"));
		hr = E_FAIL;
		goto ExecuteError2;
	}

	if (!m_isPrepared || m_cExpectedRuns == m_prepareIndex)
	{
		if (SUCCEEDED(hr = PrepareCommand(hConn, riid)))
		{
			m_isPrepared = true;
			m_cExpectedRuns = 1;
		}
		else
		{
			goto ExecuteError2;
		}
	}

	if (m_cParamsInQuery && pParams == NULL)
	{
		hr = DB_E_PARAMNOTOPTIONAL;
		goto ExecuteError2;
	}

	if (m_cParamsInQuery)
	{
		if (pParams->cParamSets == 0 || pParams->pData == NULL)
		{
			hr = E_INVALIDARG;
			goto ExecuteError2;
		}

		ATLBINDINGS *pBinding = 0;
		{
			if(!m_rgBindings.Lookup((ULONG)pParams->hAccessor, pBinding) || pBinding == NULL)
			{
				hr = DB_E_BADACCESSORHANDLE;
				goto ExecuteError2;
			}
			if(!(pBinding->dwAccessorFlags & DBACCESSOR_PARAMETERDATA))
			{
				hr = DB_E_BADACCESSORTYPE;
				goto ExecuteError2;
			}
		}

		if (pBinding->cBindings < m_cParamsInQuery)
		{
			hr = DB_E_PARAMNOTOPTIONAL;
			goto ExecuteError2;
		}

		for(DBCOUNTITEM i = 0; i < pBinding->cBindings; i++)
		{
			DBBINDING &rCurBind = pBinding->pBindings[i];
			DWORD dwPart = rCurBind.dwPart;
			DBTYPE wType = rCurBind.wType & ~DBTYPE_BYREF;

			T_CCI_A_TYPE aType = Type::GetCASTypeA(wType);
			T_CCI_U_TYPE uType = Type::GetCASTypeU(wType);

			DBSTATUS* dwStatus = (dwPart & DBPART_STATUS) ? (DBSTATUS *)((BYTE*)pParams->pData + rCurBind.obStatus) : NULL;

			DBORDINAL iOrdinal = rCurBind.iOrdinal;
			if (m_pParamInfo && m_pParamInfo[i].iOrdinal == iOrdinal)
			{
				HRESULT _hr = m_spConvert->CanConvert(m_pParamInfo[i].wType, wType);

				if (_hr == S_FALSE)
				{
					*dwStatus = DB_E_UNSUPPORTEDCONVERSION;
					hr = DB_E_ERRORSOCCURRED;
					goto ExecuteError2;
				}

				if (FAILED(_hr))
				{
					*dwStatus = DB_E_UNSUPPORTEDCONVERSION;
					hr = _hr;
					goto ExecuteError2;
				}
			}
			//Get Revised Ordinal
			for(size_t j = 0; j < m_gOID.GetCount(); j++)
			{
				if(m_gOIDIndex[j] <= (int)iOrdinal)
					iOrdinal++;
			}

			void *pSrc = (dwPart & DBPART_VALUE) ? ((BYTE*)pParams->pData + rCurBind.obValue) : NULL;
			if( (rCurBind.wType & DBTYPE_BYREF) && pSrc )
				pSrc = *(void **)pSrc;

			DBLENGTH ulLength = 0;
			if(dwPart & DBPART_LENGTH)
				ulLength = *(DBLENGTH *)((BYTE*)pParams->pData + rCurBind.obLength);
			else
			{
				if (pSrc)
				{
					switch(wType)
					{
					case DBTYPE_STR:
						ulLength = (DBLENGTH)strlen((const char *)pSrc);
						break;
					case DBTYPE_WSTR:
						ulLength = (DBLENGTH)wcslen((const wchar_t *)pSrc);
						break;
					case DBTYPE_BYTES:
						ulLength = rCurBind.cbMaxLen;
						break;
					case DBTYPE_VARNUMERIC:
						ulLength = rCurBind.cbMaxLen;
						break;
					case DBTYPE_VARIANT:
						VARIANT* var = (VARIANT *)pSrc;
						DBTYPE varType = var->vt;
						switch (varType)
						{
						case VT_BSTR :
							ulLength = (DBLENGTH)wcslen((const wchar_t *)var->bstrVal);
							break;
						}
					}
				} else
				{
					ulLength = 0;
				}
			}

			void *cci_value = NULL;
			if(dwStatus && *dwStatus == DBSTATUS_S_ISNULL)
			{
				aType = CCI_A_TYPE_FIRST;
				uType = CCI_U_TYPE_NULL;
			}
			else if (dwStatus && *dwStatus == DBSTATUS_S_IGNORE)
			{
				*dwStatus = DBSTATUS_E_BADSTATUS;
				hr = DB_E_ERRORSOCCURRED;
				goto ExecuteError2;
			}
			else
			{
				if (!pSrc)
				{
					hr = DB_E_ERRORSOCCURRED;
					goto ExecuteError2;
				}
				hr = Type::OLEDBValueToCCIValue(wType, &aType, &uType, pSrc, ulLength, &cci_value, dwStatus, m_spConvert);
				if (FAILED(hr))
					goto ExecuteError2;
			}

#ifndef LTM
			if (uType == CCI_U_TYPE_NCHAR)
				uType = CCI_U_TYPE_CHAR;
			else if (uType == CCI_U_TYPE_VARNCHAR)
				uType = CCI_U_TYPE_STRING;
#endif

			if (return_code = cci_bind_param(m_hReq, (int) iOrdinal, aType, cci_value, uType, 0) < 0)
			{
				show_error("cci_bind_param failed", return_code, &error);
				if (cci_value)
					CoTaskMemFree(cci_value);
				hr = DB_E_ERRORSOCCURRED;
				*dwStatus = DBSTATUS_E_BADACCESSOR;
				goto ExecuteError2;
			}

			if (cci_value)
				CoTaskMemFree(cci_value);
		}
	}

	//OID Binding
	if (m_gOID.GetCount() > 0)
	{
		for (size_t i = 0; i < m_gOID.GetCount(); i++)
		{

			if (return_code = cci_bind_param(m_hReq, m_gOIDIndex[i], CCI_A_TYPE_STR, m_gOID[i].GetBuffer(), CCI_U_TYPE_OBJECT, 0) < 0)
			{
				show_error("OID binding failed", return_code, &error);
				hr = E_FAIL;
				goto ExecuteError2;
			}
		}
	}

	if (pcRowsAffected)
		*pcRowsAffected = DB_COUNTUNAVAILABLE;

	GetPropValue(&DBPROPSET_ROWSET, DBPROP_IRow, &var);
	ATLASSERT(V_VT(&var) == VT_BOOL);
	if (V_BOOL(&var) == ATL_VARIANT_TRUE)
		bCreateRow = true;

	GetPropValue(&DBPROPSET_ROWSET, DBPROP_IMultipleResults, &var);
	ATLASSERT(V_VT(&var) == VT_BOOL);
	if (V_BOOL(&var) == ATL_VARIANT_TRUE)
		bMultiple = true;

	if (bIsAsyncExec)
	{
		if ((return_code = cci_execute(m_hReq, CCI_EXEC_QUERY_ALL | CCI_EXEC_ASYNC, 0, &error)) < 0)
		{
			show_error("cci_execute failed", return_code, &error);
			hr = E_FAIL;
			goto ExecuteError;
		}

		if ((numQuery = cci_execute_result(m_hReq, &qr, &error)) < 0)
		{
			show_error("cci_execute_result failed", return_code, &error);
			hr = E_FAIL;
			goto ExecuteError;
		}

		result_count = 0;
	}
	else
	{
		if ((return_code = cci_execute(m_hReq, CCI_EXEC_QUERY_ALL, 0, &error)) < 0)
		{
			show_error("cci_execute failed", return_code, &error);
			hr = E_FAIL;
			goto ExecuteError;
		}

		if ((numQuery = cci_execute_result(m_hReq, &qr, &error)) < 0)
		{
			show_error("cci_execute_result failed", return_code, &error);
			hr = E_FAIL;
			goto ExecuteError;
		}

		result_count = CCI_QUERY_RESULT_RESULT(qr, 1);
	}
	T_CCI_CUBRID_STMT cmd_type = (T_CCI_CUBRID_STMT)CCI_QUERY_RESULT_STMT_TYPE(qr, 1);

	if (riid == IID_IMultipleResults || bMultiple)
	{
		bool isUpdateAll = true;

		if (numQuery == 1)
		{
			if (cmd_type != CUBRID_STMT_SELECT &&
				cmd_type != CUBRID_STMT_GET_STATS &&
				cmd_type != CUBRID_STMT_CALL &&
				cmd_type != CUBRID_STMT_EVALUATE)
				GetSessionPtr()->AutoCommit(0);
		}
		else
		{
			for (int i = 1; i <= numQuery; i++)
			{
				cmd_type = (T_CCI_CUBRID_STMT)CCI_QUERY_RESULT_STMT_TYPE(qr, i);
				if (cmd_type == CUBRID_STMT_SELECT ||
					cmd_type == CUBRID_STMT_GET_STATS ||
					cmd_type == CUBRID_STMT_CALL ||
					cmd_type == CUBRID_STMT_EVALUATE)
				{
					isUpdateAll = false;
					break;
				}
			}
			if (isUpdateAll)
				GetSessionPtr()->AutoCommit(0);
		}

		{
			if (pUnkOuter != NULL && !InlineIsEqualUnknown(riid))
			{
				hr = DB_E_NOAGGREGATION;
				goto ExecuteError2;
			}

			CComPolyObject<CMultipleResult>* pPolyObj;
			hr = CComPolyObject<CMultipleResult>::CreateInstance(pUnkOuter, &pPolyObj);
			if (FAILED(hr))
				goto ExecuteError2;

			CComPtr<IUnknown> spUnk;
			hr = pPolyObj->QueryInterface(&spUnk);
			if (FAILED(hr))
			{
				delete pPolyObj;
				goto ExecuteError2;
			}

			pPolyObj->m_contained.Initialize(this, qr, pParams, numQuery, isUpdateAll);

			{
				CComPtr<IUnknown> spOuterUnk;
				QueryInterface(__uuidof(IUnknown), (void **)&spOuterUnk);
				pPolyObj->m_contained.SetSite(spOuterUnk);
			}

			hr = pPolyObj->QueryInterface(riid, (void **)ppRowset);
			if(FAILED(hr))
				goto ExecuteError2;
		}

		if (pcRowsAffected)
			*pcRowsAffected = result_count;
	}
	else if (riid == IID_IRow || bCreateRow)
	{
		if(result_count == 0)
			return DB_E_NOTFOUND;

		if(ppRowset && (
			cmd_type == CUBRID_STMT_SELECT ||
			cmd_type == CUBRID_STMT_GET_STATS ||
			cmd_type == CUBRID_STMT_CALL ||
			cmd_type == CUBRID_STMT_EVALUATE))
		{
			CComPolyObject<CCUBRIDRow> *pRow;
			hr = CComPolyObject<CCUBRIDRow>::CreateInstance(pUnkOuter, &pRow);
			if(FAILED(hr))
				goto ExecuteError2;

			CComPtr<IUnknown> spUnk;
			hr = pRow->QueryInterface(&spUnk);
			if(FAILED(hr))
			{
				delete pRow;
				goto ExecuteError2;
			}

			CComPtr<IUnknown> spOuterUnk;
			QueryInterface(__uuidof(IUnknown), (void **)&spOuterUnk);
			//pRow->m_contained.SetSite(spOuterUnk, CCUBRIDRow::Type::FromCommand);
			pRow->m_contained.SetSite(spOuterUnk, CCUBRIDRow::FromCommand);

			hr = pRow->m_contained.Initialize(m_hReq);
			if(FAILED(hr))
			{
				delete pRow;
				goto ExecuteError2;
			}

			hr = pRow->QueryInterface(riid, (void **)ppRowset);
			if(FAILED(hr))
				goto ExecuteError2;

		}
		else
		{
			GetSessionPtr()->AutoCommit(0);
			if (pcRowsAffected)
				*pcRowsAffected = result_count;
		}
	}
	else
	{
		if(ppRowset && (
			cmd_type == CUBRID_STMT_SELECT ||
			cmd_type == CUBRID_STMT_GET_STATS ||
			cmd_type == CUBRID_STMT_CALL ||
			cmd_type == CUBRID_STMT_EVALUATE))
		{
			hr = CreateRowset<CCUBRIDRowset>(pUnkOuter, riid, pParams, pcRowsAffected, ppRowset, pRowset);
			if (*ppRowset)
				pRowset->InitFromCommand(m_hReq, result_count, bIsAsyncExec);
		}
		else
		{
			GetSessionPtr()->AutoCommit(0);
			if (pcRowsAffected)
				*pcRowsAffected = result_count;
		}
	}
	if(pParams)
	{
		ATLBINDINGS *pBinding = 0;
		{
			if(!m_rgBindings.Lookup((ULONG)pParams->hAccessor, pBinding) || pBinding == NULL)
			{
				hr = DB_E_BADACCESSORHANDLE;
				goto ExecuteError2;
			}
			if(!(pBinding->dwAccessorFlags & DBACCESSOR_PARAMETERDATA))
			{
				hr = DB_E_BADACCESSORTYPE;
				goto ExecuteError2;
			}
			T_CCI_SQLX_CMD cmd_type;
			int num;
			T_CCI_COL_INFO* info =  cci_get_result_info(m_hReq, &cmd_type, &num);
			for(ULONG i = 0; i < pBinding->cBindings; i++)
			{
				if(pBinding->pBindings[i].wType == DBTYPE_IUNKNOWN)
				{
					CComPolyObject<CCUBRIDStream>* pObjStream;
					hr = CComPolyObject<CCUBRIDStream>::CreateInstance(NULL, &pObjStream);
					if (FAILED(hr))
					{
						return hr;
					}
					CComPtr<IUnknown> spUnk;
					hr = pObjStream->QueryInterface(&spUnk);
					if(FAILED(hr))
					{
						delete pObjStream;
						return hr;
					}
					pObjStream->m_contained.Initialize(hConn, m_hReq, info[1]);
					BYTE* temp = (BYTE*)pParams->pData + pBinding->pBindings[i].obValue;
					hr = pObjStream->QueryInterface(__uuidof(ISequentialStream), (void **)temp);
					if (FAILED(hr))
						return E_NOINTERFACE;
				}
			}
		}
	}

	m_prepareIndex++;
	m_bIsExecuting = FALSE;

	return hr;

ExecuteError:
	m_prepareIndex = 0;
	m_bIsExecuting = FALSE;

	return RaiseError(hr, 1, __uuidof(ICommand), error.err_msg);

ExecuteError2:
	m_prepareIndex = 0;
	m_bIsExecuting = FALSE;

	return RaiseError(hr, 0, __uuidof(ICommand));
}

STDMETHODIMP CCUBRIDCommand::Prepare(ULONG cExpectedRuns)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDCommand::Prepare\n");

	int	hConn;
	HRESULT hr = S_OK;

	ClearError();

	if (m_strCommandText.Length() == 0)
	{
		hr = DB_E_NOCOMMAND;
		goto PrepareError;
	}

	if (m_cRowsetsOpen > 0)
	{
		hr = DB_E_OBJECTOPEN;
		goto PrepareError;
	}

	bool bOptFailed = false;
	bool bReqFailed = false;
	{
		ULONG pcPropertySets;
		DBPROPSET *prgPropertySets;
		hr = ICommandPropertiesImpl<CCUBRIDCommand>::GetProperties(0, NULL,
			&pcPropertySets, &prgPropertySets);
		if(FAILED(hr))
			goto PrepareError;

		for (ULONG i = 0; i < prgPropertySets->cProperties; i++)
		{
			if (prgPropertySets->rgProperties[i].dwOptions == DBPROPOPTIONS_REQUIRED &&
				prgPropertySets->rgProperties[i].dwStatus == DBPROPSTATUS_NOTSET)
				bReqFailed = true;
			if (prgPropertySets->rgProperties[i].dwOptions == DBPROPOPTIONS_OPTIONAL &&
				prgPropertySets->rgProperties[i].dwStatus == DBPROPSTATUS_NOTSET)
				bOptFailed = true;

			VariantClear(&(prgPropertySets->rgProperties[i].vValue));
		}

		CoTaskMemFree(prgPropertySets->rgProperties);
		CoTaskMemFree(prgPropertySets);
	}

	if (bReqFailed)
	{
		hr = DB_E_ERRORSOCCURRED;
		goto PrepareError;
	}

	hr = GetConnectionHandle(&hConn);
	if (FAILED(hr))
		goto PrepareError;

	hr = PrepareCommand(hConn);
	if (FAILED(hr))
		goto PrepareError;

	m_cExpectedRuns = cExpectedRuns;
	m_cRowsetsOpen = 0;
	m_isPrepared = true;

	return bOptFailed ? DB_S_ERRORSOCCURRED : S_OK;

PrepareError:
	return RaiseError(hr, 0, __uuidof(ICommandPrepare));
}

STDMETHODIMP CCUBRIDCommand::Unprepare(void)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDCommand::Unprepare\n");

	ClearError();

	HRESULT hr = S_OK;

	if (m_cRowsetsOpen > 0)
	{
		hr = DB_E_OBJECTOPEN;
		goto UnprepareError;
	}

	if (!m_hReq)
		return S_OK;

	//if (!m_isPrepared)
	//	return DB_E_NOTPREPARED;

	m_cExpectedRuns = 0;

	if (cci_close_req_handle(m_hReq) < 0)
	{
		hr = E_FAIL;
		goto UnprepareError;
	}

	m_hReq = 0;
	m_isPrepared = false;

	return hr;

UnprepareError:
	return RaiseError(hr, 0, __uuidof(ICommandPrepare));
}

STDMETHODIMP CCUBRIDCommand::MapColumnIDs(DBORDINAL cColumnIDs, const DBID rgColumnIDs[], DBORDINAL rgColumns[])
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDCommand::MapColumnIDs\n");

	ClearError();

	if ((cColumnIDs != 0 && rgColumnIDs == NULL) || rgColumns == NULL)
		return E_INVALIDARG;

	if (m_strCommandText.Length() == 0)
		return DB_E_NOCOMMAND;

	if (!m_isPrepared)
		return DB_E_NOTPREPARED;

	return IColumnsInfoImpl<CCUBRIDCommand>::MapColumnIDs(cColumnIDs, rgColumnIDs, rgColumns);
}

STDMETHODIMP CCUBRIDCommand::GetColumnInfo(DBORDINAL *pcColumns, DBCOLUMNINFO **prgInfo,
																					 OLECHAR **ppStringsBuffer)
{
	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDCommand:GetColumnInfo\n"));

	HRESULT hr = S_OK;

	ClearError();

	if (pcColumns)
		*pcColumns = 0;
	if (prgInfo)
		*prgInfo = NULL;
	if (ppStringsBuffer)
		*ppStringsBuffer = NULL;

	if (!pcColumns || !prgInfo || !ppStringsBuffer)
	{
		return E_INVALIDARG;
	}

	if(m_strCommandText.Length() == 0)
		return DB_E_NOCOMMAND;

	if(!m_isPrepared)
		return DB_E_NOTPREPARED;

	hr = IColumnsInfoImpl<CCUBRIDCommand>::GetColumnInfo(pcColumns, prgInfo, ppStringsBuffer);

	if (*pcColumns == 0)
	{
		*prgInfo = NULL;
		*ppStringsBuffer = NULL;
	}

	return hr;
}

ATLCOLUMNINFO* CCUBRIDCommand::GetColumnInfo(CCUBRIDCommand* pv, DBORDINAL* pcInfo)
{
	if(!pv->m_Columns.m_pInfo)
	{
		CComVariant var;
		pv->GetPropValue(&DBPROPSET_ROWSET, DBPROP_BOOKMARKS, &var);

		CCUBRIDDataSource* pDS = pv->GetDataSourcePtr();

		//TODO Check error?
		pv->m_Columns.GetColumnInfo(pv->m_hReq, V_BOOL(&var) == ATL_VARIANT_TRUE, pDS->PARAM_MAX_STRING_LENGTH);
	}

	if(pcInfo)
		*pcInfo = pv->m_Columns.m_cColumns;
	return pv->m_Columns.m_pInfo;
}

STDMETHODIMP CCUBRIDCommand::GetParameterInfo(
	DB_UPARAMS *pcParams,
	DBPARAMINFO **prgParamInfo,
	OLECHAR **ppNamesBuffer)
{
	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDCommand::GetParameterInfo\n"));

	ClearError();

	if(pcParams)
		*pcParams = 0;
	if(prgParamInfo)
		*prgParamInfo = 0;
	if(ppNamesBuffer)
		*ppNamesBuffer = 0;

	if(pcParams == NULL || prgParamInfo == NULL)
		return E_INVALIDARG;

	if(m_cParams == 0)
		return DB_E_PARAMUNAVAILABLE;

	*prgParamInfo = (DBPARAMINFO *)CoTaskMemAlloc(m_cParams * sizeof(DBPARAMINFO));
	if(*prgParamInfo == NULL)
		return E_OUTOFMEMORY;

	if(ppNamesBuffer)
	{
		size_t cCount = 0;
		for(DBCOUNTITEM i = 0; i < m_cParams; i++)
		{
			if(m_pParamInfo[i].pwszName)
				cCount += wcslen(m_pParamInfo[i].pwszName) + 1;
		}

		*ppNamesBuffer = (OLECHAR *)CoTaskMemAlloc(cCount * sizeof(OLECHAR));
		if(*ppNamesBuffer == NULL)
		{
			CoTaskMemFree(*prgParamInfo);
			*prgParamInfo = 0;
			return E_OUTOFMEMORY;
		}
	}

	*pcParams = m_cParams;
	memcpy(*prgParamInfo, m_pParamInfo, m_cParams * sizeof(DBPARAMINFO));

	size_t cCount = 0;
	for(DBCOUNTITEM i = 0; i < m_cParams; i++)
	{
		if(m_pParamInfo[i].pwszName)
		{
			wcscpy((*ppNamesBuffer) + cCount, m_pParamInfo[i].pwszName);
			(*prgParamInfo)[i].pwszName = (*ppNamesBuffer) + cCount;
			cCount += wcslen(m_pParamInfo[i].pwszName) + 1;
		}
	}

	return S_OK;
}

STDMETHODIMP CCUBRIDCommand::MapParameterNames(
	DB_UPARAMS cParamNames,
	const OLECHAR *rgParamNames[],
	DB_LPARAMS rgParamOrdinals[])
{
	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDCommand::MapParameterNames\n"));

	ULONG j;

	ClearError();

	if (cParamNames == 0)
		return S_OK;

	if (rgParamNames == NULL || rgParamOrdinals == NULL)
		return E_INVALIDARG;

	if (m_cParams == 0 && (!m_strCommandText || wcslen(m_strCommandText) == 0))
	{
		return DB_E_NOCOMMAND;
	}

	if (m_cParams == 0 && !m_isPrepared)
		return DB_E_NOTPREPARED;

	bool bSucceeded = false;
	bool bFailed = false;

	for (ULONG i = 0; i < cParamNames; i++)
	{
		if (!rgParamNames[i])
		{
			bFailed = true;
			rgParamOrdinals[i] = 0;
			continue;
		}

		bool bFound = false;
		for (j = 0; j < m_cParams; j++)
		{
			if (m_pParamInfo[j].pwszName && _wcsicmp(rgParamNames[i], m_pParamInfo[j].pwszName) == 0)
			{
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			rgParamOrdinals[i] = j + 1;
			bSucceeded = true;
		}
		else
		{
			rgParamOrdinals[i] = 0;
			bFailed = true;
		}

	}

	if(!bFailed)
		return S_OK;
	else
		return bSucceeded ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED;
}

STDMETHODIMP CCUBRIDCommand::SetParameterInfo(
	DB_UPARAMS cParams,
	const DB_UPARAMS rgParamOrdinals[],
	const DBPARAMBINDINFO rgParamBindInfo[])
{
	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDCommand::SetParameterInfo\n"));

	HRESULT hr = S_OK;
	DBCOUNTITEM cNewParams = 0;
	DBPARAMINFO *pNewParamInfo = 0;

	ClearError();

	if (cParams == 0)
		goto FreeAndSet;

	if (rgParamOrdinals == NULL)
		return E_INVALIDARG;

	if(m_cRowsetsOpen > 0)
		return DB_E_OBJECTOPEN;

	if (rgParamOrdinals)
	{
		for (DBCOUNTITEM i = 0; i < cParams; i++)
			if (rgParamOrdinals[i] == 0)
				return E_INVALIDARG;
	}

	if (rgParamBindInfo)
	{
		for (DBCOUNTITEM i = 0; i < cParams; i++)
		{
			if (!rgParamBindInfo[i].pwszDataSourceType)
				return E_INVALIDARG;

			if ((rgParamBindInfo[i].dwFlags != 0) &&
				!(rgParamBindInfo[i].dwFlags & DBPARAMFLAGS_ISINPUT) &&
				!(rgParamBindInfo[i].dwFlags & DBPARAMFLAGS_ISOUTPUT) &&
				!(rgParamBindInfo[i].dwFlags & DBPARAMFLAGS_ISSIGNED) &&
				!(rgParamBindInfo[i].dwFlags & DBPARAMFLAGS_ISNULLABLE) &&
				!(rgParamBindInfo[i].dwFlags & DBPARAMFLAGS_ISLONG) &&
				!(rgParamBindInfo[i].dwFlags & DBPARAMFLAGS_SCALEISNEGATIVE))
				return E_INVALIDARG;
		}
	}

	if (rgParamBindInfo)
	{
		cNewParams = m_cParams;
		pNewParamInfo = (DBPARAMINFO *)CoTaskMemAlloc((m_cParams + cParams) * sizeof(DBPARAMINFO));

		if(m_cParams)
		{
			memcpy(pNewParamInfo, m_pParamInfo, m_cParams * sizeof(DBPARAMINFO));
			for(DBCOUNTITEM i = 0; i < m_cParams; i++)
				pNewParamInfo[i].pwszName = SysAllocString(m_pParamInfo[i].pwszName);
		}

		for(DBCOUNTITEM i = 0; i < cParams; i++)
		{
			DBCOUNTITEM j;
			for(j = 0; j < cNewParams; j++)
			{
				if(pNewParamInfo[j].iOrdinal == rgParamOrdinals[i])
					break;
			}
			if(j == cNewParams)
			{
				cNewParams++;
			}
			else
			{
				SysFreeString(pNewParamInfo[j].pwszName);
				hr = DB_S_TYPEINFOOVERRIDDEN;
			}

			pNewParamInfo[j].wType = Type::GetOledbTypeFromName(rgParamBindInfo[i].pwszDataSourceType);
			pNewParamInfo[j].pwszName = SysAllocString(rgParamBindInfo[i].pwszName);
			pNewParamInfo[j].dwFlags = rgParamBindInfo[i].dwFlags;
			pNewParamInfo[j].iOrdinal = rgParamOrdinals[i];
			pNewParamInfo[j].pTypeInfo = NULL;
			pNewParamInfo[j].ulParamSize = rgParamBindInfo[i].ulParamSize;
			pNewParamInfo[j].bPrecision = rgParamBindInfo[i].bPrecision;
			pNewParamInfo[j].bScale = rgParamBindInfo[i].bScale;
		}

		if(cNewParams == 0)
		{
			CoTaskMemFree(pNewParamInfo);
			pNewParamInfo = 0;
		}
		else
		{
			bool bIsNull = (pNewParamInfo[0].pwszName == NULL);
			for(DBCOUNTITEM i = 1; i < cNewParams; i++)
			{
				if( (pNewParamInfo[i].pwszName == NULL) != bIsNull )
				{
					for(DBCOUNTITEM i = 0; i < cNewParams; i++)
						SysFreeString(pNewParamInfo[i].pwszName);
					CoTaskMemFree(pNewParamInfo);
					return DB_E_BADPARAMETERNAME;
				}
			}
		}
	}
	else // rgParamBindInfo==NULL
	{
		if (m_cParams)
		{
			cNewParams = 0;
			pNewParamInfo = (DBPARAMINFO *)CoTaskMemAlloc(m_cParams * sizeof(DBPARAMINFO));

			for(DBCOUNTITEM i = 0; i < m_cParams; i++)
			{
				bool bFound = false;
				for(DBCOUNTITEM j = 0; j < cParams; j++)
				{
					if(m_pParamInfo[i].iOrdinal == rgParamOrdinals[j])
					{
						bFound = true;
						break;
					}
				}
				if(!bFound)
				{
					memcpy(&pNewParamInfo[cNewParams], &m_pParamInfo[i], sizeof(DBPARAMINFO));
					pNewParamInfo[cNewParams].pwszName = SysAllocString(m_pParamInfo[i].pwszName);
					cNewParams++;
				}
			}

			if(cNewParams == 0)
			{
				CoTaskMemFree(pNewParamInfo);
				pNewParamInfo = 0;
			}
		}
	}

FreeAndSet:
	if(m_pParamInfo)
	{
		for(DBCOUNTITEM i = 0; i < m_cParams; i++)
		{
			SysFreeString(m_pParamInfo[i].pwszName);
		}
		CoTaskMemFree(m_pParamInfo);
	}

	m_pParamInfo = pNewParamInfo;
	m_cParams = cNewParams;

	// sort
	if(m_cParams)
	{
		for(DBCOUNTITEM i = 0; i < m_cParams; i++)
		{
			DBORDINAL iMin = ~0;
			DBCOUNTITEM iMinIndex = i;

			for(DBCOUNTITEM j = i; j < m_cParams; j++)
			{
				if(m_pParamInfo[j].iOrdinal < iMin)
				{
					iMin = m_pParamInfo[j].iOrdinal;
					iMinIndex = j;
				}
			}

			if(i != iMinIndex)
			{
				DBPARAMINFO tmp;
				memcpy(&tmp, &m_pParamInfo[i], sizeof(DBPARAMINFO));
				memcpy(&m_pParamInfo[i], &m_pParamInfo[iMinIndex], sizeof(DBPARAMINFO));
				memcpy(&m_pParamInfo[iMinIndex], &tmp, sizeof(DBPARAMINFO));
			}

			if(m_pParamInfo[i].pwszName)
				_wcslwr(m_pParamInfo[i].pwszName);
		}
	}

	return hr;
}
