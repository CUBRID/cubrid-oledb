

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Sat Sep 13 14:14:51 2014
 */
/* Compiler settings for .\_CUBRIDProvider.idl:
    Oicf, W1, Zp8, env=Win64 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __CUBRIDProvider_h__
#define __CUBRIDProvider_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ICUBRIDSession_FWD_DEFINED__
#define __ICUBRIDSession_FWD_DEFINED__
typedef interface ICUBRIDSession ICUBRIDSession;
#endif 	/* __ICUBRIDSession_FWD_DEFINED__ */


#ifndef __CCUBRIDErrorLookup_FWD_DEFINED__
#define __CCUBRIDErrorLookup_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCUBRIDErrorLookup CCUBRIDErrorLookup;
#else
typedef struct CCUBRIDErrorLookup CCUBRIDErrorLookup;
#endif /* __cplusplus */

#endif 	/* __CCUBRIDErrorLookup_FWD_DEFINED__ */


#ifndef __CCUBRIDErrorInfo_FWD_DEFINED__
#define __CCUBRIDErrorInfo_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCUBRIDErrorInfo CCUBRIDErrorInfo;
#else
typedef struct CCUBRIDErrorInfo CCUBRIDErrorInfo;
#endif /* __cplusplus */

#endif 	/* __CCUBRIDErrorInfo_FWD_DEFINED__ */


#ifndef __CCUBRIDCommand_FWD_DEFINED__
#define __CCUBRIDCommand_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCUBRIDCommand CCUBRIDCommand;
#else
typedef struct CCUBRIDCommand CCUBRIDCommand;
#endif /* __cplusplus */

#endif 	/* __CCUBRIDCommand_FWD_DEFINED__ */


#ifndef __CCUBRIDSession_FWD_DEFINED__
#define __CCUBRIDSession_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCUBRIDSession CCUBRIDSession;
#else
typedef struct CCUBRIDSession CCUBRIDSession;
#endif /* __cplusplus */

#endif 	/* __CCUBRIDSession_FWD_DEFINED__ */


#ifndef __CCUBRIDDataSource_FWD_DEFINED__
#define __CCUBRIDDataSource_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCUBRIDDataSource CCUBRIDDataSource;
#else
typedef struct CCUBRIDDataSource CCUBRIDDataSource;
#endif /* __cplusplus */

#endif 	/* __CCUBRIDDataSource_FWD_DEFINED__ */


#ifndef __CMultipleResult_FWD_DEFINED__
#define __CMultipleResult_FWD_DEFINED__

#ifdef __cplusplus
typedef class CMultipleResult CMultipleResult;
#else
typedef struct CMultipleResult CMultipleResult;
#endif /* __cplusplus */

#endif 	/* __CMultipleResult_FWD_DEFINED__ */


#ifndef __CCUBRIDRow_FWD_DEFINED__
#define __CCUBRIDRow_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCUBRIDRow CCUBRIDRow;
#else
typedef struct CCUBRIDRow CCUBRIDRow;
#endif /* __cplusplus */

#endif 	/* __CCUBRIDRow_FWD_DEFINED__ */


#ifndef __CCUBRIDStream_FWD_DEFINED__
#define __CCUBRIDStream_FWD_DEFINED__

#ifdef __cplusplus
typedef class CCUBRIDStream CCUBRIDStream;
#else
typedef struct CCUBRIDStream CCUBRIDStream;
#endif /* __cplusplus */

#endif 	/* __CCUBRIDStream_FWD_DEFINED__ */


#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __CUBRIDProvider_LIBRARY_DEFINED__
#define __CUBRIDProvider_LIBRARY_DEFINED__

/* library CUBRIDProvider */
/* [helpstring][uuid][version] */ 


EXTERN_C const IID LIBID_CUBRIDProvider;

#ifndef __ICUBRIDSession_INTERFACE_DEFINED__
#define __ICUBRIDSession_INTERFACE_DEFINED__

/* interface ICUBRIDSession */
/* [unique][helpstring][uuid][object] */ 


EXTERN_C const IID IID_ICUBRIDSession;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9660C3D0-3387-4F86-92C5-6D5BA90975C9")
    ICUBRIDSession : public IUnknown
    {
    public:
        virtual /* [helpstring][local][id] */ HRESULT STDMETHODCALLTYPE Connect( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct ICUBRIDSessionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ICUBRIDSession * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ICUBRIDSession * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ICUBRIDSession * This);
        
        /* [helpstring][local][id] */ HRESULT ( STDMETHODCALLTYPE *Connect )( 
            ICUBRIDSession * This);
        
        END_INTERFACE
    } ICUBRIDSessionVtbl;

    interface ICUBRIDSession
    {
        CONST_VTBL struct ICUBRIDSessionVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ICUBRIDSession_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define ICUBRIDSession_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define ICUBRIDSession_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define ICUBRIDSession_Connect(This)	\
    ( (This)->lpVtbl -> Connect(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ICUBRIDSession_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CCUBRIDErrorLookup;

#ifdef __cplusplus

class DECLSPEC_UUID("3165D76D-CB91-482f-9378-00C216FD5F32")
CCUBRIDErrorLookup;
#endif

EXTERN_C const CLSID CLSID_CCUBRIDErrorInfo;

#ifdef __cplusplus

class DECLSPEC_UUID("ED0E5A7D-89F5-4862-BEF3-20E551E1D07B")
CCUBRIDErrorInfo;
#endif

EXTERN_C const CLSID CLSID_CCUBRIDCommand;

#ifdef __cplusplus

class DECLSPEC_UUID("3FA55BC9-F4E2-4926-906C-2B630A5F8530")
CCUBRIDCommand;
#endif

EXTERN_C const CLSID CLSID_CCUBRIDSession;

#ifdef __cplusplus

class DECLSPEC_UUID("F4CD8484-A670-4511-8DF5-F77B2942B985")
CCUBRIDSession;
#endif

EXTERN_C const CLSID CLSID_CCUBRIDDataSource;

#ifdef __cplusplus

class DECLSPEC_UUID("15A12058-4353-4c9a-8421-23D80F25EE4E")
CCUBRIDDataSource;
#endif

EXTERN_C const CLSID CLSID_CMultipleResult;

#ifdef __cplusplus

class DECLSPEC_UUID("BD659D91-36B5-4e4a-BE76-E979AEB132C3")
CMultipleResult;
#endif

EXTERN_C const CLSID CLSID_CCUBRIDRow;

#ifdef __cplusplus

class DECLSPEC_UUID("32881E3B-5F95-4019-A36B-0EF4E2AAA1DC")
CCUBRIDRow;
#endif

EXTERN_C const CLSID CLSID_CCUBRIDStream;

#ifdef __cplusplus

class DECLSPEC_UUID("857539EA-0140-40be-A8E5-1F347991CC0D")
CCUBRIDStream;
#endif
#endif /* __CUBRIDProvider_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


