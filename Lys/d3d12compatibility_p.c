

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Tue Jan 19 04:14:07 2038
 */
/* Compiler settings for d3d12compatibility.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.01.0622 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if !defined(_M_IA64) && !defined(_M_AMD64) && !defined(_ARM_)


#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */

#pragma optimize("", off ) 

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif /* __RPCPROXY_H_VERSION__ */


#include "d3d12compatibility_h.h"

#define TYPE_FORMAT_STRING_SIZE   3                                 
#define PROC_FORMAT_STRING_SIZE   1                                 
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _d3d12compatibility_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } d3d12compatibility_MIDL_TYPE_FORMAT_STRING;

typedef struct _d3d12compatibility_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } d3d12compatibility_MIDL_PROC_FORMAT_STRING;

typedef struct _d3d12compatibility_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } d3d12compatibility_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const d3d12compatibility_MIDL_TYPE_FORMAT_STRING d3d12compatibility__MIDL_TypeFormatString;
extern const d3d12compatibility_MIDL_PROC_FORMAT_STRING d3d12compatibility__MIDL_ProcFormatString;
extern const d3d12compatibility_MIDL_EXPR_FORMAT_STRING d3d12compatibility__MIDL_ExprFormatString;



#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif
#if !(TARGET_IS_NT60_OR_LATER)
#error You need Windows Vista or later to run this stub because it uses these features:
#error   compiled for Windows Vista.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will fail with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const d3d12compatibility_MIDL_PROC_FORMAT_STRING d3d12compatibility__MIDL_ProcFormatString =
    {
        0,
        {

			0x0
        }
    };

static const d3d12compatibility_MIDL_TYPE_FORMAT_STRING d3d12compatibility__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_d3d12compatibility_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: ID3D12CompatibilityDevice, ver. 0.0,
   GUID={0x8f1c0e3c,0xfae3,0x4a82,{0xb0,0x98,0xbf,0xe1,0x70,0x82,0x07,0xff}} */


/* Object interface: D3D11On12CreatorID, ver. 0.0,
   GUID={0xedbf5678,0x2960,0x4e81,{0x84,0x29,0x99,0xd4,0xb2,0x63,0x0c,0x4e}} */


/* Object interface: D3D9On12CreatorID, ver. 0.0,
   GUID={0xfffcbb7f,0x15d3,0x42a2,{0x84,0x1e,0x9d,0x8d,0x32,0xf3,0x7d,0xdd}} */


/* Object interface: OpenGLOn12CreatorID, ver. 0.0,
   GUID={0x6bb3cd34,0x0d19,0x45ab,{0x97,0xed,0xd7,0x20,0xba,0x3d,0xfc,0x80}} */


/* Object interface: OpenCLOn12CreatorID, ver. 0.0,
   GUID={0x3f76bb74,0x91b5,0x4a88,{0xb1,0x26,0x20,0xca,0x03,0x31,0xcd,0x60}} */


/* Object interface: DirectMLTensorFlowCreatorID, ver. 0.0,
   GUID={0xcb7490ac,0x8a0f,0x44ec,{0x9b,0x7b,0x6f,0x4c,0xaf,0xe8,0xe9,0xab}} */


/* Object interface: DirectMLPyTorchCreatorID, ver. 0.0,
   GUID={0xaf029192,0xfba1,0x4b05,{0x91,0x16,0x23,0x5e,0x06,0x56,0x03,0x54}} */


/* Standard interface: __MIDL_itf_d3d12compatibility_0000_0007, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    d3d12compatibility__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x60001, /* Ndr library version */
    0,
    0x801026e, /* MIDL Version 8.1.622 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

const CInterfaceProxyVtbl * const _d3d12compatibility_ProxyVtblList[] = 
{
    0
};

const CInterfaceStubVtbl * const _d3d12compatibility_StubVtblList[] = 
{
    0
};

PCInterfaceName const _d3d12compatibility_InterfaceNamesList[] = 
{
    0
};


#define _d3d12compatibility_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _d3d12compatibility, pIID, n)

int __stdcall _d3d12compatibility_IID_Lookup( const IID * pIID, int * pIndex )
{
    UNREFERENCED_PARAMETER(pIID);
    UNREFERENCED_PARAMETER(pIndex);
    return 0;
}

const ExtendedProxyFileInfo d3d12compatibility_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _d3d12compatibility_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _d3d12compatibility_StubVtblList,
    (const PCInterfaceName * ) & _d3d12compatibility_InterfaceNamesList,
    0, /* no delegation */
    & _d3d12compatibility_IID_Lookup, 
    0,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#pragma optimize("", on )
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* !defined(_M_IA64) && !defined(_M_AMD64) && !defined(_ARM_) */

