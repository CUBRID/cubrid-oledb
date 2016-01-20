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
#include "CUBRIDProvider.h"
#include "DataSource.h"
#include "Error.h"
#include "ProviderInfo.h"

//const OLEDBDECLSPEC GUID DBPROPSET_UNIPROVIDER_DBINIT = {0x7f555b1d,0xc6d2,0x40ce,{0x9a,0xb4,0x49,0x62,0x78,0x1e,0xb6,0x6c}};

//extern "C" {
//char cci_client_name[8] = "ODBC";
//}


CCUBRIDDataSource *CCUBRIDDataSource::GetDataSourcePtr(IObjectWithSite *pSite)
{
	CComPtr<IDBCreateSession> spCom;
	HRESULT hr = pSite->GetSite(__uuidof(IDBCreateSession), (void **)&spCom);
	// ����� ���α׷��� ������, �����ϴ� ��찡 ������?
	ATLASSERT(SUCCEEDED(hr));
	// ���� ������带 �����ذ��� dynamic_cast�� �� �ʿ�� ���� ��
	return static_cast<CCUBRIDDataSource *>((IDBCreateSession *)spCom);
}

STDMETHODIMP CCUBRIDDataSource::Initialize(void)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDDataSource::Initialize\n");

	// ���� ������ü�� �����Ѵ�.
	ClearError();

	// ����������(ID, ��ȣ��)�� �ùٸ��� Ȯ���Ѵ�.
	char dbmsver[16];
	{
		CConnectionProperties props;
		Util::GetConnectionProperties((IDBProperties*) this, props);
		int hConn = cci_connect(CW2A(props.strAddr), props.nPort, CW2A(props.strDB), CW2A(props.strUser), CW2A(props.strPass));
		if(hConn<0)
		{
			if(hConn==CCI_ER_NO_MORE_MEMORY)
				return E_OUTOFMEMORY;

			// wrong hostname
			return DB_SEC_E_AUTH_FAILED;
		}

		char buf[16];
		T_CCI_ERROR error;
		int rc = cci_get_db_version(hConn, buf, sizeof(buf));
		if(rc<0)
		{
			ATLASSERT(rc!=CCI_ER_CON_HANDLE);
			// rc==CCI_ER_CONNECT -> �ּҳ� ��Ʈ�� Ʋ��
			// rc==CAS_ER_DBMS -> DB �̸��̳� ID, ��ȣ�� Ʋ��

			ATLTRACE2(atlTraceDBProvider, 0, "CCUBRIDDataSource::Initialize : cci_get_db_version failed with rc=%d\n", rc);
			cci_disconnect(hConn, &error);

			CComVariant var;
			var = DBPROPVAL_CS_COMMUNICATIONFAILURE;
			SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_CONNECTIONSTATUS, &var);

			return RaiseError(DB_SEC_E_AUTH_FAILED, 0, __uuidof(IDBInitialize), (LPWSTR)0, L"42000");
		}

		rc = cci_get_db_parameter(hConn, CCI_PARAM_MAX_STRING_LENGTH, &PARAM_MAX_STRING_LENGTH, &error);
		if (rc < 0)
		{
			cci_disconnect(hConn, &error);

			CComVariant var;
			var = DBPROPVAL_CS_COMMUNICATIONFAILURE;
			SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_CONNECTIONSTATUS, &var);

			return RaiseError(E_FAIL, 0, __uuidof(IDBInitialize), CA2W(error.err_msg));
		}

		cci_disconnect(hConn, &error);

		switch(props.nCommitLevel)
		{
		case ISOLATIONLEVEL_READUNCOMMITTED:
		case ISOLATIONLEVEL_READCOMMITTED:
		case ISOLATIONLEVEL_REPEATABLEREAD:
		case ISOLATIONLEVEL_SERIALIZABLE:
			break;
		default:
			return XACT_E_ISOLATIONLEVEL;
		}

		int a=0, b=0, c=0;
		sscanf(buf, "%2d.%2d.%2d", &a, &b, &c);
		sprintf(dbmsver, "%02d.%02d.%04d", a, b, c);
	}

	// ATL�� �ʱ�ȭ ��ƾ�� ȣ��
	{
		HRESULT hr = IDBInitializeImpl<CCUBRIDDataSource>::Initialize();
		if(FAILED(hr)) return hr;
	}

	// set properties
	{
		CComVariant var;
		// �б� ���� �Ӽ��̹Ƿ� IDBProperties::SetProperties�� �̿��� �� ����.
		// ��� ���������� IDBProperties�� �̿�� CUtlProps::SetPropValue�� ����Ѵ�.
		// �ܺ������� ������ �� ������, ���������� ������ �� �ֵ���
		// DBPROPFLAGS_CHANGE flag�� �߰��Ѵ�.
		var = dbmsver;
		SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_DBMSVER, &var);

		VariantClear(&var);
		VariantInit(&var);
		var = "2.0.01.004";

		SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_PROVIDERVER, &var);

		GetPropValue(&DBPROPSET_DBINIT, DBPROP_INIT_LOCATION, &var);
		SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_DATASOURCENAME, &var);

		GetPropValue(&DBPROPSET_DBINIT, DBPROP_SESS_AUTOCOMMITISOLEVELS, &var);
		SetPropValue(&DBPROPSET_SESSION, DBPROP_SESS_AUTOCOMMITISOLEVELS, &var);

		var = DBPROPVAL_CS_INITIALIZED;
		SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_CONNECTIONSTATUS, &var);	
	}

	return S_OK;
}

STDMETHODIMP CCUBRIDDataSource::Uninitialize(void)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDDataSource::Uninitialize\n");

	CComVariant var;

	// ���� ������ü�� �����Ѵ�.
	ClearError();

	// ATL�� ��ƾ�� ȣ��
	{
		HRESULT hr = IDBInitializeImpl<CCUBRIDDataSource>::Uninitialize();
		if(FAILED(hr)) return hr;
	}

	var = DBPROPVAL_CS_UNINITIALIZED;
	SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_CONNECTIONSTATUS, &var);

	return S_OK;
}

STDMETHODIMP CCUBRIDDataSource::CreateSession(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppDBSession)
{
	if(ppDBSession==NULL) return E_INVALIDARG;
	*ppDBSession = NULL;

	// DBPROP_ACTIVESESSIONS ���� �̻��� session�� �� �� ����.
	{
		CComVariant var;
		HRESULT hr = GetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_ACTIVESESSIONS, &var);
		if(FAILED(hr)) return hr;

		ATLASSERT(var.vt==VT_I4);
		int cActSessions = V_I4(&var);

		if(cActSessions!=0 && this->m_cSessionsOpen>=cActSessions)
			return DB_E_OBJECTCREATIONLIMITREACHED;
	}

	// DBPROP_MULTIPLECONNECTIONS==FALSE��
	// ���� ���� connection handle�� ���� ���� ������� ����
	/*
	{
		CComVariant var;
		HRESULT hr = GetPropValue(&DBPROPSET_DATASOURCE, DBPROP_MULTIPLECONNECTIONS, &var);
		if(FAILED(hr)) return hr;

		ATLASSERT(var.vt==VT_BOOL);
		bool bMulSessions = V_BOOL(&var);

		if(!bMulSessions && this->m_cSessionsOpen!=0)
			return DB_E_OBJECTOPEN;
	}
	*/

	HRESULT hr = IDBCreateSessionImpl<CCUBRIDDataSource, CCUBRIDSession>::CreateSession(pUnkOuter, riid, ppDBSession);
	if (FAILED(hr)) return hr;

	ICUBRIDSession* pCUBRIDSession = NULL;
	hr = (*ppDBSession)->QueryInterface(__uuidof(ICUBRIDSession), (void**) &pCUBRIDSession);
	if (FAILED(hr))
	{
		(*ppDBSession)->Release();
		return hr;
	}

	hr = pCUBRIDSession->Connect();
	if (FAILED(hr))
	{
		(*ppDBSession)->Release();
		(*ppDBSession)->Release();
		return hr;
	}

	(*ppDBSession)->Release();
	return S_OK;
}

STDMETHODIMP CCUBRIDDataSource::GetLiteralInfo(ULONG cLiterals, const DBLITERAL rgLiterals[],
							  ULONG *pcLiteralInfo, DBLITERALINFO **prgLiteralInfo,
							  OLECHAR **ppCharBuffer)
{
	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDDataSource::GetLiteralInfo\n"));
	ObjectLock lock(this);

	// ���� ������ü�� �����Ѵ�.
	ClearError();

	// �ʱ�ȭ
	if( pcLiteralInfo )
		*pcLiteralInfo = 0;
	if( prgLiteralInfo )
		*prgLiteralInfo = NULL;
	if( ppCharBuffer )
		*ppCharBuffer = NULL;
		
	// �Ķ���� üũ
	if (!pcLiteralInfo || !prgLiteralInfo || !ppCharBuffer)
		return E_INVALIDARG;
	if( cLiterals != 0 && rgLiterals == NULL )
		return E_INVALIDARG;

	// Data Source�� �ʱ�ȭ �Ǿ����� Ȯ��
	if (!(m_dwStatus & DSF_INITIALIZED))
		return E_UNEXPECTED;

	*ppCharBuffer = (WCHAR *)CoTaskMemAlloc(ProvInfo::size_wszAllStrings);
	if(*ppCharBuffer==NULL) return E_OUTOFMEMORY;
	memcpy(*ppCharBuffer, ProvInfo::wszAllStrings, ProvInfo::size_wszAllStrings);

	// �����Ǵ� ��� literal ���� ��
	const UINT numLiteralInfos = ProvInfo::size_LiteralInfos;

	// cLiterals�� 0�̸� ��� literal ���� ����
	*pcLiteralInfo = ( cLiterals==0 ? numLiteralInfos : cLiterals );

	// *pcLiteralInfo ��ŭ�� DBLITERALINFO ����ü �迭 �Ҵ�
	*prgLiteralInfo = (DBLITERALINFO *)CoTaskMemAlloc(*pcLiteralInfo * sizeof(DBLITERALINFO));
	if (!*prgLiteralInfo)
	{
		::CoTaskMemFree(*ppCharBuffer);
		*pcLiteralInfo = 0;
		*ppCharBuffer = 0;
		return E_OUTOFMEMORY;
	}

	ULONG ulSucceeded = 0;
	if(cLiterals)
	{	// �Ϻ� literal ������ ��ȯ
		for(ULONG i=0;i<*pcLiteralInfo;i++)
		{
			ULONG j;
			// ��û�� literal�� LiteralInfos�� �ִ��� ã�ƺ���.
			for(j=0;j<numLiteralInfos;j++)
			{
				if(ProvInfo::LiteralInfos[j].lt==rgLiterals[i])
				{
					(*prgLiteralInfo)[i] = ProvInfo::LiteralInfos[j];
					ulSucceeded++;
					break;
				}
			}

			if(j==numLiteralInfos)
			{	// LiteralInfos�� ���� literal
				// fSupported�� �ڵ����� FALSE�� �ȴ�.
				ZeroMemory((*prgLiteralInfo)+i, sizeof(DBLITERALINFO));
				(*prgLiteralInfo)[i].lt = rgLiterals[i];
			}
		}
	}
	else
	{	// ��� literal ������ ��ȯ
		for( ; ulSucceeded<numLiteralInfos ; ulSucceeded++ )
			(*prgLiteralInfo)[ulSucceeded] = ProvInfo::LiteralInfos[ulSucceeded];
	}

	if(ulSucceeded==*pcLiteralInfo)
		return S_OK;
	else if(ulSucceeded!=0)
		return DB_S_ERRORSOCCURRED;
	else
	{
		// ���忡 ���� string buffer�� free�ϰ�, infos buffer�� ���д�.
		::CoTaskMemFree(*ppCharBuffer);
		*ppCharBuffer = NULL;
		return DB_E_ERRORSOCCURRED;
	}
}

STDMETHODIMP CCUBRIDDataSource::GetKeywords(LPOLESTR *ppwszKeywords)
{
	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDDataSource::GetKeywords\n"));
	ObjectLock lock(this);
	
	// ���� ������ü�� �����Ѵ�.
	ClearError();

	// check params
	if (ppwszKeywords == NULL)
		return E_INVALIDARG;
	*ppwszKeywords = NULL;
	
	// check if data source object is initialized
	if (!(m_dwStatus & DSF_INITIALIZED))
		return E_UNEXPECTED;

	// Ű���� ����Ʈ�� comma�� seperate ���ڷ� �Ͽ� continuous�� �迭�� ����
	OLECHAR Keywords[] = L"ABORT,ACTIVE,ADD_MONTHS,AFTER,ALIAS,ASYNC,ATTACH,ATTRIBUTE,"
						 L"BEFORE,BOOLEAN,BREADTH,CALL,CHANGE,CLASS,CLASSES,CLUSTER,"
						 L"COMMITTED,COMPLETION,COST,CYCLE,DATA,DATA_TYPE___,DECAY_CONSTANT,"
						 L"DEFINED,DEPTH,DICTIONARY,DIFFERENCE,DIRECTORY,DRAND,EACH,"
						 L"ELSEIF,EQUALS,EVALUATE,EVENT,EXCLUDE,FILE,FUNCTION,GDB,GENERAL,"
						 L"GROUPBY_NUM,GROUPS,HOST,IDENTIFIED,IF,IGNORE,INACTIVE,INCREMENT,"
						 L"INDEX,INFINITE,INHERIT,INOUT,INSTANCES,INST_NUM,"
						 L"INTERSECTION,INTRINSIC,INVALIDATE,LAST_DAY,LDB,LEAVE,LESS,LIMIT,LIST,"
						 L"LOCK,LOOP,LPAD,LTRIM,MAXIMUM,MAXVALUE,MAX_ACTIVE,"
						 L"MEMBERS,METHOD,MINVALUE,MIN_ACTIVE,MODIFY,MOD,MONETARY,"
						 L"MONTHS_BETWEEN,MULTISET,MULTISET_OF,NA,NAME,"
						 L"NEW,NOCYCLE,NOMAXVALUE,NOMINVALUE,NONE,OBJECT,OBJECT_ID,OFF,OID,OLD,"
						 L"OPERATION,OPERATORS,OPTIMIZATION,ORDERBY_NUM,OTHERS,OUT,PARAMETERS,"
						 L"PASSWORD,PENDANT,PREORDER,PRINT,PRIORITY,PRIVATE,PROXY,PROTECTED,"
						 L"QUERY,RAND,RECURSIVE,REF,REFERENCING,REGISTER,REJECT,RENAME,REPEATABLE,"
						 L"REPLACE,RESIGNAL,RETURN,RETURNS,ROLE,ROUTINE,ROW,ROWNUM,"
						 L"RPAD,RTRIM,SAVEPOINT,SCOPE___,SEARCH,SENSITIVE,SEQUENCE,"
						 L"SEQUENCE_OF,SERIAL,SERIALIZABLE,SETEQ,SETNEQ,SET_OF,SHARED,SHORT,SIGNAL,"
                         L"SIMILAR,SQLEXCEPTION,SQLWARNING,STABILITY,START,STATEMENT,STATISTICS,STATUS,"
						 L"STDDEV,STRING,STRUCTURE,SUBCLASS,SUBSET,SUBSETEQ,SUPERCLASS,"
						 L"SUPERSET,SUPERSETEQ,SYS_DATE,SYS_TIME,SYS_TIMESTAMP,SYS_USER,"
						 L"TEST,THERE,TIMEOUT,TO_CHAR,TO_DATE,TO_NUMBER,TO_TIMETO_TIMESTAMP,"
						 L"TRACE,TRIGGERS,TYPE,UNCOMMITTED,UNDER,USE,UTIME,VARIABLE,VARIANCE,"
						 L"VCLASS,VIRTUAL,VISIBLE,WAIT,WHILE,WITHOUT";

	// ���۸� �����Ѵ� 
	*ppwszKeywords = (LPWSTR)CoTaskMemAlloc(sizeof(Keywords));
	if( *ppwszKeywords == NULL )
		return E_OUTOFMEMORY;

	// Copy keywords
	wcscpy( *ppwszKeywords, Keywords );
	
	return S_OK;
}
