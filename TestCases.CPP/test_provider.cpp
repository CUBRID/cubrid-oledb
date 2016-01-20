#include "stdafx.h"

#include <atldbcli.h>
#include <atlconv.h>
#include <oledb.h>
#include <oledberr.h>
#include <atldbcli.h>
#include <atlcoll.h>
#include <atldbsch.h> //schema support

#include "globals.h"
#include "helpers.h"
#include "test_provider.h"


///
/// Test the CUBRID Provider name CLSID interop
///
bool Test_Provider_CLSID()
{
	CLSID	clsid = IID_NULL;
	OLECHAR* pwszCUBRIDProviderName = L"CUBRID.Provider";
	OLECHAR* pwszCUBRIDProviderNameVer = L"CUBRID.Provider.9.0";
	OLECHAR* pwszCUBRIDProviderNameVerWrong = L"CUBRID.Provider.99.99";
	OLECHAR* pwszCUBRIDGUID = L"{15A12058-4353-4C9A-8421-23D80F25EE4E}";
	
	HRESULT hr = CLSIDFromProgID(pwszCUBRIDProviderNameVerWrong, &clsid);
	if(SUCCEEDED(hr))
	{
		return false;
	}

	hr = CLSIDFromProgID(pwszCUBRIDProviderName, &clsid);
	if(!SUCCEEDED(hr))
	{
		return false;
	}
	ATLASSERT(clsid.Data1 == 0x15A12058);
	ATLASSERT(clsid.Data2 == 0x4353);
	ATLASSERT(clsid.Data3 == 0x4C9A);

	hr = CLSIDFromProgID(pwszCUBRIDProviderNameVer, &clsid);
	if(!SUCCEEDED(hr))
	{
		return false;
	}
	ATLASSERT(clsid.Data1 == 0x15A12058);
	ATLASSERT(clsid.Data2 == 0x4353);
	ATLASSERT(clsid.Data3 == 0x4C9A);

	LPOLESTR progID;
	hr = ProgIDFromCLSID(clsid, &progID);
	if(!SUCCEEDED(hr))
	{
		return false;
	}
	ATLASSERT(wcscmp(progID, pwszCUBRIDProviderNameVer) == 0);

	CoTaskMemFree(progID); 

	return true;
}

///
/// Test the CUBRID Provider definition properties
///
bool Test_Providers_List()
{
	CEnumerator oProviders;
	bool ret = false;

	HRESULT hr = oProviders.Open();
	if(SUCCEEDED(hr))
	{
		// The following macro is to initialize the conversion routines
		USES_CONVERSION;

		while(oProviders.MoveNext() == S_OK)
		{
#ifdef _UNICODE
			OutputDebugString(oProviders.m_szName);
			OutputDebugString(L"\n");
#else
			OutputDebugString(W2A(oProviders.m_szName));
			OutputDebugString("\n");
#endif

			if(!wcscmp(oProviders.m_szName, L"CUBRID.Provider"))
			{
				if(!wcscmp(oProviders.m_szDescription, L"CUBRID OLE DB Provider"))
				{
					if(!wcscmp(oProviders.m_szParseName, L"{15A12058-4353-4C9A-8421-23D80F25EE4E}"))
					{
						ret = true;
						break;
					}
				}
			}
		}
		oProviders.Close();
	}

	return ret;
}



