////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Application: CUBRID OLE DB Data Provider (http://www.cubrid.org/wiki_apis/entry/cubrid-oledb-driver)
// License: Released under a BSD license: http://www.cubrid.org/license, http://www.opensource.org/licenses/BSD-3-Clause
// Version: 8.4.1, compatible with CUBRID 8.4.1 release (http://www.cubrid.org/?mid=downloads&item=cubrid&os=detect)
// Date: March-April 2012
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "type.h"
#include "RowsetRow.h"
#include "Error.h"
#include "Rowset.h"
#include "Session.h"
#include "CUBRIDStream.h"

class CCUBRIDRowsetRowColumn
{
public:
	CCUBRIDRowsetRowColumn() : m_strData(0), m_cbDataLen(0), m_dwStatus(DBSTATUS_S_ISNULL)
	{
		ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRowColumn::CCUBRIDRowsetRowColumn\n");
	}

	~CCUBRIDRowsetRowColumn()
	{
		ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRowColumn::~CCUBRIDRowsetRowColumn\n");
		if(m_strData) free(m_strData);
	}

	char *m_strData;
	DBLENGTH m_cbDataLen;
	DBSTATUS m_dwStatus;

	HRESULT TransferData(CComPtr<IDataConvert> &spConvert, int iOrdinal, DBTYPE wType,
		BYTE bPrecision, BYTE bScale, void *pData, DBLENGTH cbMaxLen,
		DBSTATUS *pdwStatus, DBLENGTH *pcbDataLen);
	HRESULT ReadData(CComPtr<IDataConvert> &spConvert, DBTYPE wType,
		BYTE *pData, DBLENGTH cbLength, ATLCOLUMNINFO *pInfo);
	HRESULT ReadData(int hReq, int iOrdinal, DBTYPE wType);
	HRESULT WriteData(int hReq, int nPos, ATLCOLUMNINFO *pInfo);
	HRESULT WriteData(int hReq, int nPos, ATLCOLUMNINFO *pInfo, CCUBRIDStream* s);
};

HRESULT CCUBRIDRowsetRowColumn::TransferData(CComPtr<IDataConvert> &spConvert, int /*iOrdinal*/,
																						 DBTYPE wType, BYTE bPrecision, BYTE bScale, void *pData,
																						 DBLENGTH cbMaxLen, DBSTATUS *pdwStatus, DBLENGTH *pcbDataLen)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowsetRowColumn::TransferData\n");

	if(m_dwStatus == DBSTATUS_E_UNAVAILABLE)
	{
		*pdwStatus = DBSTATUS_E_UNAVAILABLE;
		*pcbDataLen = 0;
		if(pData)
			memset(pData, 0, cbMaxLen);

		return E_FAIL;
	}

	if(m_dwStatus == DBSTATUS_S_ISNULL)
	{
		*pdwStatus = DBSTATUS_S_ISNULL;
		*pcbDataLen = 0;
		if(pData)
			memset(pData, 0, cbMaxLen);

		return S_OK;
	}

	DBSTATUS dbStat = DBSTATUS_S_OK;
	DBLENGTH cbDst = m_cbDataLen;
	HRESULT hr;

	hr = spConvert->DataConvert(DBTYPE_STR, wType, m_cbDataLen, &cbDst,
		m_strData, pData, cbMaxLen, dbStat, &dbStat, bPrecision, bScale, 0);

	//we do this because CUBRID padds the bit values to the right
	if(wType == DBTYPE_BYTES)
		*(long*)pData = *(long*)pData >> (8 * cbMaxLen - bPrecision);

	if (hr == DB_E_UNSUPPORTEDCONVERSION)
	{
		*pdwStatus = DBSTATUS_E_CANTCONVERTVALUE;
		*pcbDataLen = 0;
		//if(pData)
		//	memset(pData, 0,cbMaxLen);
		return hr;
	}

	*pdwStatus = dbStat;
	*pcbDataLen = ( dbStat == DBSTATUS_S_ISNULL ? 0 : cbDst );

	return hr;
}

HRESULT CCUBRIDRowsetRowColumn::ReadData(CComPtr<IDataConvert> &spConvert, DBTYPE wType,
																				 BYTE *pData, DBLENGTH cbLength, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowsetRowColumn::ReadData(1)\n");

	char *strDataNew = 0;

	DBTYPE wPureType = wType & ~DBTYPE_BYREF;
	if( pData && ((wType & DBTYPE_BYREF) || wType == DBTYPE_BSTR) )
		pData = *(BYTE **)pData;

	switch(wPureType)
	{
	case DBTYPE_STR:
		if(cbLength > pInfo->ulColumnSize)
			return DB_E_CANTCONVERTVALUE;

		strDataNew = _strdup(*(char **)pData);
		break;
	case DBTYPE_BSTR:
	case DBTYPE_WSTR:
		if(cbLength > pInfo->ulColumnSize * 2)
			return DB_E_CANTCONVERTVALUE;

		// TODO: Interpret codepage
		strDataNew = _strdup(CW2A((LPCWSTR)pData));
		break;
	case DBTYPE_I2:
		if(pInfo->wType == DBTYPE_BYTES)
		{
			unsigned short temp = *(short*)pData;
			int bit = 0;
			bool overhead = true;
			unsigned short shift = 0x8000;
			strDataNew = (char *)malloc(pInfo->ulColumnSize * 8);
			do
			{
				if(!(temp & shift) && overhead)
				{
					shift = shift >> 1;
					continue;
				}
				overhead = false;
				strDataNew[bit] = ((temp & shift) ? '1' : '0');
				shift = shift >> 1;
				bit++;
			}
			while(shift != 0);
			strDataNew[bit] = '\0';
			break;
		}
		strDataNew = (char *)malloc(7);
		sprintf(strDataNew, "%d", *(SHORT *)pData);
		break;
	case DBTYPE_UI2:
		strDataNew = (char *)malloc(7);
		sprintf(strDataNew, "%d", *(USHORT *)pData);
		break;
	case DBTYPE_I4:
		if(pInfo->wType == DBTYPE_BYTES)
		{
			unsigned int temp = *(int*)pData;
			int bit = 0;
			bool overhead = true;
			unsigned int shift = 0x80000000;
			strDataNew = (char *)malloc(pInfo->ulColumnSize * 8);
			do
			{
				if(!(temp & shift) && overhead)
				{
					shift = shift >> 1;
					continue;
				}
				overhead = false;
				strDataNew[bit] = ((temp & shift) ? '1' : '0');
				shift = shift >> 1;
				bit++;
			}
			while(shift != 0);
			strDataNew[bit] = '\0';
			break;
		}
		strDataNew = (char *)malloc(12);
		sprintf(strDataNew, "%d", *(LONG *)pData);
		break;
	case DBTYPE_UI4:
		strDataNew = (char *)malloc(12);
		sprintf(strDataNew, "%d", *(ULONG *)pData);
		break;
	case DBTYPE_DBTIME:
		{
			strDataNew = (char *)malloc(9);
			DBTIME time = *(DBTIME *)pData;
			sprintf(strDataNew, "%02d:%02d:%02d", time.hour, time.minute, time.second);
		}
		break;
	case DBTYPE_DBDATE:
		{
			strDataNew = (char *)malloc(11);
			DBDATE date = *(DBDATE *)pData;
			sprintf(strDataNew, "%04d-%02d-%02d", date.year, date.month, date.day);
		}
		break;
	case DBTYPE_DBTIMESTAMP:
		{
			if(pInfo->bPrecision == 19)
			{
				strDataNew = (char *)malloc(20);
				DBTIMESTAMP ts = *(DBTIMESTAMP *)pData;
				sprintf(strDataNew, "%04d-%02d-%02d %02d:%02d:%02d",
					ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second);
			}
			else
			{
				strDataNew = (char *)malloc(24);
				DBTIMESTAMP ts = *(DBTIMESTAMP *)pData;
				sprintf(strDataNew, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
					ts.year, ts.month, ts.day, ts.hour, ts.minute, ts.second, ts.fraction / 1000000);
			}
		}
		break;
	case DBTYPE_R4:
		{
			strDataNew = (char *)malloc(50);
			sprintf(strDataNew, "%f", *(float *)pData);
		}
		break;
	case DBTYPE_R8:
		{
			strDataNew = (char *)malloc(50);
			sprintf(strDataNew, "%f", *(double *)pData);
		}
		break;
	case DBTYPE_BYTES:
		{
			unsigned long temp = *(long*)pData;
			int bit = 0;
			bool overhead = true;
			unsigned long shift = 0x80000000;
			strDataNew = (char *)malloc(pInfo->ulColumnSize * 8);
			do
			{
				if(!(temp & shift) && overhead)
				{
					shift = shift >> 1;
					continue;
				}
				overhead = false;
				strDataNew[bit] = ((temp & shift) ? '1' : '0');
				shift = shift >> 1;
				bit++;
			}
			while(shift != 0);
			strDataNew[bit] = '\0';
		}
		break;
	case DBTYPE_NUMERIC:
		{
			DB_NUMERIC &tmp = *(DB_NUMERIC *)pData;
			strDataNew = (char *)malloc(tmp.precision + 5);
			spConvert->DataConvert(DBTYPE_NUMERIC, DBTYPE_STR, sizeof(DB_NUMERIC), NULL,
				pData, strDataNew, tmp.precision + 4, DBSTATUS_S_OK, NULL, 0, 0, 0);
		}
		break;
	case DBTYPE_UI1:
		{
			if(pInfo->wType == DBTYPE_BYTES)
			{
				BYTE temp = *(BYTE*)pData;
				int bit = 0;
				bool overhead = true;
				BYTE shift = 0x80;
				strDataNew = (char *)malloc(pInfo->ulColumnSize * 8);
				do
				{
					if(!(temp & shift) && overhead)
					{
						shift = shift >> 1;
						continue;
					}
					overhead = false;
					strDataNew[bit] = ((temp & shift) ? '1' : '0');
					shift = shift >> 1;
					bit++;
				}
				while(shift != 0);
				strDataNew[bit] = '\0';
				break;
			}
			return DB_E_CANTCONVERTVALUE;
		}
	default:
		return DB_E_CANTCONVERTVALUE;
	}

	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRowColumn::ReadData Result(type=%d) : %s\n", wType, m_strData);

	if(m_strData)
	{
		free(m_strData);
	}

	m_strData = strDataNew;
	m_cbDataLen = (m_strData ? (DBLENGTH)strlen(m_strData) : 0);
	m_dwStatus = DBSTATUS_S_OK;

	return S_OK;
}

HRESULT CCUBRIDRowsetRowColumn::ReadData(int hReq, int iOrdinal, DBTYPE wType)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowsetRowColumn::ReadData(2)\n");

	if(m_strData)
	{
		free(m_strData);
		m_strData = 0;
	}

	if(iOrdinal == 0)
	{
		// Bookmark Column
		return S_OK;
	}

	// Non-Bookmark Columns
	char *value;
	int ind;

	if(wType == DBTYPE_DBTIME || wType == DBTYPE_DBTIMESTAMP || wType == DBTYPE_DBDATE)
	{
		T_CCI_DATE date;
		if(cci_get_data(hReq, iOrdinal, CCI_A_TYPE_DATE, &date, &ind) < 0)
		{
			m_dwStatus = DBSTATUS_E_UNAVAILABLE;
			return S_OK;
		}

		if(ind == -1) // null value?
		{
			m_dwStatus = DBSTATUS_S_ISNULL;
			return S_OK;
		}

		if(wType == DBTYPE_DBTIME)
		{
			m_strData = (char *)malloc(9);
			sprintf(m_strData, "%02d:%02d:%02d", date.hh, date.mm, date.ss);
			m_cbDataLen = 8;
		}
		else if(wType == DBTYPE_DBTIMESTAMP)
		{
			//m_strData = (char *)malloc(20);
			m_strData = (char *)malloc(24);
			sprintf(m_strData, "%04d-%02d-%02d %02d:%02d:%02d.%03d", date.yr, date.mon, date.day, date.hh, date.mm, date.ss, date.ms);
			m_cbDataLen = 23;
		}
		else if(wType == DBTYPE_DBDATE) // DBTYPE_DBDATE (DATE or DATETIME)
		{
			m_strData = (char *)malloc(20);
			sprintf(m_strData, "%04d-%02d-%02d %02d:%02d:%02d", date.yr, date.mon, date.day, date.hh, date.mm, date.ss);
			m_cbDataLen = 19;
		}

		m_dwStatus = DBSTATUS_S_OK;

		return S_OK;
	}

	//TODO Implement more customized data retrieval?
	if(cci_get_data(hReq, iOrdinal, CCI_A_TYPE_STR, &value, &ind) < 0)
	{
		m_dwStatus = DBSTATUS_E_UNAVAILABLE;

		return S_OK;
	}

	if(ind == -1)
	{
		// null value
		m_dwStatus = DBSTATUS_S_ISNULL;

		return S_OK;
	}

	while(value[ind - 1] == ' ')
	{
		ind--;
		value[ind] = 0;
	}

	m_strData = _strdup(value);
	m_dwStatus = DBSTATUS_S_OK;
	m_cbDataLen = ind;

	return S_OK;
}

//TODO Investigate the need for additional explicit data types
HRESULT CCUBRIDRowsetRowColumn::WriteData(int hReq, int iRowset, ATLCOLUMNINFO *pInfo)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowsetRowColumn::WriteData\n");

	//Verify data is updateable
	if(!cci_is_updatable(hReq))
	{
		ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowsetRowColumn::WriteData Error: Data is not updateable!\n");
		return E_FAIL;
	}

	T_CCI_ERROR err_buf;
	int rc;

	if(m_dwStatus == DBSTATUS_S_ISNULL)
		rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_STR, NULL, &err_buf);
	//else if(m_dwStatus==DBSTATUS_S_DEFAULT) //Do nothing
	//	rc = 0;
	else
	{
		switch(pInfo->wType)
		{
		case DBTYPE_I1:
		case DBTYPE_I2:
		case DBTYPE_I4:
			{
				int val = atoi(m_strData);
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_INT, &val, &err_buf);
			}
			break;
		case DBTYPE_I8:
			{
				long val = atol(m_strData);
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_BIGINT, &val, &err_buf);
			}
			break;
		case DBTYPE_R4:
		case DBTYPE_R8:
			{
				double val = atof(m_strData);
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_DOUBLE, &val, &err_buf);
			}
			break;
		case DBTYPE_BYTES:
			{
				T_CCI_BIT val;
				val.size = (int)strlen(m_strData) / 2;
				val.buf = (char *)malloc(val.size);
				for(int i = 0; i < val.size; i++)
				{
					int t;
					sscanf(m_strData + i * 2, "%02X", &t);
					val.buf[i] = t;
				}
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_BIT, &val, &err_buf);
				free(val.buf);
			}
			break;
		case DBTYPE_DBDATE:
			{
				T_CCI_DATE val;
				int y, m, d;
				sscanf(m_strData, "%d-%d-%d", &y, &m, &d);
				val.yr = y;
				val.mon = m;
				val.day = d;
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_DATE, &val, &err_buf);
			}
			break;
		case DBTYPE_DBTIME:
			{
				T_CCI_DATE val;
				int hh, mm, ss;
				sscanf(m_strData, "%d:%d:%d", &hh, &mm, &ss);
				val.hh = hh;
				val.mm = mm;
				val.ss = ss;
				val.ms = 0;
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_DATE, &val, &err_buf);
			}
			break;
		case DBTYPE_DBTIMESTAMP:
			{
				T_CCI_DATE val;
				int y, m, d, H, M, S, MS;
				sscanf(m_strData, "%d-%d-%d %d:%d:%d.%03d", &y, &m, &d, &H, &M, &S, &MS);
				val.yr = y;
				val.mon = m;
				val.day = d;
				val.hh = H;
				val.mm = M;
				val.ss = S;
				val.ms = MS;
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_DATE, &val, &err_buf);
			}
			break;
		case DBTYPE_STR:
			{
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_STR, m_strData, &err_buf);
			}
			break;
		default:
			{
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_STR, m_strData, &err_buf);
			}
			break;
		}
	}

	if(rc == 0)
		return S_OK;
	else if(err_buf.err_code == -670)
		return DB_E_INTEGRITYVIOLATION;
	else
	{
		ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowsetRowColumn::WriteData Error:%d\n", rc);
		return E_FAIL;
	}
}

HRESULT CCUBRIDRowsetRowColumn::WriteData(int hReq, int iRowset, ATLCOLUMNINFO *pInfo, CCUBRIDStream* s)
{
	ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowsetRowColumn::WriteData\n");

	//Verify data is updateable
	if(!cci_is_updatable(hReq))
	{
		ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowsetRowColumn::WriteData Error: Data is not updateable!\n");
		return E_FAIL;
	}

	T_CCI_ERROR err_buf;
	int rc;

	if(m_dwStatus == DBSTATUS_S_ISNULL)
		rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_STR, NULL, &err_buf);
	else
	{
		switch(pInfo->wType)
		{
		case DBTYPE_CY:
			{
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_BLOB, (void*)s->GetBlob(), &err_buf);
			}
			break;
		case DBTYPE_VARIANT:
			{
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_CLOB, (void*)s->GetClob(), &err_buf);
			}
			break;
		default:
			{
				rc = cci_cursor_update(hReq, iRowset + 1, (int) pInfo->iOrdinal, CCI_A_TYPE_STR, m_strData, &err_buf);
			}
		}
	}

	if(rc == 0)
		return S_OK;
	else if(err_buf.err_code == -670)
		return DB_E_INTEGRITYVIOLATION;
	else
	{
		ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowsetRowColumn::WriteData Error:%d\n", rc);
		return E_FAIL;
	}
}

void CCUBRIDRowsetRow::FreeData()
{
	delete [] m_rgColumns;
	m_rgColumns = 0;
}

HRESULT CCUBRIDRowsetRow::ReadData(int hReq, bool bOIDOnly, bool bSensitive)
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRow::ReadData(1)\n");

	CCUBRIDRowsetRowColumn *pColumns; // = m_rgColumns
	{
		if(m_rgColumns)
		{
			// refresh(undo) row
			pColumns = m_rgColumns;
		}
		else
		{
			// create row
			pColumns = new CCUBRIDRowsetRowColumn[m_cColumns];
			if(pColumns == NULL)
				return E_OUTOFMEMORY;
			m_rgColumns = pColumns;
		}
	}

	{
		T_CCI_ERROR err_buf;
		int res = cci_cursor(hReq, (int) m_iRowset + 1, CCI_CURSOR_FIRST, &err_buf);

		if(res == CCI_ER_NO_MORE_DATA)
			return DB_S_ENDOFROWSET;
		if(res < 0)
			return E_FAIL;

		if(bSensitive)
			res = cci_fetch_sensitive(hReq, &err_buf);
		else
			res = cci_fetch(hReq, &err_buf);

		if(res == CCI_ER_DELETED_TUPLE)
			return DB_E_DELETEDROW;
		if(res == CCI_ER_NO_MORE_DATA)
			//if(res==CCI_ER_NO_MORE_DATA || res==CAS_ER_NO_MORE_DATA)
				return DB_S_ENDOFROWSET;
		if(res < 0)
		{
			ATLTRACE(atlTraceDBProvider, 2, "CCUBRIDRowsetRow::ReadData(1), cci_fetch error=%d\n", res);
			return E_FAIL;
		}
	}

	cci_get_cur_oid(hReq, m_szOID);
	if(bOIDOnly)
		return S_OK;

	for(DBORDINAL i = 0; i < m_cColumns; i++)
	{
		CCUBRIDRowsetRowColumn *pColCur = &pColumns[i];
		pColCur->ReadData(hReq, (int) m_pInfo[i].iOrdinal, m_pInfo[i].wType);
	}

	return S_OK;
}

HRESULT CCUBRIDRowsetRow::ReadData(ATLBINDINGS *pBinding, void *pData)
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRow::ReadData(2)\n");

	CCUBRIDRowsetRowColumn *pColumns; // = m_rgColumns
	{
		if(m_rgColumns)
		{
			// update row
			pColumns = m_rgColumns;
		}
		else
		{	// insert row
			pColumns = new CCUBRIDRowsetRowColumn[m_cColumns];
			if(pColumns == NULL)
				return E_OUTOFMEMORY;
			m_rgColumns = pColumns;
		}
	}

	bool bFailed = false;
	bool bSucceeded = false;

	for(DBORDINAL i = 0; i < pBinding->cBindings; i++)
	{
		DBBINDING *pBindCur = &pBinding->pBindings[i];

		if(pBindCur->iOrdinal == 0)
		{
			// Bookmark Column
			if(m_pInfo[0].iOrdinal != 0)
				return DB_E_BADORDINAL;
			continue;
		}

		CCUBRIDRowsetRowColumn *pColCur;
		ATLCOLUMNINFO *pInfoCur;
		{
			DBORDINAL j;
			for( j = 0; j < m_cColumns && pBindCur->iOrdinal != m_pInfo[j].iOrdinal; j++);
			if(j == m_cColumns)
			{
				ATLTRACE(atlTraceDBProvider, 0, "Invalid binding\n");
				return DB_E_BADORDINAL;
				continue;
			}
			pColCur = &pColumns[j];
			pInfoCur = &m_pInfo[j];
		}

		DBSTATUS dwSrcStatus = DBSTATUS_S_OK;
		DBSTATUS dwRetStatus = (DBSTATUS) - 1;

		if(pBindCur->dwPart & DBPART_STATUS)
			dwSrcStatus = *(DBSTATUS *)((BYTE *)pData + pBindCur->obStatus);

		if(pBindCur->dwPart & DBPART_VALUE)
		{
			BYTE *pSrcData = (BYTE *)pData + pBindCur->obValue;
			if( (pBindCur->dwPart & DBPART_LENGTH) || (pInfoCur->wType != DBTYPE_CY && pInfoCur->wType != DBTYPE_VARIANT))
			{
				DBLENGTH cbLength;
				if(pBindCur->dwPart & DBPART_LENGTH)
					cbLength = *(DBLENGTH *)((BYTE *)pData + pBindCur->obLength);
				/*
				{
				if (pInfoCur->wType==DBTYPE_STR || pInfoCur->wType==DBTYPE_WSTR)
				cbLength = *(DBLENGTH *)((BYTE *)pData+pBindCur->obLength);
				else
				cbLength = 0;
				}
				*/
				else if(pInfoCur->wType == DBTYPE_STR)
					cbLength = (DBLENGTH)strlen((const char *)pSrcData);
				else if(pInfoCur->wType == DBTYPE_WSTR)
					cbLength = (DBLENGTH)wcslen((const wchar_t *)pSrcData);
				else
					cbLength = 0; // fixed-length column?

				if(dwSrcStatus == DBSTATUS_S_ISNULL)
				{
					pColCur->m_dwStatus = DBSTATUS_S_ISNULL;
					bSucceeded = true;
				}
				else if(dwSrcStatus == DBSTATUS_S_DEFAULT)
				{
					if (m_defaultVal && m_defaultVal->GetAt(i).GetLength() > 0)
					{
						pColCur->m_strData = _strdup((char *)m_defaultVal->GetAt(i).GetBuffer());
						pColCur->m_cbDataLen = (DBLENGTH)strlen(pColCur->m_strData);
						pColCur->m_dwStatus = DBSTATUS_S_DEFAULT;
						dwRetStatus = DBSTATUS_S_DEFAULT;
						bSucceeded = true;
					} else
					{
						pColCur->m_dwStatus = DBSTATUS_E_UNAVAILABLE;
						dwRetStatus = DBSTATUS_E_UNAVAILABLE;
						bSucceeded = true;
					}
				}
				else if(dwSrcStatus == DBSTATUS_S_OK)
				{
					HRESULT hr = pColCur->ReadData(m_spConvert, pBindCur->wType, pSrcData, cbLength, pInfoCur);
					if(FAILED(hr))
					{
						bFailed = true;
						if(hr == DB_E_CANTCONVERTVALUE)
							dwRetStatus = DBSTATUS_E_CANTCONVERTVALUE;
						else
							dwRetStatus = DBSTATUS_E_UNAVAILABLE;
					}
					else
						bSucceeded = true;
				}
				else if(dwSrcStatus == DBSTATUS_S_IGNORE)
				{
					bSucceeded = true;
				}
				else
				{
					bFailed = true;
					dwRetStatus = DBSTATUS_E_BADSTATUS;
				}
			}
			else
			{
				if(pInfoCur->wType == DBTYPE_CY || pInfoCur->wType == DBTYPE_VARIANT)
					bSucceeded = true;
				else
				{
					bFailed = true;
					dwRetStatus = DBSTATUS_E_UNAVAILABLE;
				}
			}
		}
		else
		{
			if(pBindCur->dwPart & DBPART_LENGTH)
			{
				if(pInfoCur->dwFlags & DBCOLUMNFLAGS_ISLONG)
				{
					bSucceeded = true;
				}
				else
				{
					bFailed = true;
					dwRetStatus = DBSTATUS_E_UNAVAILABLE;
				}
			}
			else
			{
				if(dwSrcStatus == DBSTATUS_S_ISNULL)
				{
					pColCur->m_dwStatus = DBSTATUS_S_ISNULL;
					bSucceeded = true;
				}
				else if(dwSrcStatus == DBSTATUS_S_DEFAULT)
				{
					if (m_defaultVal && m_defaultVal->GetAt(i).GetLength() > 0)
					{
						pColCur->m_strData = _strdup((char *)m_defaultVal->GetAt(i).GetBuffer());
						pColCur->m_cbDataLen = (DBLENGTH)strlen(pColCur->m_strData);
						pColCur->m_dwStatus = DBSTATUS_S_OK;
						dwRetStatus = DBSTATUS_S_DEFAULT;
						bSucceeded = true;
					} else
					{
						dwRetStatus = DBSTATUS_E_UNAVAILABLE;
						bFailed = true;
					}
				}
				else if(dwSrcStatus == DBSTATUS_S_OK)
				{
					bFailed = true;
					dwRetStatus = DBSTATUS_E_UNAVAILABLE;
				}
				else if(dwSrcStatus == DBSTATUS_S_IGNORE)
				{
					bSucceeded = true;
				}
				else
				{
					bFailed = true;
					dwRetStatus = DBSTATUS_E_BADSTATUS;
				}
			}
		}

		if( (pBindCur->dwPart & DBPART_STATUS) && dwRetStatus != (DBSTATUS) - 1 )
			*(DBSTATUS *)((BYTE *)pData + pBindCur->obStatus) = dwRetStatus;
	}

	if(bFailed)
	{
		// partially succeeded?
		return bSucceeded ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED;
	}
	else
	{
		return S_OK;
	}
}

HRESULT CCUBRIDRowsetRow::ReadData(int hReq, char* szOID)
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRow::ReadData(3)\n");
	int res, ind;
	T_CCI_SET* set;
	char* strVal;

	CCUBRIDRowsetRowColumn *pColumns; // = m_rgColumns
	{
		if(m_rgColumns)
		{
			// refresh(undo) row
			pColumns = m_rgColumns;
		}
		else
		{
			// create row
			pColumns = new CCUBRIDRowsetRowColumn[1];
			if(pColumns == NULL)
				return E_OUTOFMEMORY;
			m_rgColumns = pColumns;
		}
	}

	//OID Copy
	strcpy(m_szOID, szOID);

	res = cci_get_data(hReq, 1, CCI_A_TYPE_SET, &set, &ind);
	if(res < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IRow), "CCUBRIDRowsetRow::ReadData(3) cci_get_data failed");

	CCUBRIDRowsetRowColumn *pColCur = &pColumns[0];

	if (cci_set_get(set, 1, CCI_A_TYPE_STR, &strVal, &ind) < 0)
		return RaiseError(E_FAIL, 0, __uuidof(IRow), "CCUBRIDRowsetRow::ReadData(3) cci_set_data failed");

	pColCur->m_strData = _strdup(strVal);
	pColCur->m_dwStatus = DBSTATUS_S_OK;
	pColCur->m_cbDataLen = ind;

	return S_OK;
}

static CComBSTR BuildColumnValue(ATLCOLUMNINFO *m_pInfoCur, CCUBRIDRowsetRowColumn *pColCur)
{
	if(pColCur->m_dwStatus == DBSTATUS_S_ISNULL)
		return "NULL";

	CComBSTR str;
	CString temp(pColCur->m_strData);
	switch(m_pInfoCur->wType)
	{
	case DBTYPE_I2:
	case DBTYPE_I4:
	case DBTYPE_R4:
	case DBTYPE_R8:
	case DBTYPE_NUMERIC:
		str.Append(pColCur->m_strData);
		break;
	case DBTYPE_WSTR: // NCHAR?
		str.Append("N'");

		temp.Replace("'", "''");
		str.Append(temp);

		str.Append("'");
		break;
	case DBTYPE_BYTES:
		str.Append("B'");
		str.Append(pColCur->m_strData);
		str.Append("'");
		break;
	case DBTYPE_DBDATE:
		str.Append("DATE'");
		str.Append(pColCur->m_strData);
		str.Append("'");
		break;
	case DBTYPE_DBTIME:
		str.Append("TIME'");
		str.Append(pColCur->m_strData);
		str.Append("'");
		break;
	case DBTYPE_DBTIMESTAMP:
		if(m_pInfoCur->bPrecision == 23)
			str.Append("DATETIME'");
		else
			str.Append("TIMESTAMP'");
		str.Append(pColCur->m_strData);
		str.Append("'");
		break;
	default:
		str.Append("'");

		temp.Replace("'", "''");
		str.Append(temp);

		str.Append("'");

		break;
	}

	return str;
}

HRESULT CCUBRIDRowsetRow::WriteData(int hConn, int hReq, CComBSTR &strTableName, ATLBINDINGS *pBinding, void *pData)
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRow::WriteData(1)\n");

	HRESULT hr = S_OK;

	CComBSTR query;
	{
		switch(m_status)
		{
		case DBPENDINGSTATUS_DELETED:
			{
				CComBSTR column;
				for(DBORDINAL i = 0; i < m_cColumns; i++)
				{
					ATLCOLUMNINFO *m_pInfoCur = &m_pInfo[i];
					if(m_pInfoCur->iOrdinal == 0)
						continue; // skip bookmark column
					column.Append("[");
					column.Append(m_pInfoCur->pwszName);
					column.Append("]");
					column.Append(" = ");
					CComBSTR value = BuildColumnValue(m_pInfoCur, m_rgColumns + i);
					column.Append(value);
					if(i != m_cColumns - 1)
					{
						column.Append(" AND ");
					}
				}

				// delete from <table> where <table>=?
				query.Append("delete from ");
				query.Append("[");
				query.Append(strTableName);
				query.Append("]");
				query.Append(" where ");
				query.Append(column);
			}
			break;
		case DBPENDINGSTATUS_NEW:
			// insert into <table>(<column>,<column>) values(<value>,<value>)
			{
				CComBSTR column;
				CComBSTR value;
				for(DBORDINAL i = 0; i < m_cColumns; i++)
				{
					ATLCOLUMNINFO *m_pInfoCur = &m_pInfo[i];
					if(m_pInfoCur->iOrdinal == 0)
						continue; // skip bookmark column
					column.Append("[");
					column.Append(m_pInfoCur->pwszName);
					column.Append("]");
					if( m_pInfoCur->wType == DBTYPE_CY || m_pInfoCur->wType == DBTYPE_VARIANT && pBinding != NULL && pData != NULL)
					{
						value.Append("?");
					}
					else
						value.Append(BuildColumnValue(m_pInfoCur, m_rgColumns + i));
					if(i != m_cColumns - 1)
					{
						column.Append(",");
						value.Append(",");
					}
				}

				query.Append("insert into ");
				query.Append("[");
				query.Append(strTableName);
				query.Append("]");
				query.Append("(");
				query.Append(column);
				query.Append(") values(");
				query.Append(value);
				query.Append(")");
			}
			break;
		case DBPENDINGSTATUS_CHANGED:
			{
				for(DBORDINAL i = 0; i < m_cColumns; i++)
				{
					ATLCOLUMNINFO *pInfoCur = &m_pInfo[i];
					if(pInfoCur->iOrdinal == 0)
						continue; // skip bookmark column

					CCUBRIDRowsetRowColumn *pCol = m_rgColumns + i;
					if(pInfoCur->wType == DBTYPE_CY || pInfoCur->wType == DBTYPE_VARIANT)
					{
						void* temp = (void*)((BYTE *)pData + pBinding->pBindings[i - 1].cbMaxLen);
						IUnknown* pUnknown = *(IUnknown**)temp;
						CComContainedObject<CCUBRIDStream>* Stream;
						hr = pUnknown->QueryInterface(__uuidof(ISequentialStream), (void**)&Stream);
						hr = pCol->WriteData(hReq, (int) m_iRowset, pInfoCur, Stream);
					}
					else
					{
						HRESULT hr = pCol->WriteData(hReq, (int) m_iRowset, pInfoCur);
					}
					if(FAILED(hr))
						return hr;
				}
				return S_OK;
			}

			// update <table> set <column>=<value>, <column>=<value> where <table>=?
			//{
			//	CComBSTR set;
			//	for(DBORDINAL i=0;i<m_cColumns;i++)
			//	{
			//		ATLCOLUMNINFO *m_pInfoCur = &m_pInfo[i];
			//		if(m_pInfoCur->iOrdinal==0)
			//			continue; // skip bookmark column
			//		set.Append(m_pInfoCur->pwszName);
			//		set.Append("=");
			//		set.Append(BuildColumnValue(m_pInfoCur, m_rgColumns+i));
			//		if(i != m_cColumns-1)
			//			set.Append(",");
			//	}

			//	query.Append("update ");
			//	query.Append("[");
			//	query.Append(strTableName);
			//	query.Append("]");
			//	query.Append(" set ");
			//	query.Append(set);
			//	query.Append(" where ");
			//	query.Append("[");
			//	query.Append(strTableName);
			//	query.Append("]");
			//	query.Append("=?");
			//}

			break;
		default:
			return DB_E_ERRORSOCCURRED;
		}
	}

	//#ifdef _DEBUG
	//	{
	//		ATLTRACE(atlTraceDBProvider, 2, "Execute Query = ");
	//		CW2A local(query);
	//		for(size_t i=0;i<strlen(local);)
	//		{
	//			char buf[513]; memcpy(buf, local+i, 512); buf[512] = 0;
	//			ATLTRACE(atlTraceDBProvider, 2, "%s", buf);
	//			i += 512;
	//		}
	//		ATLTRACE(atlTraceDBProvider, 2, "\n");
	//	}
	//#endif

	// prepare query
	int hLocalReq;
	{
		T_CCI_ERROR err_buf;
		hLocalReq = cci_prepare(hConn, CW2A(query), 0, &err_buf);
		if (hLocalReq < 0)
			return RaiseError(DB_E_ERRORSOCCURRED, 1, __uuidof(IRowset), err_buf.err_msg);
	}

	// execute query
	{
		if(pBinding != NULL && pData != NULL)
		{
			for(DBORDINAL i = 0; i < m_cColumns; i++)
			{
				if(pBinding->pBindings[i].wType == DBTYPE_IUNKNOWN)
				{
					void* temp = (void*)((BYTE *)pData + pBinding->pBindings[i].cbMaxLen);
					IUnknown* pUnknown = *(IUnknown**)temp;
					CComContainedObject<CCUBRIDStream>* Stream;
					hr = pUnknown->QueryInterface(__uuidof(ISequentialStream), (void**)&Stream);
					if(SUCCEEDED(hr))
					{
						switch(Stream->GetType())
						{
						case CCI_U_TYPE_BLOB:
							hr = cci_bind_param(hLocalReq, (int)i, CCI_A_TYPE_BLOB, (void *)Stream->GetBlob(), CCI_U_TYPE_BLOB, CCI_BIND_PTR);
							break;
						case CCI_U_TYPE_CLOB:
							hr = cci_bind_param(hLocalReq, (int)i, CCI_A_TYPE_CLOB, (void *)Stream->GetClob(), CCI_U_TYPE_CLOB, CCI_BIND_PTR);
							break;
						}
					}
				}
			}
		}
		if(m_status == DBPENDINGSTATUS_DELETED || m_status == DBPENDINGSTATUS_CHANGED)
			cci_bind_param(hLocalReq, 1, CCI_A_TYPE_STR, m_szOID, CCI_U_TYPE_OBJECT, 0);

		T_CCI_ERROR err_buf;
		int rc = cci_execute(hLocalReq, 0, 0, &err_buf);
		if(rc < 0)
		{
			cci_close_req_handle(hLocalReq);
			if(err_buf.err_code == -670) // constraint violation code
				return RaiseError(DB_E_INTEGRITYVIOLATION, 0, __uuidof(IRowset));
			else
				return RaiseError(DB_E_ERRORSOCCURRED, 1, __uuidof(IRowset), err_buf.err_msg);
		}
		else if(rc == 0)
		{
			if(m_status == DBPENDINGSTATUS_DELETED)
				hr = DB_E_DELETEDROW;
		}
		else
		{
			ATLASSERT(rc == 1);
		}
	}

	cci_close_req_handle(hLocalReq);

	return hr;
}

HRESULT CCUBRIDRowsetRow::WriteData(ATLBINDINGS *pBinding, void *pData, DBROWCOUNT dwBookmark, CCUBRIDRowset* pRowset)
{
	ATLTRACE(atlTraceDBProvider, 3, "CCUBRIDRowsetRow::WriteData(2)\n");

	bool bFailed = false;
	bool bSucceeded = false;

	for(DBCOUNTITEM i = 0; i < pBinding->cBindings; i++)
	{
		DBBINDING *pBindCur = &(pBinding->pBindings[i]);

		if(pBindCur->iOrdinal == 0)
		{
			// Bookmark Column
			DBSTATUS dbStat = DBSTATUS_S_OK;
			DBLENGTH cbDst = 4;
			if(pBindCur->dwPart & DBPART_VALUE)
			{
				// deferred validation
				DBLENGTH cbMaxLen = pBindCur->cbMaxLen;
				if(pBindCur->dwPart & DBPART_STATUS)
				{
					DBLENGTH cbLen = pBindCur->obStatus - pBindCur->obValue;
					if(cbLen < cbMaxLen)
						cbMaxLen = cbLen;
				}
				if(pBindCur->dwPart & DBPART_LENGTH)
				{
					DBLENGTH cbLen = pBindCur->obLength - pBindCur->obValue;
					if(cbLen < cbMaxLen)
						cbMaxLen = cbLen;
				}

				BYTE *pDstTemp = (BYTE *)pData + pBindCur->obValue;
				HRESULT hr = m_spConvert->DataConvert(DBTYPE_I4, pBindCur->wType, 4, &cbDst,
					&dwBookmark, pDstTemp, pBindCur->cbMaxLen, dbStat,
					&dbStat, pBindCur->bPrecision, pBindCur->bScale, 0);
				if (FAILED(hr)) return hr;
			}
			if(pBindCur->dwPart & DBPART_STATUS)
				*(DBSTATUS *)((BYTE *)pData + pBindCur->obStatus) = dbStat;
			if(pBindCur->dwPart & DBPART_LENGTH)
				*(DBLENGTH *)((BYTE *)pData + pBindCur->obLength) = ( dbStat == DBSTATUS_S_ISNULL ? 0 : cbDst );
			continue;
		}

		DBORDINAL cNCols;
		ATLCOLUMNINFO *pColumnInfo = pRowset->GetColumnInfo(pRowset, &cNCols);
		// Non-Bookmark Columns
		if (pBindCur->pObject || pColumnInfo[pBindCur->iOrdinal - 1].wType == DBTYPE_CY || pColumnInfo[pBindCur->iOrdinal - 1].wType == DBTYPE_VARIANT)
		{
			//Storage interface
			//if (pBindCur->pObject->iid != IID_ISequentialStream)
			//return E_NOINTERFACE;

			CComPolyObject<CCUBRIDStream>* pObjStream;

			HRESULT hr = CComPolyObject<CCUBRIDStream>::CreateInstance(NULL, &pObjStream);
			if (FAILED(hr))
				return hr;

			CComPtr<IUnknown> spUnk;
			hr = pObjStream->QueryInterface(&spUnk);
			if(FAILED(hr))
			{
				delete pObjStream;
				return hr;
			}

			int hConn;
			hr = pRowset->GetSessionPtr()->GetConnectionHandle(&hConn);
			if (FAILED(hr))
				return hr;

			DBORDINAL cCols;
			ATLCOLUMNINFO *pInfo = pRowset->GetColumnInfo(pRowset, &cCols);

			pObjStream->m_contained.Initialize(hConn, this->m_szOID, &pInfo[pBindCur->iOrdinal - 1]);

			BYTE* temp = (BYTE *)pData + pBindCur->obValue;
			hr = pObjStream->QueryInterface(IID_ISequentialStream, (void **)temp);
			if (FAILED(hr))
				return E_NOINTERFACE;

			continue;
		}

		ATLCOLUMNINFO *m_pInfoCur;
		CCUBRIDRowsetRowColumn *pColCur;
		{
			// m_pInfo[i].iOrdinal = i+1 if there is no self bookmark
			// m_pInfo[i].iOrdinal = i if there is a self bookmark
			if(m_pInfo[0].iOrdinal == 0)
			{
				m_pInfoCur = &(m_pInfo[pBindCur->iOrdinal]);
				pColCur = m_rgColumns + pBindCur->iOrdinal;
			}
			else
			{
				m_pInfoCur = &(m_pInfo[pBindCur->iOrdinal - 1]);
				pColCur = m_rgColumns + pBindCur->iOrdinal - 1;
			}
		}

		if(pColCur->m_dwStatus == DBSTATUS_S_OK || pColCur->m_dwStatus == DBSTATUS_S_DEFAULT)
		{
			BYTE *pDstTemp = NULL;
			if(pBindCur->dwPart & DBPART_VALUE)
			{
				pDstTemp = (BYTE *)pData + pBindCur->obValue;
				//memset(pDstTemp, 0, pBindCur->cbMaxLen);
			}
			DBSTATUS dbStatus;
			DBLENGTH cbDataLen;
			/*if(m_pInfoCur->wType==DBTYPE_STR && pBindCur->wType==DBTYPE_STR && (m_pInfoCur->dwFlags & DBCOLUMNFLAGS_ISFIXEDLENGTH))
			{
			pBindCur->cbMaxLen = pBindCur->bPrecision + 1;
			pBindCur->obLength = pBindCur->cbMaxLen + 1;
			pBindCur->obStatus = pBindCur->obLength + 4;
			}*/
			HRESULT hr = pColCur->TransferData(m_spConvert, (int) m_pInfoCur->iOrdinal, pBindCur->wType,
				pBindCur->bPrecision, pBindCur->bScale, pDstTemp,
				pBindCur->cbMaxLen, &dbStatus, &cbDataLen);
			if (FAILED(hr))
				return hr;

			if( (m_pInfoCur->wType == DBTYPE_STR || m_pInfoCur->wType == DBTYPE_WSTR)
				&& (m_pInfoCur->dwFlags & DBCOLUMNFLAGS_ISFIXEDLENGTH)
				&& (pBindCur->wType == DBTYPE_STR || pBindCur->wType == DBTYPE_WSTR) )
			{
				//DBLENGTH maxsize = m_pInfoCur->bPrecision;
				DBLENGTH maxsize = m_pInfoCur->ulColumnSize;
				if(pBindCur->wType == DBTYPE_WSTR)
					maxsize *= 2;
				while(cbDataLen < pBindCur->cbMaxLen && cbDataLen < maxsize)
				{
					if(pBindCur->wType == DBTYPE_STR)
					{
						pDstTemp[cbDataLen] = ' ';
						cbDataLen++;
					}
					else
					{
						*(wchar_t *)(pDstTemp + cbDataLen) = L' ';
						cbDataLen += 2;
					}
				}
				if(pBindCur->wType == DBTYPE_STR)
					pDstTemp[cbDataLen] = '\0';
				else
					*(wchar_t *)(pDstTemp + cbDataLen) = L'\0';
			}

			if(pBindCur->dwPart & DBPART_STATUS)
				*(DBSTATUS *)((BYTE *)pData + pBindCur->obStatus) = dbStatus;
			if(pBindCur->dwPart & DBPART_LENGTH)
				*(DBLENGTH *)((BYTE *)pData + pBindCur->obLength) = cbDataLen;
			if(FAILED(hr))
				bFailed = true;
			else
				bSucceeded = true;
		}
		else
		{
			if(pBindCur->dwPart & DBPART_STATUS)
				*(DBSTATUS *)((BYTE *)pData + pBindCur->obStatus) = pColCur->m_dwStatus;
			bSucceeded = true;
		}
	}

	if(bFailed)
	{
		// partially succeeded?
		return bSucceeded ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED;
	}
	else
	{
		return S_OK;
	}
}

HRESULT CCUBRIDRowsetRow::WriteData(DBORDINAL cColumns, DBCOLUMNACCESS rgColumns[])
{
	CDBIDOps op;
	bool bFailed = false;
	bool bSucceeded = false;

	for(DBCOUNTITEM i = 0; i < cColumns; i++)
	{
		DBCOLUMNACCESS *pAccessCur = &(rgColumns[i]);
		DBORDINAL j;
		for(j = 0; j < m_cColumns; j++)
		{
			if(op.CompareDBIDs(&pAccessCur->columnid, &m_pInfo[j].columnid) == S_OK)
				break;
		}
		if(j == m_cColumns)
		{
			bFailed = true;
			pAccessCur->dwStatus = DBSTATUS_E_DOESNOTEXIST;
			continue;
		}
		ATLCOLUMNINFO *m_pInfoCur = &m_pInfo[j];
		CCUBRIDRowsetRowColumn *pColCur = m_rgColumns + j;
#ifdef FIXME
		//memset(pAccessCur->pData, 0, pAccessCur->cbMaxLen);
#endif

		HRESULT hr = pColCur->TransferData(m_spConvert, (int) m_pInfoCur->iOrdinal, pAccessCur->wType,
			pAccessCur->bPrecision, pAccessCur->bScale, pAccessCur->pData,
			pAccessCur->cbMaxLen, &pAccessCur->dwStatus, &pAccessCur->cbDataLen);
		DBLENGTH &cbDataLen = pAccessCur->cbDataLen;
		if( (m_pInfoCur->wType == DBTYPE_STR || m_pInfoCur->wType == DBTYPE_WSTR)
			&& (m_pInfoCur->dwFlags & DBCOLUMNFLAGS_ISFIXEDLENGTH)
			&& (pAccessCur->wType == DBTYPE_STR || pAccessCur->wType == DBTYPE_WSTR) )
		{
			DBLENGTH maxsize = m_pInfoCur->ulColumnSize;
			if(pAccessCur->wType == DBTYPE_WSTR) maxsize *= 2;
			while(cbDataLen < pAccessCur->cbMaxLen && cbDataLen < maxsize)
			{
				if(pAccessCur->wType == DBTYPE_STR)
				{
					*((char *)pAccessCur->pData + cbDataLen) = ' ';
					cbDataLen++;
				}
				else
				{
					*(wchar_t *)((char *)pAccessCur->pData + cbDataLen) = L' ';
					cbDataLen += 2;
				}
			}
			if(pAccessCur->wType == DBTYPE_STR)
				*((char *)pAccessCur->pData + cbDataLen) = '\0';
			else
				*(wchar_t *)((char *)pAccessCur->pData + cbDataLen) = L' ';
		}
		if(FAILED(hr))
			bFailed = true;
		else
			bSucceeded = true;
	}

	if(bFailed)
	{
		// partially succeeded?
		return bSucceeded ? DB_S_ERRORSOCCURRED : DB_E_ERRORSOCCURRED;
	}
	else
	{
		return S_OK;
	}
}

HRESULT CCUBRIDRowsetRow::Compare(void *pFindData, DBCOMPAREOP CompareOp, DBBINDING &rBinding)
{
	if(CompareOp == DBCOMPAREOPS_IGNORE)
		return S_OK;

	DBORDINAL iOrdinal = rBinding.iOrdinal - 1;
	{
		if(m_pInfo[0].iOrdinal == 0) iOrdinal++;
	}

	const char *szRowValue = NULL;
	{
		CCUBRIDRowsetRowColumn *pRowColumn = m_rgColumns + iOrdinal;
		szRowValue = pRowColumn->m_strData;
		if(pRowColumn->m_dwStatus == DBSTATUS_S_ISNULL)
			szRowValue = NULL;
	}

	void *pValue = NULL;
	if(pFindData)
	{
		if(rBinding.dwPart & DBPART_VALUE)
			pValue = (char *)pFindData + rBinding.obValue;

		if(rBinding.dwPart & DBPART_STATUS)
		{
			DBSTATUS dbStat = *(DBSTATUS *)((BYTE *)pFindData + rBinding.obStatus);
			if(dbStat == DBSTATUS_S_ISNULL)
				pValue = NULL;
		}
	}

	if(szRowValue == NULL || pValue == NULL)
	{
		if((const void *)szRowValue == pValue)
			return S_OK;
		else
			return S_FALSE;
	}

	DBLENGTH cbSrcLen;
	if(rBinding.dwPart & DBPART_LENGTH)
		cbSrcLen = *(DBLENGTH *)((BYTE *)pFindData + rBinding.obLength);
	else if(rBinding.wType == DBTYPE_STR)
		cbSrcLen = (DBLENGTH)strlen((const char *)pValue);
	else if(rBinding.wType == DBTYPE_WSTR)
		cbSrcLen = (DBLENGTH)wcslen((const wchar_t *)pValue);
	else
		cbSrcLen = 0; // fixed-length column?
	DBLENGTH cbDstMaxLen = (DBLENGTH)strlen(szRowValue) + 5;
	DBSTATUS dbStat = DBSTATUS_S_OK;

	char *szFindValue = new char[cbDstMaxLen + 1];
	m_spConvert->DataConvert(rBinding.wType, DBTYPE_STR, cbSrcLen, &cbSrcLen,
		pValue, szFindValue, cbDstMaxLen, dbStat, &dbStat,
		rBinding.bPrecision, rBinding.bScale, 0);

	ATLTRACE(atlTraceDBProvider, 3, "Find Value with op=%d", CompareOp);
	HRESULT hr = ::Type::Compare(CompareOp, m_pInfo[iOrdinal].wType, szRowValue, szFindValue);
	ATLTRACE(atlTraceDBProvider, 3, "\n");

	delete [] szFindValue;

	return hr;
}
