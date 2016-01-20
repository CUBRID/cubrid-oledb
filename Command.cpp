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

CCUBRIDDataSource *CCUBRIDCommand::GetDataSourcePtr()
{
	return GetSessionPtr()->GetDataSourcePtr();
}

CCUBRIDSession *CCUBRIDCommand::GetSessionPtr()
{
	return CCUBRIDSession::GetSessionPtr(this);
}

CCUBRIDCommand *CCUBRIDCommand::GetCommandPtr(IObjectWithSite *pSite)
{
	CComPtr<ICommand> spCom;
	HRESULT hr = pSite->GetSite(__uuidof(ICommand), (void **)&spCom);
	// ����� ���α׷��� ������, �����ϴ� ��찡 ������?
	ATLASSERT(SUCCEEDED(hr));
	// ���� ������带 �����ذ��� dynamic_cast�� �� �ʿ�� ���� ��
	return static_cast<CCUBRIDCommand *>((ICommand *)spCom);
}

CCUBRIDCommand::CCUBRIDCommand()
	: m_hReq(0), m_cExpectedRuns(0), m_prepareIndex(0), m_isPrepared(false),
	  m_cParams(0), m_cParamsInQuery(0), m_pParamInfo(NULL)
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDCommand::CCUBRIDCommand\n");
	m_phAccessor = NULL;
	m_pBindings = NULL;
	m_cBindings = 0;
}

CCUBRIDCommand::~CCUBRIDCommand()
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDCommand::~CCUBRIDCommand\n");

	if(m_hReq>0)
	{
		cci_close_req_handle(m_hReq);
		m_hReq = 0;
	}

	if(m_pParamInfo)
	{
		for(DBCOUNTITEM i=0;i<m_cParams;i++)
			SysFreeString(m_pParamInfo[i].pwszName);
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
	if(pOwner!=this)
	{
		cci_close_req_handle(m_hReq);
		m_hReq = 0;
		m_isPrepared = false;
	}
}

HRESULT CCUBRIDCommand::OnPropertyChanged(ULONG iCurSet, DBPROP* pDBProp)
{
	HRESULT hr = CUBRIDOnPropertyChanged(this, iCurSet, pDBProp);
	if(hr==S_FALSE)
        return ICommandPropertiesImpl<CCUBRIDCommand>::OnPropertyChanged(iCurSet, pDBProp);
	else
		return hr;
}

HRESULT CCUBRIDCommand::IsValidValue(ULONG iCurSet, DBPROP* pDBProp)
{
	ATLASSERT(pDBProp);
	if(pDBProp->dwPropertyID==DBPROP_ROWSET_ASYNCH)
	{
		// TODO: PREPOPULATE�� POPULATEONDEMAND �� �� �����ϴ� �� ��������?
		LONG val = V_I4(&pDBProp->vValue);
		if(val==0 || val==DBPROPVAL_ASYNCH_PREPOPULATE // --> synchronous
			|| val==DBPROPVAL_ASYNCH_POPULATEONDEMAND) // --> asynchronous
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

//ICommandText::SetCommandText
STDMETHODIMP CCUBRIDCommand::SetCommandText(REFGUID rguidDialect,LPCOLESTR pwszCommand)
{
	ATLTRACE(atlTraceDBProvider, 0, L"CCUBRIDCommand::SetCommandText with SQL '%s'\n",
										pwszCommand ? pwszCommand : L"");

	HRESULT hr;

	ClearError();

	hr = ICommandTextImpl<CCUBRIDCommand>::SetCommandText(rguidDialect, pwszCommand);
	Util::ExtractTableName(m_strCommandText, m_strTableName);	

	//���� Command�� ������ ��� unprepared ���·� �����.
	m_isPrepared = false;

	return hr;
}

STDMETHODIMP CCUBRIDCommand::PrepareCommand(int hConn, UINT uCodepage, REFIID riid)
{
	// OID ����
	{
		BSTR strNewCMD = ::SysAllocStringLen(NULL, m_strCommandText.Length());
		int idxNewCMD = 0;
		int idxOID = 1;
		char in_str = 0;
		char in_comment = 0;
		char line_comment = 0;
	
		//OID�� ���´� ������ ���� ������ ã�´�.
		// '���� �ѷ��׿� �ְ� '�������� @�� �´�.
		// @������ |�� �� �� �;� �ϰ� �������� �ݴ� '�� �;� �Ѵ�.
		//by risensh1ne
		wchar_t *pos = m_strCommandText;
		while(*pos)
		{
			if (in_str) {
			  if (*pos == '\'')
			    in_str = 0;
			}
			else if (in_comment) {
			  if (line_comment && *pos == '\n') {
			    in_comment = 0;
			  }
			  else if (!line_comment && wcsncmp(pos, L"*/", 2) == 0) {
			    in_comment = 0;
			    strNewCMD[idxNewCMD++] = *pos++;
			  }
			}
			else {
			  if (*pos == '\'')
			    in_str = 1;
			    
			  if (wcsncmp(pos, L"//", 2) == 0 || wcsncmp(pos, L"--", 2) == 0) {
			    in_comment = 1;
			    line_comment = 1;
			    strNewCMD[idxNewCMD++] = *pos++;
			  }
			  else if (wcsncmp(pos, L"/*", 2) == 0) {
			    in_comment = 1;
			    line_comment = 0;
			    strNewCMD[idxNewCMD++] = *pos++;
			  }
			}

			if(!in_comment && *pos=='@')
			{
				if(*(pos-1)=='\'')
				{
					wchar_t *end = wcschr(pos, '\'');
					wchar_t *delimiter = wcschr(pos, '|');
					if (delimiter)
					{
						delimiter = wcschr(delimiter, '|');
						if (delimiter && delimiter < end)
						{
							if(end==NULL) break; //error�� �� ����
							*end = 0;

							strNewCMD[idxNewCMD-1] = '?';

							m_gOID.Add(CW2A(pos, uCodepage));
							m_gOIDIndex.Add(idxOID);
							idxOID++;
							pos = end+1;
							continue;
						}
					}
				}
			}

			if(!in_str && !in_comment && *pos=='?') idxOID++;
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
		//if (Util::RequestedRIIDNeedsOID(riid))
		//	prepareFlag |= CCI_PREPARE_INCLUDE_OID;
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


    if(prepareFlag&CCI_PREPARE_UPDATABLE)
    {
    //    cci_set_holdability(hConn, 0);
    }
	if ((return_code = cci_prepare(hConn, CW2A(m_strCommandText.m_str, uCodepage), prepareFlag, &error)) < 0)
	{
		if (return_code == CCI_ER_DBMS)
		{
			int error_code = error.err_code;
			return RaiseError(DB_E_ERRORSINCOMMAND, 1, __uuidof(ICommandPrepare), CA2W(error.err_msg, uCodepage));
		}

		show_error("cci_prepare failed", return_code, &error);
		return E_FAIL;
	}
	m_hReq = return_code;

	m_prepareIndex = 0;
	
	return S_OK;
}

//ICommand::Execute
STDMETHODIMP CCUBRIDCommand::Execute(IUnknown * pUnkOuter, REFIID riid, DBPARAMS * pParams, 
						  DBROWCOUNT * pcRowsAffected, IUnknown ** ppRowset)
{
	ATLTRACE2(atlTraceDBProvider, 2, _T("CCUBRIDCommand::Execute\n"));

	HRESULT hr = S_OK;
	T_CCI_ERROR error;
	int return_code;
	int numQuery;
	CCUBRIDRowset* pRowset = NULL;
	T_CCI_QUERY_RESULT	*qr = NULL;
	int result_count;
	CComVariant var;
	bool bIsAsyncExec = false;
	bool bCreateRow = false;
	bool bMultiple = false;

	// ���� ������ü�� �����Ѵ�.
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
	
	if (m_strCommandText.Length()==0)
	{
		hr = DB_E_NOCOMMAND;
		goto ExecuteError2;
	}
	
	int hConn = GetSessionPtr()->GetConnection();
	UINT uCodepage = GetSessionPtr()->GetCodepage();
	
	//cci_prepare �ϴ� �κ�
	if (!m_isPrepared || m_cExpectedRuns==m_prepareIndex)
	{
		if (SUCCEEDED(hr = PrepareCommand(hConn, uCodepage, riid)))
		{
			m_isPrepared = true;
			m_cExpectedRuns = 1;
		} else
		{
			goto ExecuteError2;
		}
	}

	if (m_cParamsInQuery && pParams==NULL)
	{
		hr = DB_E_PARAMNOTOPTIONAL;
		goto ExecuteError2;
	}

	if (m_cParamsInQuery)
	{
		if (pParams->cParamSets==0 || pParams->pData==NULL)
		{
			hr = E_INVALIDARG;
			goto ExecuteError2;
		}

		ATLBINDINGS *pBinding = 0;
		{
			if(!m_rgBindings.Lookup((ULONG)pParams->hAccessor, pBinding) || pBinding==NULL)
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

		for(DBCOUNTITEM i=0;i<pBinding->cBindings;i++)
		{
			DBBINDING &rCurBind = pBinding->pBindings[i];
			DWORD dwPart = rCurBind.dwPart;
			DBTYPE wType = rCurBind.wType & ~DBTYPE_BYREF;
			
			T_CCI_A_TYPE aType = Type::GetCASTypeA(wType);
			T_CCI_U_TYPE uType = Type::GetCASTypeU(wType);

			DBSTATUS* dwStatus = (dwPart&DBPART_STATUS) ? (DBSTATUS *)((BYTE*)pParams->pData+rCurBind.obStatus) : NULL;

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
			for(size_t j=0;j<m_gOID.GetCount();j++)
			{
				if(m_gOIDIndex[j]<=(int)iOrdinal)
					iOrdinal++;
			}
			
			void *pSrc = (dwPart&DBPART_VALUE) ? ((BYTE*)pParams->pData+rCurBind.obValue) : NULL;
			if( (rCurBind.wType&DBTYPE_BYREF) && pSrc )
				pSrc = *(void **)pSrc;

			DBLENGTH ulLength = 0;
			if(dwPart&DBPART_LENGTH)
				ulLength = *(DBLENGTH *)((BYTE*)pParams->pData+rCurBind.obLength);
			else
			{
				if (pSrc)
				{
					switch(wType)
					{
					case DBTYPE_STR: ulLength = (DBLENGTH)strlen((const char *)pSrc); break;
					case DBTYPE_WSTR: ulLength = (DBLENGTH)wcslen((const wchar_t *)pSrc); break;
					case DBTYPE_BYTES: ulLength = rCurBind.cbMaxLen; break;
					case DBTYPE_VARNUMERIC: ulLength = rCurBind.cbMaxLen; break;
					case DBTYPE_VARIANT:
						VARIANT* var = (VARIANT *)pSrc;
						DBTYPE varType = var->vt;
						//Length�� �ʿ��� ��찡 �� ������?
						switch (varType)
						{
						case VT_BSTR :	ulLength = (DBLENGTH)wcslen((const wchar_t *)var->bstrVal);
										break;
						}
					}
				} else
				{
					ulLength = 0;
				}
			}
			
			void *cci_value = NULL;
			if(dwStatus && *dwStatus==DBSTATUS_S_ISNULL)
			{
				aType = CCI_A_TYPE_FIRST;
				uType = CCI_U_TYPE_NULL;
			} else if (dwStatus && *dwStatus==DBSTATUS_S_IGNORE)
			{
				*dwStatus=DBSTATUS_E_BADSTATUS;
				hr = DB_E_ERRORSOCCURRED;
				goto ExecuteError2;
			} else
			{
				if (!pSrc)
				{
					hr = DB_E_ERRORSOCCURRED;
					goto ExecuteError2;
				}
				hr = Type::OLEDBValueToCCIValue(wType, &aType, &uType, pSrc, ulLength, &cci_value, dwStatus, m_spConvert, uCodepage);
				if (FAILED(hr))
					goto ExecuteError2;
			}

#ifndef LTM
			//NCHAR ������ ��� CHAR �������� ��ȯ�Ͽ� ���ε��Ѵ�.
			if (uType == CCI_U_TYPE_NCHAR)
				uType = CCI_U_TYPE_CHAR;
			else if (uType == CCI_U_TYPE_VARNCHAR)
				uType = CCI_U_TYPE_STRING;
#endif

			if (return_code = cci_bind_param(m_hReq, (int) iOrdinal, aType, cci_value, uType, 0) < 0)
			{
				show_error("cci_bind_param_failed", return_code, &error);
				if (cci_value) CoTaskMemFree(cci_value);

				
				hr = DB_E_ERRORSOCCURRED;
				*dwStatus = DBSTATUS_E_BADACCESSOR;
				goto ExecuteError2;
			}
			if (cci_value) CoTaskMemFree(cci_value);
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
	
	//Asynch option Ȯ��
	/*
	GetPropValue(&DBPROPSET_ROWSET, DBPROP_ROWSET_ASYNCH, &var);
	ATLASSERT(V_VT(&var)==VT_I4);
	if (V_I4(&var)&DBPROPVAL_ASYNCH_POPULATEONDEMAND)
		bIsAsyncExec = true;
	*/

	//DBPROP_IRow ó��
	GetPropValue(&DBPROPSET_ROWSET, DBPROP_IRow, &var);
	ATLASSERT(V_VT(&var)==VT_BOOL);
	if (V_BOOL(&var) == ATL_VARIANT_TRUE)
		bCreateRow = true;
	//DBPROP_IMultipleResults
	GetPropValue(&DBPROPSET_ROWSET, DBPROP_IMultipleResults, &var);
	ATLASSERT(V_VT(&var)==VT_BOOL);
	if (V_BOOL(&var) == ATL_VARIANT_TRUE)
		bMultiple = true;

	if (bIsAsyncExec)
	{
		if ((return_code = cci_execute(m_hReq, CCI_EXEC_QUERY_ALL|CCI_EXEC_ASYNC, 0, &error)) < 0)
		{
			show_error("cci_execute_failed", return_code, &error);
			hr = E_FAIL;
			goto ExecuteError;
		}

		if ((numQuery = cci_execute_result(m_hReq, &qr, &error)) < 0)
		{
			show_error("cci_execute_result_failed", return_code, &error);
			hr = E_FAIL;
			goto ExecuteError;
		}
		result_count = 0;
	} else
	{
		if ((return_code = cci_execute(m_hReq, CCI_EXEC_QUERY_ALL, 0, &error)) < 0)
		{
			/*
			if (error.err_code == -495)
			{
				hr = DB_E_CANTCONVERTVALUE;
				goto ExecuteError;
			}
			*/

			show_error("cci_execute_failed", return_code, &error);
			hr = E_FAIL;
			goto ExecuteError;
		}

		//���� ���� ����� �����´�
		if ((numQuery = cci_execute_result(m_hReq, &qr, &error)) < 0)
		{
			show_error("cci_execute_result_failed", return_code, &error);
			hr = E_FAIL;
			goto ExecuteError;
		}
		result_count = CCI_QUERY_RESULT_RESULT(qr, 1);
	}
	T_CCI_CUBRID_STMT cmd_type = (T_CCI_CUBRID_STMT)CCI_QUERY_RESULT_STMT_TYPE(qr, 1);


	if (riid == IID_IMultipleResults || bMultiple)
	{
		//ó���� ���Ǽ��� �ϳ��̰� ������Ʈ ������ ���,
		//��� ���ǰ� ������Ʈ ������ ���
		//Execute Ÿ�ӿ� Ŀ���Ѵ�.
		bool isUpdateAll = true;
		if (numQuery == 1)
		{
			if (cmd_type!=CUBRID_STMT_SELECT &&
				cmd_type!=CUBRID_STMT_GET_STATS &&
				cmd_type!=CUBRID_STMT_CALL &&
				cmd_type!=CUBRID_STMT_EVALUATE)
				GetSessionPtr()->AutoCommit(0);
			
		} else {

			for (int i = 1; i <= numQuery; i++)
			{
				cmd_type = (T_CCI_CUBRID_STMT)CCI_QUERY_RESULT_STMT_TYPE(qr, i);
				if (cmd_type==CUBRID_STMT_SELECT ||
					cmd_type==CUBRID_STMT_GET_STATS ||
					cmd_type==CUBRID_STMT_CALL ||
					cmd_type==CUBRID_STMT_EVALUATE)
				{
					isUpdateAll = false;
					break;
				}
			}
			if (isUpdateAll)
				GetSessionPtr()->AutoCommit(0);
		}

		// MultipleResult ��ü ����
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

			// Ref the created COM object and Auto release it on failure
			CComPtr<IUnknown> spUnk;
			hr = pPolyObj->QueryInterface(&spUnk);
			if (FAILED(hr))
			{
				delete pPolyObj; // must hand delete as it is not ref'd
				goto ExecuteError2;
			}

			pPolyObj->m_contained.Initialize(this, uCodepage, qr, pParams, numQuery, isUpdateAll);

			// Command object�� IUnknown�� MultipleResult�� Site�� �����Ѵ�.
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
		// TODO: return ��� goto ExecuteError?
		// ���ǿ� �´� Row�� �ϳ��� ���� ��
		if(result_count==0)
			return DB_E_NOTFOUND;

		if(ppRowset && (
			cmd_type==CUBRID_STMT_SELECT ||
			cmd_type==CUBRID_STMT_GET_STATS ||
			cmd_type==CUBRID_STMT_CALL ||
			cmd_type==CUBRID_STMT_EVALUATE))
		{
			//Row object�� �����Ѵ�.
			CComPolyObject<CCUBRIDRow> *pRow;
			hr = CComPolyObject<CCUBRIDRow>::CreateInstance(pUnkOuter, &pRow);
			if(FAILED(hr))
				goto ExecuteError2;

			// ������ COM ��ü�� �����ؼ�, ���н� �ڵ� �����ϵ��� �Ѵ�.
			CComPtr<IUnknown> spUnk;
			hr = pRow->QueryInterface(&spUnk);
			if(FAILED(hr))
			{
				delete pRow; // �������� �ʾұ� ������ �������� �����.
				goto ExecuteError2;
			}

			// Command object�� IUnknown�� Row�� Site�� �����Ѵ�.
			CComPtr<IUnknown> spOuterUnk;
			QueryInterface(__uuidof(IUnknown), (void **)&spOuterUnk);
			pRow->m_contained.SetSite(spOuterUnk, CCUBRIDRow::FromCommand);

			hr = pRow->m_contained.Initialize(m_hReq);
			if(FAILED(hr))
			{
				delete pRow;
				goto ExecuteError2;
			}
			
			//������ Row ��ü�� IRow �������̽� ��ȯ
			hr = pRow->QueryInterface(riid, (void **)ppRowset);
			if(FAILED(hr))
				goto ExecuteError2;

			// LTM���� �̿� ���� ����� ���� �ʰ� �ִ�.
			// �ϴ� �ּ� ó���Ѵ�.
			//if (result_count > 1)
			//	hr = DB_S_NOTSINGLETON;
		} else
		{
			GetSessionPtr()->AutoCommit(0);
			if (pcRowsAffected)
				*pcRowsAffected = result_count;
		}
	} else
	{	
		if(ppRowset && (
			cmd_type==CUBRID_STMT_SELECT ||
			cmd_type==CUBRID_STMT_GET_STATS ||
			cmd_type==CUBRID_STMT_CALL ||
			cmd_type==CUBRID_STMT_EVALUATE))
		{
			hr = CreateRowset<CCUBRIDRowset>(pUnkOuter, riid, pParams, pcRowsAffected, ppRowset, pRowset);	
			
			if (*ppRowset)
				pRowset->InitFromCommand(m_hReq, uCodepage,result_count, bIsAsyncExec);
		} else
		{
			GetSessionPtr()->AutoCommit(0);
			if (pcRowsAffected)
				*pcRowsAffected = result_count;
		}
	}

	m_prepareIndex++;
	m_bIsExecuting = FALSE;

	return hr;

ExecuteError:
	m_prepareIndex = 0;
	m_bIsExecuting = FALSE;
	return RaiseError(hr, 1, __uuidof(ICommand), CA2W(error.err_msg, uCodepage));

//���� ��Ʈ���� ���� ���� ���
ExecuteError2:
	m_prepareIndex = 0;
	m_bIsExecuting = FALSE;
	return RaiseError(hr, 0, __uuidof(ICommand));
}

STDMETHODIMP CCUBRIDCommand::Prepare(ULONG cExpectedRuns)
{
	HRESULT hr = S_OK;

	// ���� ������ü�� �����Ѵ�.
	ClearError();

	//Command�� ���õǾ� ���� ���� ��
	if (m_strCommandText.Length()==0)
	{
			hr = DB_E_NOCOMMAND;
			goto PrepareError;
	}

	//Rowset�� �ϳ��� �����ִ� ���¸� DB_E_OBJECTOPEN�� ����
	if (m_cRowsetsOpen > 0)
	{
			hr = DB_E_OBJECTOPEN;
			goto PrepareError;
	}
		
	// ���� Command�� ���� ���õǾ� �ִ� ������Ƽ���� ���¸� �˻��Ѵ�
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

	//Connection handle�� �����´�
	int hConn = GetSessionPtr()->GetConnection();
	UINT uCodepage = GetSessionPtr()->GetCodepage();

	hr = PrepareCommand(hConn, uCodepage);
	if (FAILED(hr))
		goto PrepareError;

	//�� �� �ֱ�� prepare�� �ٽ� �� �������� ����
	m_cExpectedRuns = cExpectedRuns;

	//���� Command�� ���� �����ִ� Rowset ���� 0����
	m_cRowsetsOpen = 0;
	m_isPrepared = true;

	return bOptFailed ? DB_S_ERRORSOCCURRED : S_OK;

PrepareError:
	return RaiseError(hr, 0, __uuidof(ICommandPrepare));
}

STDMETHODIMP CCUBRIDCommand::Unprepare(void)
{
	// ���� ������ü�� �����Ѵ�.
	ClearError();

	HRESULT hr = S_OK;

	if (m_cRowsetsOpen > 0)
	{
		hr = DB_E_OBJECTOPEN;
		goto UnprepareError;
	}
		
	//Prepare�� ���°� �ƴҶ�
	if (!m_hReq)
		return S_OK;

	//if (!m_isPrepared)
	//	return DB_E_NOTPREPARED;

	//prepare �ֱ⸦ 0����, request handle�� ����
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

STDMETHODIMP CCUBRIDCommand::MapColumnIDs(DBORDINAL cColumnIDs, const DBID rgColumnIDs[],
							DBORDINAL rgColumns[])
{
	ClearError();

	if ((cColumnIDs != 0 && rgColumnIDs == NULL) || rgColumns == NULL)
			return E_INVALIDARG;

	if (m_strCommandText.Length()==0)
		return DB_E_NOCOMMAND;

	if (!m_isPrepared)
		return DB_E_NOTPREPARED;

	return IColumnsInfoImpl<CCUBRIDCommand>::MapColumnIDs(cColumnIDs, rgColumnIDs, rgColumns);
}

STDMETHODIMP CCUBRIDCommand::GetColumnInfo(DBORDINAL *pcColumns, DBCOLUMNINFO **prgInfo,
							 OLECHAR **ppStringsBuffer)
{
	ATLTRACE(atlTraceDBProvider, 2, _T("UniCommand:GetColumnInfo\n"));

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

	if(m_strCommandText.Length()==0)
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

		//MAX_STRING_LENGTH�� ��� ����
		CCUBRIDDataSource* pDS = pv->GetDataSourcePtr();

		// TODO : check error?
		pv->m_Columns.GetColumnInfo(pv->m_hReq, pv->m_uCodepage, V_BOOL(&var)==ATL_VARIANT_TRUE, pDS->PARAM_MAX_STRING_LENGTH);
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
		
	if(pcParams==NULL || prgParamInfo==NULL)
		return E_INVALIDARG;

	if(m_cParams==0)
		return DB_E_PARAMUNAVAILABLE;

	// SQL���� �Ķ���� ������ ���� �� �����Ƿ�
	// DB_E_NOCOMMAND, DB_E_NOTPREPARED�� ��ȯ�ϴ� ���� ����.

	*prgParamInfo = (DBPARAMINFO *)CoTaskMemAlloc(m_cParams*sizeof(DBPARAMINFO));
	if(*prgParamInfo==NULL)
		return E_OUTOFMEMORY;

	if(ppNamesBuffer)
	{
		size_t cCount = 0;
		for(DBCOUNTITEM i=0;i<m_cParams;i++)
		{
			//���� NULL ���ڱ��� ����Ͽ� cCount�� ����Ѵ�
			if(m_pParamInfo[i].pwszName)
				cCount += wcslen(m_pParamInfo[i].pwszName) + 1;
		}

		*ppNamesBuffer = (OLECHAR *)CoTaskMemAlloc(cCount*sizeof(OLECHAR));
		if(*ppNamesBuffer==NULL)
		{
			CoTaskMemFree(*prgParamInfo);
			*prgParamInfo = 0;
			return E_OUTOFMEMORY;
		}
	}

	//�Ķ���� ���� ī��
	*pcParams = m_cParams;
	//�Ķ���� ���� ����ü �迭 ī��
	memcpy(*prgParamInfo, m_pParamInfo, m_cParams*sizeof(DBPARAMINFO));

	//�Ķ���� �̸��� �̾� ���δ�.
	size_t cCount = 0;
	for(DBCOUNTITEM i=0;i<m_cParams;i++)
	{
		if(m_pParamInfo[i].pwszName)
		{
			wcscpy((*ppNamesBuffer)+cCount, m_pParamInfo[i].pwszName);
			(*prgParamInfo)[i].pwszName = (*ppNamesBuffer)+cCount;
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
	ULONG j;

	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDCommand::MapParameterNames\n"));

	ClearError();

	//cParamNames�� 0�̸� �׳� S_OK�� ����
	if (cParamNames == 0)
		return S_OK;

	//cParamNames�� 0�� �ƴϰ� rgParamNames Ȥ�� rgParamOrdinals�� NULL�̸�
	//E_INVALIDARG�� ����
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
			if (m_pParamInfo[j].pwszName &&
				_wcsicmp(rgParamNames[i], m_pParamInfo[j].pwszName) == 0)
			{
				bFound = true;
				break;
			}
		}

		//�Ķ���� �̸��� ���� ���� �߰ߵǸ� bSucceeded�� true�� �����ϰ�
		//�ϳ��� �߰ߵ��� ���ϸ� bFailed�� true�� ����
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

	//cParam�� 0�̸� ������ ���õ� �Ķ���� ������ ��� �����Ѵ�
	if (cParams == 0)
		goto FreeAndSet;
	
	//�Լ� argument üũ
	if (rgParamOrdinals == NULL)
		return E_INVALIDARG;

	//���� Command�� ���� Rowset�� ���� ������ DB_E_OBJECTOPEN ����
	if(m_cRowsetsOpen>0)
		return DB_E_OBJECTOPEN;

	//Ordinal�� �ϳ��� 0�̸� E_INVALIDARG�� ����
	if (rgParamOrdinals)
	{
		for (DBCOUNTITEM i = 0; i < cParams; i++)
			if (rgParamOrdinals[i] == 0)
				return E_INVALIDARG;
	}

	//pwszDataSourceType �׸��� NULL�̸� E_INVALIDARG�� ����
	//���� DefaultTypeConversion�� �������� �ʴ� ������ ��
	//dwFlags �׸��� valid���� ���� ��� E_INVALIDARG�� ����
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
		pNewParamInfo = (DBPARAMINFO *)CoTaskMemAlloc((m_cParams+cParams)*sizeof(DBPARAMINFO));

		// ���� �Ķ���� ������ �����Ѵ�.
		if(m_cParams)
		{
			memcpy(pNewParamInfo, m_pParamInfo, m_cParams*sizeof(DBPARAMINFO));
			for(DBCOUNTITEM i=0;i<m_cParams;i++)
				pNewParamInfo[i].pwszName = SysAllocString(m_pParamInfo[i].pwszName);
		}

		for(DBCOUNTITEM i=0;i<cParams;i++)
		{
			// ���ο� �Ķ������ ordinal�� ���� ������ ordinal�� ��ġ������ ����
			DBCOUNTITEM j;
			for(j=0;j<cNewParams;j++)
			{
				if(pNewParamInfo[j].iOrdinal==rgParamOrdinals[i])
					break;
			}
			if(j==cNewParams)
			{	// �� ã����
				cNewParams++;
			}
			else
			{	// ã����. ���� ������ override
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

		if(cNewParams==0)
		{
			CoTaskMemFree(pNewParamInfo);
			pNewParamInfo = 0;
		}
		else
		{
			//rgParamBindInfo����ü �迭�� pwszName �ʵ�� ��� NULL�̴��� ��� ���� �������� �ؾ� �Ѵ�
			//�׷��� ���ϸ� DB_E_BADPARAMETERNAME�� ����
			bool bIsNull = (pNewParamInfo[0].pwszName==NULL);
			for(DBCOUNTITEM i=1;i<cNewParams;i++)
			{
				if( (pNewParamInfo[i].pwszName==NULL) != bIsNull )
				{
					for(DBCOUNTITEM i=0;i<cNewParams;i++)
						SysFreeString(pNewParamInfo[i].pwszName);
					CoTaskMemFree(pNewParamInfo);
					return DB_E_BADPARAMETERNAME;
				}
			}
		}
	}
	else // rgParamBindInfo==NULL
	{	// rgParamOrdinals �迭�� Ordinal�� �ش��ϴ� �Ķ���͵��� Discard�Ѵ�.
		if (m_cParams)
		{
			cNewParams = 0;
			pNewParamInfo = (DBPARAMINFO *)CoTaskMemAlloc(m_cParams*sizeof(DBPARAMINFO));

			for(DBCOUNTITEM i=0;i<m_cParams;i++)
			{
				bool bFound = false;
				for(DBCOUNTITEM j=0;j<cParams;j++)
				{
					if(m_pParamInfo[i].iOrdinal==rgParamOrdinals[j])
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

			if(cNewParams==0)
			{
				CoTaskMemFree(pNewParamInfo);
				pNewParamInfo = 0;
			}
		}
	}

FreeAndSet:
	//������ ���� ����
	if(m_pParamInfo)
	{
		for(DBCOUNTITEM i=0;i<m_cParams;i++)
			SysFreeString(m_pParamInfo[i].pwszName);
		CoTaskMemFree(m_pParamInfo);
	}

	m_pParamInfo = pNewParamInfo;
	m_cParams = cNewParams;

	// sort
	if(m_cParams)
	{
		for(DBCOUNTITEM i=0;i<m_cParams;i++)
		{
			DBORDINAL iMin = ~0;
			DBCOUNTITEM iMinIndex = i;
			for(DBCOUNTITEM j=i;j<m_cParams;j++)
			{
				if(m_pParamInfo[j].iOrdinal<iMin)
				{
					iMin = m_pParamInfo[j].iOrdinal;
					iMinIndex = j;
				}
			}

			if(i!=iMinIndex)
			{
				DBPARAMINFO tmp;
				memcpy(&tmp, &m_pParamInfo[i], sizeof(DBPARAMINFO));
				memcpy(&m_pParamInfo[i], &m_pParamInfo[iMinIndex], sizeof(DBPARAMINFO));
				memcpy(&m_pParamInfo[iMinIndex], &tmp, sizeof(DBPARAMINFO));
			}

			// TODO: CUBRIDCas ���� �Ѱ��� �÷� �̸��� �ҹ���, LTM�� �빮�ڶ� ������ �����.
			// �ϴ� �ҹ��ڷ� ����� LTM�� ����Ѵ�.
			if(m_pParamInfo[i].pwszName)
				_wcslwr(m_pParamInfo[i].pwszName);
		}
	}

	return hr;
}
