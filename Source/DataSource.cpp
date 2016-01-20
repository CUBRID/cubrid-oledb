////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DataSource.h"
#include "Error.h"
#include "ProviderInfo.h"
#include "CUBRIDStream.h"

CCUBRIDDataSource *CCUBRIDDataSource::GetDataSourcePtr(IObjectWithSite *pSite)
{
	CComPtr<IDBCreateSession> spCom;

	HRESULT hr = pSite->GetSite(__uuidof(IDBCreateSession), (void **)&spCom);
	ATLASSERT(SUCCEEDED(hr));

	return static_cast<CCUBRIDDataSource *>((IDBCreateSession *)spCom);
}

STDMETHODIMP CCUBRIDDataSource::Initialize(void)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDDataSource::Initialize\n");

	ClearError();

	char dbmsver[16];

	{
		int hConn = 0;

		HRESULT hr = Util::Connect(this, &hConn);
		if(FAILED(hr))
			return hr;

		char buf[16];
		T_CCI_ERROR error;
		int rc = cci_get_db_version(hConn, buf, sizeof(buf));
		if (rc < 0)
		{
			ATLASSERT(rc != CCI_ER_CON_HANDLE);

			ATLTRACE2(atlTraceDBProvider, 0, "CCUBRIDDataSource::Initialize : cci_get_db_version failed with rc=%d\n", rc);
			Util::Disconnect(&hConn);

			CComVariant var;
			var = DBPROPVAL_CS_COMMUNICATIONFAILURE;
			SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_CONNECTIONSTATUS, &var);

			return RaiseError(DB_SEC_E_AUTH_FAILED, 0, __uuidof(IDBInitialize), (LPWSTR)0, L"42000"); //SQLSTATE=42000
		}

		rc = cci_get_db_parameter(hConn, CCI_PARAM_MAX_STRING_LENGTH, &PARAM_MAX_STRING_LENGTH, &error);
		if (rc < 0)
		{
			Util::Disconnect(&hConn);

			CComVariant var;
			var = DBPROPVAL_CS_COMMUNICATIONFAILURE;
			SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_CONNECTIONSTATUS, &var);

			return RaiseError(E_FAIL, 0, __uuidof(IDBInitialize), error.err_msg);
		}

		Util::Disconnect(&hConn);

		//TODO Investigate if minor build number should be added
		int a = 0, b = 0, c = 0;
		sscanf(buf, "%2d.%2d.%2d", &a, &b, &c);
		sprintf(dbmsver, "%02d.%02d.%04d", a, b, c);
	}

	{
		HRESULT hr = IDBInitializeImpl<CCUBRIDDataSource>::Initialize();
		if(FAILED(hr))
			return hr;
	}

	//Set properties
	{
		CComVariant var;
		VariantClear(&var);
		VariantInit(&var);
		var = dbmsver;
		SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_DBMSVER, &var);
		//The version is of the form ##.##.####
		SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_PROVIDERVER, &var);
		VariantClear(&var);
		VariantInit(&var);

		GetPropValue(&DBPROPSET_DBINIT, DBPROP_INIT_LOCATION, &var);
		SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_DATASOURCENAME, &var);

		var = DBPROPVAL_CS_INITIALIZED;
		SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_CONNECTIONSTATUS, &var);
	}

	return S_OK;
}

STDMETHODIMP CCUBRIDDataSource::Uninitialize(void)
{
	ATLTRACE2(atlTraceDBProvider, 2, "CCUBRIDDataSource::Uninitialize\n");

	CComVariant var;

	ClearError();

	{
		HRESULT hr = IDBInitializeImpl<CCUBRIDDataSource>::Uninitialize();
		if(FAILED(hr))
			return hr;
	}

	var = DBPROPVAL_CS_UNINITIALIZED;
	SetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_CONNECTIONSTATUS, &var);

	return S_OK;
}

STDMETHODIMP CCUBRIDDataSource::CreateSession(IUnknown *pUnkOuter, REFIID riid, IUnknown **ppDBSession)
{
	if(ppDBSession == NULL)
		return E_INVALIDARG;
	*ppDBSession = NULL;

	{
		CComVariant var;
		HRESULT hr = GetPropValue(&DBPROPSET_DATASOURCEINFO, DBPROP_ACTIVESESSIONS, &var);
		if(FAILED(hr))
			return hr;

		ATLASSERT(var.vt == VT_I4);

		int cActSessions = V_I4(&var);
		if(cActSessions != 0 && this->m_cSessionsOpen >= cActSessions)
			return DB_E_OBJECTCREATIONLIMITREACHED;
	}

	return IDBCreateSessionImpl<CCUBRIDDataSource, CCUBRIDSession>::CreateSession(pUnkOuter, riid, ppDBSession);
}

STDMETHODIMP CCUBRIDDataSource::GetLiteralInfo(ULONG cLiterals, const DBLITERAL rgLiterals[],
																							 ULONG *pcLiteralInfo, DBLITERALINFO **prgLiteralInfo, OLECHAR **ppCharBuffer)
{
	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDDataSource::GetLiteralInfo\n"));
	ObjectLock lock(this);

	ClearError();

	if( pcLiteralInfo )
		*pcLiteralInfo = 0;
	if( prgLiteralInfo )
		*prgLiteralInfo = NULL;
	if( ppCharBuffer )
		*ppCharBuffer = NULL;

	if (!pcLiteralInfo || !prgLiteralInfo || !ppCharBuffer)
		return E_INVALIDARG;
	if( cLiterals != 0 && rgLiterals == NULL )
		return E_INVALIDARG;

	if (!(m_dwStatus & DSF_INITIALIZED))
		return E_UNEXPECTED;

	*ppCharBuffer = (WCHAR *)CoTaskMemAlloc(ProvInfo::size_wszAllStrings);
	if(*ppCharBuffer == NULL)
		return E_OUTOFMEMORY;
	memcpy(*ppCharBuffer, ProvInfo::wszAllStrings, ProvInfo::size_wszAllStrings);

	const UINT numLiteralInfos = ProvInfo::size_LiteralInfos;

	*pcLiteralInfo = ( cLiterals == 0 ? numLiteralInfos : cLiterals );

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
	{
		for(ULONG i = 0; i < *pcLiteralInfo; i++)
		{
			ULONG j;
			for(j = 0; j < numLiteralInfos; j++)
			{
				if(ProvInfo::LiteralInfos[j].lt == rgLiterals[i])
				{
					(*prgLiteralInfo)[i] = ProvInfo::LiteralInfos[j];
					ulSucceeded++;
					break;
				}
			}

			if(j == numLiteralInfos)
			{
				ZeroMemory((*prgLiteralInfo) + i, sizeof(DBLITERALINFO));
				(*prgLiteralInfo)[i].lt = rgLiterals[i];
			}
		}
	}
	else
	{
		for( ; ulSucceeded < numLiteralInfos ; ulSucceeded++ )
			(*prgLiteralInfo)[ulSucceeded] = ProvInfo::LiteralInfos[ulSucceeded];
	}

	if(ulSucceeded == *pcLiteralInfo)
		return S_OK;
	else if(ulSucceeded != 0)
		return DB_S_ERRORSOCCURRED;
	else
	{
		::CoTaskMemFree(*ppCharBuffer);
		*ppCharBuffer = NULL;

		return DB_E_ERRORSOCCURRED;
	}
}

STDMETHODIMP CCUBRIDDataSource::GetKeywords(LPOLESTR *ppwszKeywords)
{
	ATLTRACE(atlTraceDBProvider, 2, _T("CCUBRIDDataSource::GetKeywords\n"));
	ObjectLock lock(this);

	ClearError();

	// check params
	if (ppwszKeywords == NULL)
		return E_INVALIDARG;
	*ppwszKeywords = NULL;

	// check if data source object is initialized
	if (!(m_dwStatus & DSF_INITIALIZED))
		return E_UNEXPECTED;

	//OleDbSchemaGuid.DbInfoKeyword
	OLECHAR Keywords[] =
		L"ABORT,ACTIVE,ADD_MONTHS,AFTER,ALIAS,ASYNC,ATTACH,ATTRIBUTE,"
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

	*ppwszKeywords = (LPWSTR)CoTaskMemAlloc(sizeof(Keywords));
	if( *ppwszKeywords == NULL )
		return E_OUTOFMEMORY;

	wcscpy( *ppwszKeywords, Keywords );

	return S_OK;
}
