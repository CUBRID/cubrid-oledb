//Following values are copied from stdafx.h si session.h
static const GUID DBPROPSET_CUBRIDPROVIDER_DBINIT = {0x7f555b1d,0xc6d2,0x40ce,{0x9a,0xb4,0x49,0x62,0x78,0x1e,0xb6,0x6c}};
#define DBPROP_CUBRIDPROVIDER_BROKER_PORT	0x200
#define DBPROP_CUBRIDPROVIDER_FETCH_SIZE	0x201
#define DBPROP_CUBRIDPROVIDER_AUTOCOMMIT	0x202
#define DBPROP_CUBRIDPROVIDER_LOGIN_TIMEOUT	0x203
#define DBPROP_CUBRIDPROVIDER_QUERY_TIMEOUT	0x204
#define DBPROP_CUBRIDPROVIDER_CHARSET	0x205

bool Test_Connect_Basic();
bool Test_Connect_CDBPropSet();
bool Test_Connect_WrongParams_CDBPropSet();
bool Test_Connect_CDBPropSet_ProviderString();
bool Test_Connect_Extended();
bool Test_Connect_Extended_2();
bool Test_Connect_Timeout();
