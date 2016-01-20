////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CUBRIDstream.h"
#include "session.h"
#include "Error.h"

void CCUBRIDStream::Initialize(int hConn, char* cur_oid, const ATLCOLUMNINFO* info)
{
	m_hConn = hConn;
	strcpy(m_OID, cur_oid);

	m_colName = _wcsdup(info->pwszName);
	m_colType = Type::GetCASTypeU(info->wType);
	m_colPrecision = info->bPrecision;
	m_colScale = info->bScale;
}

void CCUBRIDStream::Initialize(int hConn, char* cur_oid, T_CCI_COL_INFO info)
{
	m_hConn = hConn;
	strcpy(m_OID, cur_oid);

	m_colName = _wcsdup(CA2W(info.real_attr));
	m_colType = info.type;
	m_colPrecision = info.precision;
	m_colScale = info.scale;
}

void CCUBRIDStream::Initialize(int hConn, int req, T_CCI_COL_INFO info)
{
	m_hConn = hConn;
	m_hReq = req;
	m_colName = _wcsdup(CA2W(info.real_attr));
	m_colType = info.type;
	m_colPrecision = info.precision;
	m_colScale = info.scale;
	m_curPos = 0;
	HRESULT hr;
	T_CCI_ERROR error;
	switch(m_colType)
	{
	case CCI_U_TYPE_BLOB:
		hr = cci_blob_new(hConn, &blob, &error);
		if(hr < 0)
			RaiseError(S_FALSE, 0, __uuidof(ISequentialStream));
		break;
	case CCI_U_TYPE_CLOB:
		hr = cci_clob_new(hConn, &clob, &error);
		if(hr < 0)
			RaiseError(S_FALSE, 0, __uuidof(ISequentialStream));
		break;
	}
}

T_CCI_U_TYPE CCUBRIDStream::GetType()
{
	return this->m_colType;
}
T_CCI_BLOB CCUBRIDStream::GetBlob()
{
	return this->blob;
}
T_CCI_BLOB CCUBRIDStream::GetClob()
{
	return this->clob;
}

STDMETHODIMP CCUBRIDStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	HRESULT hr = S_OK;
	void*		buffer = NULL;
	ULONG		len, bRead;
	int			res, ind;
	char* attr_list[2];
	T_CCI_ERROR error;

	ClearError();

	if (!pv || !cb)
		return RaiseError(STG_E_INVALIDPOINTER, 0, __uuidof(ISequentialStream));

	attr_list[0] = _strdup(CW2A(m_colName));
	attr_list[1] = NULL;

	res = cci_oid_get(m_hConn, m_OID, attr_list, &error);
	if (res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(ISequentialStream), error.err_msg);

	m_hReq = res;
	res = cci_cursor(m_hReq, 1, CCI_CURSOR_FIRST, &error);
	if (res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(ISequentialStream), error.err_msg);
	res = cci_fetch(m_hReq, &error);
	if (res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(ISequentialStream), error.err_msg);

	switch (m_colType)
	{
		/*
		case CCI_U_TYPE_SHORT:
		case CCI_U_TYPE_INT:
		if (m_curPos == sizeof(int))
		{
		*pcbRead = 0;
		break;
		}
		buffer = (int *)CoTaskMemAlloc(sizeof(int));
		res = cci_get_data(m_hReq, m_colIndex, CCI_A_TYPE_INT, buffer, &ind);
		if (res < 0)
		goto error;

		if (ind < 0) //if null
		*pcbRead = 0;
		else
		{
		memcpy(pv, buffer, sizeof(int));
		if (pcbRead)
		*pcbRead = sizeof(int);
		m_curPos += sizeof(int);
		}
		CoTaskMemFree(buffer);

		break;
		case CCI_U_TYPE_FLOAT:
		if (m_curPos == sizeof(float))
		{
		*pcbRead = 0;
		break;
		}
		buffer = (float *)CoTaskMemAlloc(sizeof(float));
		res = cci_get_data(m_hReq, m_colIndex, CCI_A_TYPE_FLOAT, buffer, &ind);
		if (res < 0)
		goto error;

		if (ind < 0) //if null
		*pcbRead = 0;
		else
		{
		memcpy(pv, buffer, sizeof(float));
		if (pcbRead)
		*pcbRead = sizeof(float);
		m_curPos += sizeof(float);
		}
		CoTaskMemFree(buffer);

		break;
		case CCI_U_TYPE_DOUBLE:
		case CCI_U_TYPE_MONETARY:
		if (m_curPos == sizeof(double))
		{
		*pcbRead = 0;
		break;
		}
		buffer = (double *)CoTaskMemAlloc(sizeof(double));
		res = cci_get_data(m_hReq, m_colIndex, CCI_A_TYPE_DOUBLE, buffer, &ind);
		if (res < 0)
		goto error;

		if (ind < 0) //if null
		*pcbRead = 0;
		else
		{
		memcpy(pv, buffer, sizeof(double));
		if (pcbRead)
		*pcbRead = sizeof(double);
		m_curPos += sizeof(double);
		}
		CoTaskMemFree(buffer);

		break;
		*/
	case CCI_U_TYPE_BIT:
	case CCI_U_TYPE_VARBIT:
		buffer = (T_CCI_BIT *)CoTaskMemAlloc(sizeof(T_CCI_BIT));
		res = cci_get_data(m_hReq, 1, CCI_A_TYPE_BIT, buffer, &ind);
		if (res < 0)
			goto error;

		len = ((T_CCI_BIT *)buffer)->size;
		if (m_curPos >= len)
		{
			*pcbRead = 0;
			break;
		}

		if (ind < 0) //if NULL
			*pcbRead = 0;
		else
		{
			bRead = (cb >= len - m_curPos) ? len - m_curPos : cb;
			memcpy(pv, ((T_CCI_BIT *)buffer)->buf + m_curPos, bRead);
			if (pcbRead) *pcbRead = bRead;
			m_curPos += bRead;
		}
		CoTaskMemFree(buffer);

		break;
	case CCI_U_TYPE_CHAR:
	case CCI_U_TYPE_STRING:
		res = cci_get_data(m_hReq, 1, CCI_A_TYPE_STR, &buffer, &ind);
		if (res < 0)
			goto error;

		if (ind < 0) //if NULL
			*pcbRead = 0;
		else
		{
			len = (ULONG)strlen((char *)buffer);
			bRead = (cb >= len - m_curPos) ? len - m_curPos : cb;
			memcpy(pv, (char *)buffer + m_curPos, bRead);
			m_curPos += bRead;

			if (pcbRead)
				*pcbRead = bRead;
		}
		break;
	case CCI_U_TYPE_NCHAR:
	case CCI_U_TYPE_VARNCHAR:
		res = cci_get_data(m_hReq, 1, CCI_A_TYPE_STR, &buffer, &ind);
		if (res < 0)
			goto error;

		if (ind < 0) //if NULL
			*pcbRead = 0;
		else
		{
			len = (ULONG)strlen((char *)buffer);
			bRead = (cb >= len - m_curPos) ? len - m_curPos : cb;
			m_curPos += bRead;

			//CA2W wBuf((char *)buffer);
			memcpy(pv, buffer, bRead);
			if (bRead) bRead /= 2;
			*(WCHAR *)((BYTE *)pv + bRead) = L'\0';

			if (pcbRead)
				*pcbRead = bRead;
		}
		break;
	case CCI_U_TYPE_BLOB:
		T_CCI_BLOB blob;
		char* blob_buffer;
		res = cci_get_data(m_hReq, m_colIndex + 1, CCI_A_TYPE_BLOB, (void *)&blob, &ind);

		if ( res < 0 )
			goto error;

		len = (ULONG)cci_blob_size(blob);
		if( len < 0 )
			goto error;

		if(len > cb)
		{
			len = cb;
		}

		blob_buffer = new char[len];
		res = cci_blob_read(m_hConn, blob, m_curPos, len, blob_buffer, &error);

		if ( res < 0 )
			goto error;

		if ( blob_buffer == NULL )
			goto error;
		else
		{
			memcpy(pv, blob_buffer, res);
			*pcbRead = (ULONG)res;
			m_curPos += res;
		}

		cci_blob_free(blob);
		break;
	case CCI_U_TYPE_CLOB:
		T_CCI_CLOB clob;
		char* clob_buffer;
		res = cci_get_data(m_hReq, m_colIndex + 1, CCI_A_TYPE_CLOB, (void *)&clob, &ind);

		if ( res < 0 )
			goto error;

		len = (ULONG)cci_clob_size(clob);
		if( len < 0 )
			goto error;

		if(len > cb)
		{
			len = cb;
		}

		clob_buffer = new char[len];
		res = cci_clob_read(m_hConn, clob, m_curPos, len, clob_buffer, &error);

		if ( res < 0 )
			goto error;

		if ( clob_buffer == NULL )
			goto error;
		else
		{
			memcpy(pv, clob_buffer, res);
			*pcbRead = (ULONG)res;
			m_curPos += res;
		}

		cci_clob_free(clob);
		break;
		/*
		case CCI_U_TYPE_NUMERIC:
		if (m_curPos == sizeof(DB_NUMERIC))
		{
		*pcbRead = 0;
		break;
		}

		res = cci_get_data(m_hReq, m_colIndex, CCI_A_TYPE_STR, &buffer, &ind);
		if (res < 0)
		goto error;

		if (ind < 0) //if null
		*pcbRead = 0;
		else
		{
		DB_NUMERIC* numeric = (DB_NUMERIC *)CoTaskMemAlloc(sizeof(DB_NUMERIC));
		DBLENGTH DstLen;
		DBSTATUS dbStat = DBSTATUS_S_OK;
		DBLENGTH cbMaxLen;

		cbMaxLen = 38;
		len = (ULONG)strlen((char *)buffer);

		hr = m_spConvert->DataConvert(DBTYPE_STR, DBTYPE_NUMERIC, cbMaxLen, &DstLen,
		buffer, numeric, cbMaxLen, dbStat, &dbStat, m_colPrecision, m_colScale, 0);
		if (FAILED(hr))
		goto error;

		bRead = sizeof(DB_NUMERIC);
		memcpy(pv, numeric, sizeof(DB_NUMERIC));
		m_curPos += bRead;
		if (pcbRead)
		*pcbRead = bRead;
		CoTaskMemFree(numeric);
		}
		break;

		case CCI_U_TYPE_DATE:
		buffer = (T_CCI_DATE *)CoTaskMemAlloc(sizeof(T_CCI_DATE));
		res = cci_get_data(m_hReq, m_colIndex, CCI_A_TYPE_DATE, buffer, &ind);
		if (res < 0)
		goto error;

		if (m_curPos >= sizeof(DBDATE))
		{
		*pcbRead = 0;
		break;
		}

		if (ind < 0) //if null
		*pcbRead = 0;
		else
		{
		DBDATE oleTime;
		oleTime.year = ((T_CCI_DATE *)buffer)->yr;
		oleTime.month = ((T_CCI_DATE *)buffer)->mon;
		oleTime.day = ((T_CCI_DATE *)buffer)->day;

		bRead = sizeof(DBDATE);
		memcpy(pv, &oleTime, bRead);
		if (pcbRead) *pcbRead = bRead;
		m_curPos += bRead;
		}
		CoTaskMemFree(buffer);
		break;

		case CCI_U_TYPE_TIME:
		buffer = (T_CCI_DATE *)CoTaskMemAlloc(sizeof(T_CCI_DATE));
		res = cci_get_data(m_hReq, m_colIndex, CCI_A_TYPE_DATE, buffer, &ind);
		if (res < 0)
		goto error;

		if (m_curPos >= sizeof(DBTIME))
		{
		*pcbRead = 0;
		break;
		}

		if (ind < 0) //if null
		*pcbRead = 0;
		else
		{
		DBTIME oleTime;
		oleTime.hour = ((T_CCI_DATE *)buffer)->hh;
		oleTime.minute = ((T_CCI_DATE *)buffer)->mm;
		oleTime.second = ((T_CCI_DATE *)buffer)->ss;

		bRead = sizeof(DBTIME);
		memcpy(pv, &oleTime, bRead);
		if (pcbRead)
		*pcbRead = bRead;
		m_curPos += bRead;
		}
		CoTaskMemFree(buffer);

		break;
		case CCI_U_TYPE_TIMESTAMP:
		buffer = (T_CCI_DATE *)CoTaskMemAlloc(sizeof(T_CCI_DATE));
		res = cci_get_data(m_hReq, m_colIndex, CCI_A_TYPE_DATE, buffer, &ind);
		if (res < 0)
		goto error;

		if (m_curPos >= sizeof(DBTIMESTAMP))
		{
		*pcbRead = 0;
		break;
		}

		if (ind < 0) //if null
		*pcbRead = 0;
		else
		{
		DBTIMESTAMP oleTime;
		oleTime.year = ((T_CCI_DATE *)buffer)->yr;
		oleTime.month = ((T_CCI_DATE *)buffer)->mon;
		oleTime.day = ((T_CCI_DATE *)buffer)->day;
		oleTime.hour = ((T_CCI_DATE *)buffer)->hh;
		oleTime.minute = ((T_CCI_DATE *)buffer)->mm;
		oleTime.second = ((T_CCI_DATE *)buffer)->ss;
		oleTime.fraction = 0;

		bRead = sizeof(DBTIMESTAMP);
		memcpy(pv, &oleTime, bRead);
		if (pcbRead)
		*pcbRead = bRead;
		m_curPos += bRead;
		}
		CoTaskMemFree(buffer);
		break;

		default:
		res = cci_get_data(m_hReq, 1, CCI_A_TYPE_STR, &buffer, &ind);
		if (res < 0)
		goto error;

		if (ind < 0) //if null
		*pcbRead = 0;
		else
		{
		len = (ULONG)strlen((char *)buffer);
		bRead = (cb >= len - m_curPos)?len - m_curPos : cb;
		memcpy(pv, (char *)buffer + m_curPos, bRead);
		m_curPos += bRead;

		if (pcbRead)
		*pcbRead = bRead;
		}
		break;
		*/
	}

	cci_close_req_handle(m_hReq);

	return hr;

error:
	pv = NULL;
	return RaiseError(S_FALSE, 0, __uuidof(ISequentialStream));
}

STDMETHODIMP CCUBRIDStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	HRESULT hr = S_OK;
	int rc;
	T_CCI_ERROR error;
	ULONG res;

	if (!pv)
		return STG_E_INVALIDPOINTER;

	if (cb == 0)
	{
		char* attr_list[2];
		char* new_val_list[2];

		if (!m_curPos)
			return hr;

		attr_list[0] = _strdup(CW2A(m_colName));
		attr_list[1] = NULL;
		new_val_list[0] = _strdup(m_writeBuf);
		new_val_list[1] = NULL;

		if (cci_oid_put(m_hConn, m_OID, attr_list, new_val_list, &error) < 0)
			goto WriteError;

		rc = cci_end_tran(m_hConn, CCI_TRAN_COMMIT, &error);
		if (rc < 0)
			goto WriteError;
	}
	else
	{
		if (!m_writeBuf)
		{
			m_writeBuf = (char *)CoTaskMemAlloc(cb * sizeof(char));
			memcpy(m_writeBuf, pv, cb);
			//m_curPos = cb;
		}
		else
		{
			//m_writeBuf = CoTaskMemRealloc(m_writeBuf, m_curPos + cb);
			memcpy(m_writeBuf + m_curPos, pv, cb);
			//m_curPos += cb;
		}
	}

	switch(m_colType)
	{
	case CCI_U_TYPE_BLOB:
		res = cci_blob_write(m_hConn, blob, m_curPos, cb, (char*)pv, &error);
		if(res < 0)
			goto WriteError;
		m_curPos += res;
		*pcbWritten = res;
		break;
	case CCI_U_TYPE_CLOB:
		res = cci_clob_write(m_hConn, clob, m_curPos, cb, (char*)pv, &error);
		if(res < 0)
			goto WriteError;
		m_curPos += res;
		*pcbWritten = res;
		break;
	}


	return hr;

WriteError:
	return RaiseError(STG_E_WRITEFAULT, 1, __uuidof(ISequentialStream), error.err_msg);
}
