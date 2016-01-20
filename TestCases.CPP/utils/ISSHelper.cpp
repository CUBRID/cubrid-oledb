// ISSHelper.cpp: implementation of the CISSHelper class.
#include "stdafx.h" // precompiled header directive

#include <windows.h>
#include <stdio.h>// printf(...)
#include <stddef.h>// offsetof(...)
#include <conio.h>// _getch()
#include <oledb.h>// OLEDB
#include <oledberr.h>// OLEDB
#include <objbase.h>// CoInitializeEx()
#include <msdasc.h>// IDataInitialize 
#include <msdadc.h>// OLEDB conversion library header.
#include <msdaguid.h>// OLEDB conversion library guids.
#include <tchar.h>

#include "ISSHelper.h"

// Implementation of ISequentialStream interface
CISSHelper::CISSHelper(__int64 i64LobBytes) {
	m_cRef= 0;
	m_pBuffer= NULL;
	m_ulLength= 0;
	m_ulStatus   = DBSTATUS_S_OK;
	m_iReadPos= 0;
	m_iWritePos= 0;
	m_i64ThreshHold = i64LobBytes;
	m_i64BytesUploaded = 0;
	m_i64LastPrintUploaded = 0;
	m_dwStartTick = 0;
	m_dwParamOrdinal = 0;
}

CISSHelper::~CISSHelper() {
	Clear();
}

void CISSHelper::Clear() {
	CoTaskMemFree( m_pBuffer );
	m_cRef= 0;
	m_pBuffer= NULL;
	m_ulLength= 0;
	m_ulStatus  = DBSTATUS_S_OK;
	m_iReadPos= 0;
	m_iWritePos= 0;
}

ULONG CISSHelper::AddRef() {
	return ++m_cRef;
}

ULONG CISSHelper::Release() {
	return --m_cRef;
}

HRESULT CISSHelper::QueryInterface( REFIID riid, void** ppv ) {
	*ppv = NULL;
	if ( riid == IID_IUnknown ) *ppv = this;
	if ( riid == IID_ISequentialStream ) *ppv = this;
	if ( *ppv )
	{
		( (IUnknown*) *ppv )->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

HRESULT CISSHelper::Read( void *pv,ULONG cb, ULONG* pcbRead ) {
	// Check parameters.
	if ( pcbRead ) *pcbRead = 0;
	if ( !pv ) return STG_E_INVALIDPOINTER;
	if ( 0 == cb ) return S_OK; 

	// Cut out now if threshold is hit.
	__int64 left = m_i64ThreshHold-m_i64BytesUploaded;
	if (left < cb)
		cb = (ULONG)left;

	if (0 == m_dwStartTick)
		m_dwStartTick = GetTickCount();

	m_i64BytesUploaded += cb;

	if (( m_i64BytesUploaded - m_i64LastPrintUploaded ) >= (1024*1024*4)) {
		__int64 i64Elapsed = (__int64) GetTickCount()-m_dwStartTick;
		// __int64 i64BytesPerSecond = m_i64BytesUploaded * 1000 / (i64Elapsed > 0 ) ? i64Elapsed : 1;
		__int64 i64BytesPerSecond = m_i64BytesUploaded * 1000 / i64Elapsed > 0  ? i64Elapsed : 1;
		printf("Param=%lu TotalBytes=%010I64u ElapsedMS=%010I64u BytesPerSec=%010I64u\n", 
			m_dwParamOrdinal, m_i64BytesUploaded, i64Elapsed, i64BytesPerSecond );
		m_i64LastPrintUploaded = m_i64BytesUploaded;
	}

	if ( cb == left ) {
		__int64 i64Elapsed = (__int64) GetTickCount()-m_dwStartTick;
		// __int64 i64BytesPerSecond = m_i64BytesUploaded * 1000 / (i64Elapsed > 0 ) ? i64Elapsed : 1;
		__int64 i64BytesPerSecond = m_i64BytesUploaded * 1000 / i64Elapsed > 0 ? i64Elapsed : 1;
		printf("Last read:\nParam=%lu TotalBytes=%010I64u ElapsedMS=%010I64u BytesPerSec=%010I64u\n", 
			m_dwParamOrdinal, m_i64BytesUploaded, i64Elapsed, i64BytesPerSecond );
		m_i64LastPrintUploaded = m_i64BytesUploaded;
	}

	*pcbRead = cb;
	return S_OK;
}

HRESULT CISSHelper::Write( const void *pv, ULONG cb, ULONG* pcbWritten ) {
	// Check parameters.
	if ( !pv ) return STG_E_INVALIDPOINTER;
	if ( pcbWritten ) *pcbWritten = 0;
	if ( 0 == cb ) return S_OK;

	// Enlarge the current buffer.
	m_ulLength += cb;

	// Grow internal buffer to new size.
	m_pBuffer = CoTaskMemRealloc( m_pBuffer, m_ulLength );

	// Check for out of memory situation.
	if ( NULL == m_pBuffer ) {
		Clear();
		return E_OUTOFMEMORY;
	}

	// Copy callers memory to internal bufffer and update write position.
	memcpy( (void*)((BYTE*)m_pBuffer + m_iWritePos), pv, cb );
	m_iWritePos += cb;

	// Return bytes written to caller.
	if ( pcbWritten ) *pcbWritten = cb;

	return S_OK;
}
