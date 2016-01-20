

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Tue Dec 15 14:22:38 2015
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


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, LIBID_CUBRIDProvider,0x2B22247F,0xE9F7,0x47e8,0xA9,0xB0,0x79,0xE8,0x03,0x9D,0xCF,0xC8);


MIDL_DEFINE_GUID(IID, IID_ICUBRIDSession,0x9660C3D0,0x3387,0x4F86,0x92,0xC5,0x6D,0x5B,0xA9,0x09,0x75,0xC9);


MIDL_DEFINE_GUID(CLSID, CLSID_CCUBRIDErrorLookup,0x3165D76D,0xCB91,0x482f,0x93,0x78,0x00,0xC2,0x16,0xFD,0x5F,0x32);


MIDL_DEFINE_GUID(CLSID, CLSID_CCUBRIDErrorInfo,0xED0E5A7D,0x89F5,0x4862,0xBE,0xF3,0x20,0xE5,0x51,0xE1,0xD0,0x7B);


MIDL_DEFINE_GUID(CLSID, CLSID_CCUBRIDCommand,0x3FA55BC9,0xF4E2,0x4926,0x90,0x6C,0x2B,0x63,0x0A,0x5F,0x85,0x30);


MIDL_DEFINE_GUID(CLSID, CLSID_CCUBRIDSession,0xF4CD8484,0xA670,0x4511,0x8D,0xF5,0xF7,0x7B,0x29,0x42,0xB9,0x85);


MIDL_DEFINE_GUID(CLSID, CLSID_CCUBRIDDataSource,0x15A12058,0x4353,0x4c9a,0x84,0x21,0x23,0xD8,0x0F,0x25,0xEE,0x4E);


MIDL_DEFINE_GUID(CLSID, CLSID_CMultipleResult,0xBD659D91,0x36B5,0x4e4a,0xBE,0x76,0xE9,0x79,0xAE,0xB1,0x32,0xC3);


MIDL_DEFINE_GUID(CLSID, CLSID_CCUBRIDRow,0x32881E3B,0x5F95,0x4019,0xA3,0x6B,0x0E,0xF4,0xE2,0xAA,0xA1,0xDC);


MIDL_DEFINE_GUID(CLSID, CLSID_CCUBRIDStream,0x857539EA,0x0140,0x40be,0xA8,0xE5,0x1F,0x34,0x79,0x91,0xCC,0x0D);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



