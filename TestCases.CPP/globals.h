//This header file contains the declaration of global variables
extern IMalloc* g_pIMalloc;
extern LPCOLESTR connectString;
extern CDataSource ds;
extern CSession session;

#define OUTPUT_DEBUG_STRING_ARG(fmt,s) CAtlString str;str.Format(fmt,s);OutputDebugString(str);

#define RETURN_FAIL(hr) if(FAILED((HRESULT)hr)) { AtlTraceErrorRecords((HRESULT)hr); return E_FAIL; }

