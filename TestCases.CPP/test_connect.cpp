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
#include "test_connect.h"

///
/// Test simple Connect with no prompt
///
bool Test_Connect_Basic()
{
	CDataSource ds;
	CSession session;

	HRESULT hr = ds.OpenFromInitializationString(connectString, false);
	if(SUCCEEDED(hr))
	{
		hr = session.Open(ds);
		if(SUCCEEDED(hr))
		{
			CloseConnection();
			return true;
		}
	}

	ds.Close();
	return false;
}

///
/// Test Connect with CDBPropSet
///
bool Test_Connect_CDBPropSet()
{
	CDataSource ds;
	CSession session;

  CDBPropSet propset(DBPROPSET_DBINIT);
	propset.AddProperty(DBPROP_INIT_LOCATION, L"localhost");
  propset.AddProperty(DBPROP_INIT_DATASOURCE, L"demodb");
  //TODO How to set Port?
	propset.AddProperty(DBPROP_AUTH_USERID, L"public");
  propset.AddProperty(DBPROP_AUTH_PASSWORD, L"");

	HRESULT hr = ds.Open("CUBRID.Provider", &propset);
	if(SUCCEEDED(hr))
	{
		//Open a session
		hr = session.Open(ds);
		if(SUCCEEDED(hr))
		{
			CloseConnection();
			return true;
		}
	}

	ds.Close();
	return false;
}

///
/// Test Connect fails with wrong params
///
bool Test_Connect_WrongParams_CDBPropSet()
{
	CDataSource ds;

  CDBPropSet propset1(DBPROPSET_DBINIT);
	propset1.AddProperty(DBPROP_INIT_LOCATION, L"localhostx");
  propset1.AddProperty(DBPROP_INIT_DATASOURCE, L"demodb");
	propset1.AddProperty(DBPROP_AUTH_USERID, L"public");
  propset1.AddProperty(DBPROP_AUTH_PASSWORD, L"");
	HRESULT hr = ds.Open("CUBRID.Provider", &propset1);
	if(SUCCEEDED(hr))
	{
			ds.Close();
			return false;
	}

  CDBPropSet propset2(DBPROPSET_DBINIT);
	propset2.AddProperty(DBPROP_INIT_LOCATION, L"localhost");
  propset2.AddProperty(DBPROP_INIT_DATASOURCE, L"demodbx");
	propset2.AddProperty(DBPROP_AUTH_USERID, L"public");
  propset2.AddProperty(DBPROP_AUTH_PASSWORD, L"");
	hr = ds.Open("CUBRID.Provider", &propset2);
	if(SUCCEEDED(hr))
	{
			ds.Close();
			return false;
	}

  CDBPropSet propset3(DBPROPSET_DBINIT);
	propset3.AddProperty(DBPROP_INIT_LOCATION, L"localhost");
  propset3.AddProperty(DBPROP_INIT_DATASOURCE, L"demodb");
	propset3.AddProperty(DBPROP_AUTH_USERID, L"publicx");
  propset3.AddProperty(DBPROP_AUTH_PASSWORD, L"");
	_ATLTRY
	{
		hr = ds.Open("CUBRID.Provider", &propset3);
		if(SUCCEEDED(hr))
		{
				ds.Close();
				return false;
		}
	}
	_ATLCATCH(ex)
	{
		ds.Close();
		return false;
	}

  CDBPropSet propset4(DBPROPSET_DBINIT);
	propset4.AddProperty(DBPROP_INIT_LOCATION, L"localhost");
  propset4.AddProperty(DBPROP_INIT_DATASOURCE, L"demodb");
	propset4.AddProperty(DBPROP_AUTH_USERID, L"public");
  propset4.AddProperty(DBPROP_AUTH_PASSWORD, L"x");
	hr = ds.Open("CUBRID.Provider", &propset4);
	if(SUCCEEDED(hr))
	{
			ds.Close();
			return false;
	}

	return true;
}

///
/// Test Connect with CDBPropSet and ProviderString
///
bool Test_Connect_CDBPropSet_ProviderString()
{
	CDataSource ds;
	CSession session;

  CDBPropSet propset(DBPROPSET_DBINIT);
	LPCOLESTR providerString = L"Provider=CUBRID.Provider;Location=localhost;Data Source=demodb;User Id=public;Port=33000";
  propset.AddProperty(DBPROP_INIT_PROVIDERSTRING, providerString);
  propset.AddProperty(DBPROP_AUTH_PASSWORD, L"");

	HRESULT hr = ds.Open("CUBRID.Provider", &propset);
	if(SUCCEEDED(hr))
	{
		hr = session.Open(ds);
		if(SUCCEEDED(hr))
		{
			CloseConnection();
			return true;
		}
	}

	ds.Close();
	return false;
}

///
/// Test Connect with CDBPropSet and ProviderString extended properties
///
bool Test_Connect_Extended()
{
	CDataSource ds;
	CSession session;

  CDBPropSet propset(DBPROPSET_DBINIT);
	TCHAR *providerString = L"Provider=CUBRID.Provider;Location=localhost;Data Source=demodb;User Id=public;Port=33000;Autocommit=false";
  propset.AddProperty(DBPROP_INIT_PROVIDERSTRING, providerString);
  propset.AddProperty(DBPROP_AUTH_PASSWORD, L"");

	HRESULT hr = ds.Open("CUBRID.Provider", &propset);
	if(SUCCEEDED(hr))
	{
		hr = session.Open(ds);
		if(SUCCEEDED(hr))
		{
			session.Close();
			ds.Close();
		}
	}
	else
	{
		ds.Close();
		return false;
	}

  CDBPropSet propset2(DBPROPSET_DBINIT);
	providerString = L"Provider=CUBRID.Provider;Location=localhost;Data Source=demodb;User Id=public;Port=33000 \
										;Query timeout=20000;Login timeout=20000;Charset=UTF-8";
  propset2.AddProperty(DBPROP_INIT_PROVIDERSTRING, providerString);
  propset2.AddProperty(DBPROP_AUTH_PASSWORD, L"");

	hr = ds.Open("CUBRID.Provider", &propset2);
	if(SUCCEEDED(hr))
	{
		hr = session.Open(ds);
		if(SUCCEEDED(hr))
		{
			session.Close();
			ds.Close();
		}
	}
	else
	{
		ds.Close();
		return false;
	}

	return true;
}

///
/// Test Connect with CDBPropSet extended properties
///
bool Test_Connect_Extended_2()
{
	CDataSource ds;
	CSession session;

	//http://support.microsoft.com/kb/191747
	CDBPropSet propset[2] = {DBPROPSET_DBINIT, DBPROPSET_CUBRIDPROVIDER_DBINIT};

	propset[0].AddProperty(DBPROP_INIT_LOCATION, L"localhost");
  propset[0].AddProperty(DBPROP_INIT_DATASOURCE, L"demodb");
	propset[0].AddProperty(DBPROP_AUTH_USERID, L"public");
  propset[0].AddProperty(DBPROP_AUTH_PASSWORD, L"");

	propset[1].AddProperty(DBPROP_CUBRIDPROVIDER_BROKER_PORT, (long)30000);
  propset[1].AddProperty(DBPROP_CUBRIDPROVIDER_FETCH_SIZE, (long)999);
  propset[1].AddProperty(DBPROP_CUBRIDPROVIDER_LOGIN_TIMEOUT, (long)20000);
  propset[1].AddProperty(DBPROP_CUBRIDPROVIDER_QUERY_TIMEOUT, (long)20000);
  propset[1].AddProperty(DBPROP_CUBRIDPROVIDER_AUTOCOMMIT, false); //OFF

	HRESULT hr = ds.Open("CUBRID.Provider", propset, 2);
	if(SUCCEEDED(hr))
	{
		TCHAR *conn_str = L"Provider=CUBRID.Provider.9.0;Location=localhost;Data Source=demodb;User ID=public;Cache Authentication=False;Encrypt Password=False;Mask Password=False;Persist Encrypted=False;Persist Security Info=False;Port=30000;Fetch Size=999;Login timeout=20000;Query timeout=20000;Autocommit=False;Charset=UTF-8";
		BSTR bstr;
    ds.GetInitializationString(&bstr);

		if(wcscmp(bstr, conn_str) != 0)
		{
			ds.Close();
			return false;
		}

		hr = session.Open(ds);
		if(SUCCEEDED(hr))
		{
			session.Close();
			ds.Close();
		}
	}
	else
	{
		ds.Close();
		return false;
	}

	return true;
}

///
/// Test Connect timeout property
///
bool Test_Connect_Timeout()
{
	CDataSource ds;
	CSession session;

	//http://support.microsoft.com/kb/191747
	CDBPropSet propset[2] = {DBPROPSET_DBINIT, DBPROPSET_CUBRIDPROVIDER_DBINIT};

	propset[0].AddProperty(DBPROP_INIT_LOCATION, L"localhost");
  propset[0].AddProperty(DBPROP_INIT_DATASOURCE, L"demodb");
	propset[0].AddProperty(DBPROP_AUTH_USERID, L"public");
  propset[0].AddProperty(DBPROP_AUTH_PASSWORD, L"");

  propset[1].AddProperty(DBPROP_CUBRIDPROVIDER_LOGIN_TIMEOUT, (long)5000);

	HRESULT hr = ds.Open("CUBRID.Provider", propset, 2);
	if(SUCCEEDED(hr))
	{
		TCHAR *conn_str = L"Provider=CUBRID.Provider.9.0;Location=localhost;Data Source=demodb;User ID=public;Cache Authentication=False;Encrypt Password=False;Mask Password=False;Persist Encrypted=False;Persist Security Info=False;Port=30000;Fetch Size=10000;Login timeout=5000;Query timeout=10000;Autocommit=True;Charset=UTF-8";
		BSTR bstr;
    ds.GetInitializationString(&bstr);

		if(wcscmp(bstr, conn_str) != 0)
		{
			ds.Close();
			return false;
		}

		hr = session.Open(ds);
		if(SUCCEEDED(hr))
		{
			session.Close();
			ds.Close();
		}
	}
	else
	{
		ds.Close();
		return false;
	}

	return true;
}


