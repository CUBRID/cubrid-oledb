/*!
 * $RCSfile: dynamicaccessorex.h,v $
 * \file oledb/dynamicaccessorex.h
 *
 * \brief Defines a dynamic accessor class for use with OLE DB.
 *
 * \author J&ouml;rgen Sigvardsson &lt;jorgen@profitab.com&gt;
 * \version $Revision: 1.17 $
 */

#pragma once

#include "ATLComTime.h"

#define _CRT_SECURE_NO_WARNINGS

namespace oledb {

	using namespace ATL;

	/*!
	 * \brief Class which extends <tt>ATL::CDynamicAccessor</tt> with typing.
	 *
	 * \author J&ouml;rgen Sigvardsson \<jorgen@profitab.com\>
	 */
	class CDynamicAccessorEx : public ATL::CDynamicAccessor {
	public:
		/*!
		 * \brief Tests if a column is NULL or not.
		 *
		 * \param lpszColumnName name of column (narrow string)
		 * \return <tt>true</tt> if NULL
		 */
		bool IsNull(const CHAR* lpszColumnName) {
			DBSTATUS st;
            ATLVERIFY(GetStatus(lpszColumnName, &st));
			return st == DBSTATUS_S_ISNULL;
		}

		/*!
		 * \brief Tests if a column is NULL or not.
		 *
		 * \param lpszColumnName name of column (wide string)
		 * \return <tt>true</tt> if NULL
		 */
		bool IsNull(const WCHAR* lpszColumnName) {
			DBSTATUS st;
            ATLVERIFY(GetStatus(lpszColumnName, &st));
			return st == DBSTATUS_S_ISNULL;
		}

		/*!
		 * \brief Tests if a column is NULL or not.
		 *
		 * \param idx column index
		 * \return <tt>true</tt> if NULL
		 */
		bool IsNull(DBORDINAL idx) {
			DBSTATUS st;
            ATLVERIFY(GetStatus(idx, &st));
			return st == DBSTATUS_S_ISNULL;
		}

		/*!
		 * \brief Sets a column to NULL.
		 *
		 * \param idx column index
		 */
		void SetNull(DBORDINAL idx) {
			ATLVERIFY(SetStatus(idx, DBSTATUS_S_ISNULL));
		}

		/*!
		 * \brief Sets a column to NULL.
		 *
		 * \param lpszColumnName column name (narrow string)
		 */
		void SetNull(const CHAR* lpszColumnName) {
			ATLVERIFY(SetStatus(lpszColumnName, DBSTATUS_S_ISNULL));
		}

		/*!
		 * \brief Sets a column to NULL.
		 *
		 * \param lpszColumnName column name (wide string)
		 */
		void SetNull(const WCHAR* lpszColumnName) {
			ATLVERIFY(SetStatus(lpszColumnName, DBSTATUS_S_ISNULL));
		}

		/*!
		 * \brief Generic version for acquiring data from result.
		 *
		 * Simply copies the data in named column to storage pointed to by <tt>pData</tt>.
		 *
		 * \param lpszColumnName name of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <typename T>
		bool GetValue(const CHAR* lpszColumnName, T* pData) {
			DBORDINAL  idx;
			if(!GetOrdinal(lpszColumnName, &idx))
				return false;

			return GetValue(idx, pData);
		}

		/*!
		 * \brief Generic version for acquiring data from result.
		 *
		 * Simply copies the data in named column to storage pointed to by <tt>pData</tt>.
		 *
		 * \param lpszColumnName name of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <typename T>
		bool GetValue(const WCHAR* lpszColumnName, T* pData) {
			DBORDINAL  idx;
			if(!GetOrdinal(lpszColumnName, &idx))
				return false;

			return GetValue(idx, pData);
		}

		/*!
		 * \brief Generic version for acquiring data from result.
		 *
		 * \param idx ordinal of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <typename T>
		bool GetValue(DBORDINAL idx, T* pData) {
			return CDynamicAccessor::GetValue(idx, pData);
		}

		/*!
		 * \brief <tt>char</tt> version for acquiring data from result.
		 *
		 * \note This function will fetch an 8 bit integer, not a string!
		 *
		 * \param idx ordinal of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<char>(DBORDINAL idx, char* pData) {
			return CastIntegerValue(idx, pData);
		}

		/*!
		 * \brief <tt>short</tt> version for acquiring data from result.
		 *
		 * \param idx ordinal of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<short>(DBORDINAL idx, short* pData) {
			return CastIntegerValue(idx, pData);
		}

		/*!
		 * \brief <tt>int</tt> version for acquiring data from result.
		 *
		 * \param idx ordinal of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<int>(DBORDINAL idx, int* pData) {
			return CastIntegerValue(idx, pData);
		}

		/*!
		 * \brief <tt>long</tt> version for acquiring data from result.
		 *
		 * \param idx ordinal of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<long>(DBORDINAL idx, long* pData) {
			return CastIntegerValue(idx, pData);
		}

		/*!
		 * \brief <tt>unsigned char</tt> version for acquiring data from result.
		 *
		 * \param idx ordinal of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<unsigned char>(DBORDINAL idx, unsigned char* pData) {
			return CastIntegerValue(idx, pData);
		}

		/*!
		 * \brief <tt>unsigned short</tt> version for acquiring data from result.
		 *
		 * \param idx ordinal of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<unsigned short>(DBORDINAL idx, unsigned short* pData) {
			return CastIntegerValue(idx, pData);
		}

		/*!
		 * \brief <tt>unsigned int</tt> version for acquiring data from result.
		 *
		 * \param idx ordinal of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<unsigned int>(DBORDINAL idx, unsigned int* pData) {
			return CastIntegerValue(idx, pData);
		}

		/*!
		 * \brief <tt>unsigned long</tt> version for acquiring data from result.
		 *
		 * \param idx ordinal of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<unsigned long>(DBORDINAL idx, unsigned long* pData) {
			return CastIntegerValue(idx, pData);
		}

		/*!
		 * \brief <tt>bool</tt> version for acquiring data from result.
		 *
		 * \param idx ordinal of column
		 * \param pData pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<bool>(DBORDINAL idx, bool* pData) {
			DBTYPE type;

			if(!GetColumnType(idx, &type))
				return false;

			if(type == DBTYPE_BOOL) {
				*pData = VARIANT_FALSE != *(VARIANT_BOOL*)CDynamicAccessor::GetValue(idx);
			} else {
				long nValue;
				if(!GetValue(idx, &nValue)) // Get as integer type
					return false;
				*pData = (nValue != 0);    // C-style implicit bool (0 = false, !0 = true)
			}
			return true;
		}

		/*!
		 * \brief <tt>wchar_t*</tt> version for acquiring data from result.
		 *
		 * \note This function will fetch a wide string, not a 16 bit value!
		 *
		 * \param idx ordinal of column
		 * \param str pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<wchar_t*>(DBORDINAL idx, wchar_t** str) {
			*str = 0;
			return GetWideString(idx, str);
		}

		/*!
		 * \brief <tt>char*</tt> version for acquiring data from result.
		 *
		 * \note This function will fetch a narrow string, not a 8 bit value!
		 *
		 * \param idx ordinal of column
		 * \param str pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<char*>(DBORDINAL idx, char** str) {
			*str = 0;
			return GetNarrowString(idx, str);
		}

		/*!
		 * \brief <tt>CString</tt> version for acquiring data from result.
		 *
		 * \param idx ordinal of column
		 * \param str pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<CString>(DBORDINAL idx, CString* str) {
			DBLENGTH     byte_len;
			DBTYPE       type;

			if(!GetLength(idx, &byte_len))
				return false;

			if(!GetColumnType(idx, &type))
				return false;

			if(DBTYPE_STR == type || DBTYPE_WSTR == type || DBTYPE_BSTR == type) {
				DBLENGTH   db_char_len;

				db_char_len = (DBTYPE_STR == type ? byte_len : byte_len / 2);
				CString::PXSTR lpszBuffer = str->GetBuffer(db_char_len + 1);

				// Note that reinterpret_cast<> is needed because of runtime/compile time differences.
				if(sizeof(CString::XCHAR) == sizeof(char))
					GetNarrowString(idx, reinterpret_cast<char**>(&lpszBuffer), db_char_len + 1);
				else if(sizeof(CString::XCHAR) == sizeof(wchar_t))
					GetWideString(idx, reinterpret_cast<wchar_t**>(&lpszBuffer), db_char_len + 1);
				else {
					ATLASSERT(false); // WTF is XCHAR anyway?
					str->ReleaseBuffer();
					str->SetString(_T(""));
					return false;
				}
				str->ReleaseBuffer();
			} else {
				CString::PXSTR lpszValue;
				if(!GetValue(idx, &lpszValue))
					return false;
				*str = lpszValue;
				delete [] lpszValue;
			}

			return true;
		}

		// Specialization for CTime (iff CTime is a known identifier...)
#ifndef DOXYGEN_SKIP_THIS
		__if_exists(CTime) {
#endif
			/*!
			 * \brief <tt>CTime</tt> version for acquiring data from result.
			 *
			 * \note This function will only be visible if <tt>CTime</tt> is available
			 * in the global scope at the point of inclusion.
			 *
			 * \param idx ordinal of column
			 * \param time pointer to receiving storage
			 * \return <tt>true</tt> on success
			 */
			template <>
			bool GetValue<CTime>(DBORDINAL idx, CTime* time) {
				DBTYPE     type;

				if(!GetColumnType(idx, &type))
					return false;

				switch(type) {
					case DBTYPE_DBDATE:
						{
							DBDATE* pDate = (DBDATE*)CDynamicAccessor::GetValue(idx);
							*time = CTime(pDate->year, pDate->month, pDate->day, 0, 0, 0);
						}
						break;
#ifndef MFC_SOURCE						
					case DBTYPE_DBTIMESTAMP:
						{
							*time = CTime(*(DBTIMESTAMP*)CDynamicAccessor::GetValue(idx));
						}
						break;
#endif // !MFC_SOURCE
					case DBTYPE_DATE:
						{
							SYSTEMTIME systm;
							DATE dt = *((DATE*)CDynamicAccessor::GetValue(idx));
							if(!::VariantTimeToSystemTime(dt, &systm))
								return false;

							*time = CTime(systm);
						}
						break;
					default:
						ATLASSERT(false);
						return false;
				}
				return true;
			}
#ifndef DOXYGEN_SKIP_THIS
		}
#endif

		// Specialization for CTime (iff COleDateTime is a known identifier...)
#ifndef DOXYGEN_SKIP_THIS
		__if_exists(COleDateTime) {
#endif
			/*!
			 * \brief <tt>COleDateTime</tt> version for acquiring data from result.
			 *
			 * \note This function will only be visible if <tt>COleDateTime</tt> is available
			 * in the global scope at the point of inclusion.
			 *
			 * \param idx ordinal of column
			 * \param time pointer to receiving storage
			 * \return <tt>true</tt> on success
			 */
			template <>
			bool GetValue<COleDateTime>(DBORDINAL idx, COleDateTime* time) {
				DBTYPE    type;

				if(!GetColumnType(idx, &type))
					return false;

				switch(type) {
					case DBTYPE_DBDATE:
						{
							DBDATE* pDate = (DBDATE*)CDynamicAccessor::GetValue(idx);
							*time = COleDateTime(pDate->year, pDate->month, pDate->day, 0, 0, 0);
						}
						break;
#ifndef MFC_SOURCE						
					case DBTYPE_DBTIMESTAMP:
						{
							*time = COleDateTime(*(DBTIMESTAMP*)CDynamicAccessor::GetValue(idx));
						}
						break;
#endif // !MFC_SOURCE	
					case DBTYPE_DATE:
						{
							*time = COleDateTime(*(DATE*)CDynamicAccessor::GetValue(idx));
						}
						break;
					default:
						ATLASSERT(false);
						return false;
				}
				return true;
			}
#ifndef DOXYGEN_SKIP_THIS
		}
#endif

		/*!
		 * \brief <tt>char*</tt> version for acquiring data from result.
		 *
		 * Function for copying the column as a string into a preallocated string buffer.
		 *
		 * \param lpszColumnName name of column
		 * \param buf string buffer
		 * \param buf_size the size of the string buffer in bytes
		 * \return <tt>true</tt> on success
		 */
		bool GetValue(const CHAR* lpszColumnName, char* buf, size_t buf_size) {
			DBORDINAL  idx;
			if(!GetOrdinal(lpszColumnName, &idx))
				return false;
			return GetValue(idx, buf, buf_size);
		}

		/*!
		 * \brief <tt>wchar_t*</tt> version for acquiring data from result.
		 *
		 * Function for copying the column as a string into a preallocated string buffer.
		 *
		 * \param lpszColumnName name of column
		 * \param buf string buffer
		 * \param buf_size the size of the string buffer in bytes
		 * \return <tt>true</tt> on success
		 */
		bool GetValue(const WCHAR* lpszColumnName, wchar_t* buf, size_t buf_size) {
			DBORDINAL  idx;
			if(!GetOrdinal(lpszColumnName, &idx))
				return false;
			return GetValue(idx, buf, buf_size);
		}

		/*!
		 * \brief <tt>char*</tt> version for acquiring data from result.
		 *
		 * Function for copying the column as a string into a preallocated string buffer.
		 *
		 * \param idx ordinal of column
		 * \param buf string buffer
		 * \param buf_size the size of the string buffer in bytes
		 * \return <tt>true</tt> on success
		 */
		bool GetValue(DBORDINAL idx, char* buf, size_t buf_size) {
			return GetNarrowString(idx, &buf, buf_size);
		}

		/*!
		 * \brief <tt>wchar_t*</tt> version for acquiring data from result.
		 *
		 * Function for copying the column as a string into a preallocated string buffer.
		 *
		 * \param idx ordinal of column
		 * \param buf string buffer
		 * \param buf_size the size of the string buffer in bytes
		 * \return <tt>true</tt> on success
		 */
		bool GetValue(DBORDINAL idx, wchar_t* buf, size_t buf_size) {
			return GetWideString(idx, &buf, buf_size);
		}

#ifndef DOXYGEN_SKIP_THIS
		__if_exists(com::Variant) {
#endif 

		/*!
		 * \brief <tt>com::Variant</tt> version for acquiring data from result.
		 *
		 * \note The variant passed in must not hold any resources, as no attempts to 
		 *  free it will be done!
		 *
		 * \param idx ordinal of column
		 * \param pVar pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
			template <>
			bool GetValue<com::Variant>(DBORDINAL idx, com::Variant* pVar) {
				return GetValue<VARIANT>(idx, pVar);
			}
#ifndef DOXYGEN_SKIP_THIS
		}
#endif
		/*!
		 * \brief <tt>VARIANT</tt> version for acquiring data from result.
		 *
		 * \note The variant passed in must not hold any resources, as no attempts to 
		 *  free it will be done!
		 *
		 * \param idx ordinal of column
		 * \param pVar pointer to receiving storage
		 * \return <tt>true</tt> on success
		 */
		template <>
		bool GetValue<VARIANT>(DBORDINAL idx, VARIANT* pVar) {
			DBTYPE   type;
			DBSTATUS status;

			pVar->vt = VT_EMPTY;

			if(!GetStatus(idx, &status))
				return false;
			
            if(DBSTATUS_S_ISNULL == status) {
				pVar->vt = VT_NULL;
				return true;
			}

			if(DBSTATUS_S_OK != status)
				return false;

			if(!GetColumnType(idx, &type))
				return false;

			if((type & DBTYPE_ARRAY) ||
			   (type & DBTYPE_BYREF) ||
			   (type & DBTYPE_VECTOR) ||
			   (type & DBTYPE_RESERVED))
				return false;

			switch(type) {
				case DBTYPE_EMPTY:
					pVar->vt = VT_EMPTY;
					return true;
				case DBTYPE_NULL:
					pVar->vt = VT_NULL;
					return true;
				case DBTYPE_I1:
					return S_OK == CComVariant(*(char*)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_I2:
					return S_OK == CComVariant(*(short*)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_I4:
					return S_OK == CComVariant(*(long*)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_UI1:
					return S_OK == CComVariant(*(unsigned char*)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_UI2:
					return S_OK == CComVariant(*(unsigned short*)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_UI4:
					return S_OK == CComVariant(*(unsigned long*)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_R4:
					return S_OK == CComVariant(*(float*)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_R8:
					return S_OK == CComVariant(*(double*)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_CY:
					return S_OK == CComVariant(*(CY*)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_DATE:
					return S_OK == CComVariant(*(DATE*)CDynamicAccessor::GetValue(idx), VT_DATE).Detach(pVar);
				case DBTYPE_STR:
					return S_OK == CComVariant((LPCSTR)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_WSTR:
					return S_OK == CComVariant((LPCWSTR)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_BOOL:
					return S_OK == CComVariant(VARIANT_FALSE != *(VARIANT_BOOL*)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_VARIANT:
					return S_OK == ::VariantCopy(pVar, (VARIANT*)CDynamicAccessor::GetValue(idx));
				case DBTYPE_NUMERIC: 
					{
						DECIMAL dec;
						DB_NUMERIC* pNum = (DB_NUMERIC*)CDynamicAccessor::GetValue(idx);
						dec.sign = pNum->sign == 0 ? DECIMAL_NEG : 0; // 0 is NEG, 1 is POS
        				dec.scale = pNum->scale;
        				memcpy((void*)&dec.Lo64, pNum->val, sizeof(dec.Lo64));
        				memcpy((void*)&dec.Hi32, pNum->val + sizeof(dec.Lo64), sizeof(dec.Hi32));
						pVar->decVal = dec;
						pVar->vt = VT_DECIMAL;
						return S_OK;
					}
				__if_exists(COleDateTime) {
					case DBTYPE_DBDATE:
						{
							DBDATE* pDate = (DBDATE*)CDynamicAccessor::GetValue(idx);
							pVar->vt = VT_DATE;
							pVar->date = (DATE)COleDateTime(pDate->year, pDate->month, pDate->day, 0, 0, 0);
							return S_OK;
						}
					case DBTYPE_DBTIMESTAMP:
						{
							pVar->vt = VT_DATE;
							pVar->date = (DATE)COleDateTime(*(DBTIMESTAMP*)CDynamicAccessor::GetValue(idx));
							return S_OK;
						}
					case DBTYPE_DBTIME:
						{
							DBTIME* pTime = (DBTIME*)CDynamicAccessor::GetValue(idx);
							pVar->vt = VT_DATE;
							pVar->date = (DATE)COleDateTime(100, 0, 0, pTime->hour, pTime->minute, pTime->second);
							return S_OK;
						}
				}

				case DBTYPE_IUNKNOWN:
					return S_OK == CComVariant(*(IUnknown**)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				case DBTYPE_IDISPATCH:
					return S_OK == CComVariant(*(IDispatch**)CDynamicAccessor::GetValue(idx)).Detach(pVar);
				
					
				default:
					ATLASSERT(false);
					break;
			}
		
			return false;
		}
	private:

		// Gets a value from the accessor in any (almost any) format, and returns a wide string
		// Any conversions necessary are done. Conversions from narrow to wide are done using the
		// ANSI Codepage, using '?' as default character
		bool GetWideString(DBORDINAL idx, wchar_t** str, size_t buf_size = 0) {
			DBLENGTH    byte_len;
			size_t      len;
			DBTYPE      type;

			if(!GetLength(idx, &byte_len))
				return false;

			if(!GetColumnType(idx, &type))
				return false;

			switch(type) {
				case DBTYPE_WSTR:
					if(0 == *str) {
						*str = new wchar_t[byte_len / sizeof(wchar_t) + 1];
						memcpy(*str, CDynamicAccessor::GetValue(idx), byte_len + sizeof(wchar_t));
					} else {
						wcsncpy(*str, static_cast<wchar_t*>(CDynamicAccessor::GetValue(idx)), buf_size);
						(*str)[buf_size - 1] = 0;
					}
				break;
				case DBTYPE_BSTR:
					if(0 == *str) {
						*str = new wchar_t[byte_len / sizeof(wchar_t) + 1];
						memcpy(*str, CDynamicAccessor::GetValue(idx), byte_len);
						(*str)[byte_len / sizeof(wchar_t) + 1] = 0;
					} else {
						wcsncpy(*str, static_cast<wchar_t*>(CDynamicAccessor::GetValue(idx)), buf_size);
						(*str)[buf_size - 1] = 0;
					}
					break;
				case DBTYPE_STR:
					if(0 == *str) {
						*str = new wchar_t[byte_len + 1];
						wcsncpy(*str, CA2W(static_cast<char*>(CDynamicAccessor::GetValue(idx))), byte_len);
						(*str)[byte_len] = 0;
					} else {
						wcsncpy(*str, CA2W(static_cast<char*>(CDynamicAccessor::GetValue(idx))), buf_size);
						(*str)[buf_size - 1] = 0;
					}
					return true;
				case DBTYPE_I1:
					if(0 == *str) {
						*str = new wchar_t[4];
						len = 4;
					} else
						len = buf_size;

					_snwprintf(*str, len, L"%3d", *(CHAR*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_I2:
					if(0 == *str) {
						*str = new wchar_t[6];
						len = 6;
					} else
						len = buf_size;

					_snwprintf(*str, len, L"%5d", *(SHORT*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_I4:
					if(0 == *str) {
						*str = new wchar_t[11];
						len = 11;
					} else
						len = buf_size;

					_snwprintf(*str, len, L"%10d", *(LONG32*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_I8:
					if(0 == *str) {
						*str = new wchar_t[21];
						len = 21;
					} else
						len = buf_size;

					_snwprintf(*str, len, L"%20I64d", *(LONG64*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_BOOL:
					if(0 == *str) {
						*str = new wchar_t[6];
						len = 6;
					} else
						len = buf_size;

					_snwprintf(*str, len, L"%s", *(VARIANT_BOOL*)CDynamicAccessor::GetValue(idx) == VARIANT_TRUE ? L"true" : L"false");
					break;
				case DBTYPE_DECIMAL:
					{
						BSTR bstr = NULL;
						if(FAILED(VarBstrFromDec((DECIMAL*)CDynamicAccessor::GetValue(idx), 1033 /* US Locale */, 0, &bstr)))
							return false;
						if(0 == *str) {
							len = ::SysStringLen(bstr) + 1;
							*str = new wchar_t[len];
						} else
							len = buf_size;
						wcsncpy(*str, bstr, len);
						::SysFreeString(bstr);
					}
					break;
				case DBTYPE_GUID:
					if(0 == *str) {
						len = 39;
						*str = new wchar_t[len];
					} else
						len = (int)buf_size;
					return 0 != StringFromGUID2(*(GUID*)CDynamicAccessor::GetValue(idx), *str, (int)len);

				case DBTYPE_DATE:
					{
						COleDateTime dt(*(DATE*)CDynamicAccessor::GetValue(idx));
						CString strDt = dt.Format(_T("%Y-%m-%d"));

						if(0 == *str) {
							len = strDt.GetLength();
							*str = new wchar_t[len + 1];
						} else
							len = buf_size;

						wcsncpy(*str, CT2W(strDt, CP_ACP), len);

						return true;
					}
					break;
#ifndef MFC_SOURCE					
				case DBTYPE_DBTIMESTAMP:
					{
						COleDateTime dt(*(DBTIMESTAMP*)CDynamicAccessor::GetValue(idx));
						CString strDt = dt.Format(_T("%Y-%m-%d %H:%M:%S"));

						if(0 == *str) {
							len = strDt.GetLength();
							*str = new wchar_t[len + 1];
						} else
							len = buf_size;

						wcsncpy(*str, CT2W(strDt, CP_ACP), len);

						return true;
					}
					break;
#endif // !MFC_SOURCE					
				case DBTYPE_DBDATE:
					{
						DBDATE* pDate = (DBDATE*)CDynamicAccessor::GetValue(idx);
						COleDateTime dt(pDate->year, pDate->month, pDate->day, 0, 0, 0);
						CString strDt = dt.Format(_T("%Y-%m-%d"));

						if(0 == *str) {
							len = strDt.GetLength();
							*str = new wchar_t[len + 1];
						} else
							len = buf_size;

						wcsncpy(*str, CT2W(strDt, CP_ACP), len);

						return true;
					}
					break;
				default:
					ATLASSERT(false);
					return false;
			}
			return true;
		}

		// Gets a value from the accessor in any (almost any) format, and returns a narrow/8bit string
		// Any conversions necessary are done. Conversions from wide to narrow are done using the
		// ANSI Codepage, using '?' as default character
		bool GetNarrowString(DBORDINAL idx, char** str, size_t buf_size = 0) {
			DBLENGTH   byte_len;
			size_t     len;
			DBTYPE     type;

			if(!GetLength(idx, &byte_len))
				return false;

			if(!GetColumnType(idx, &type))
				return false;

			switch(type) {
				case DBTYPE_WSTR:
					if(0 == *str) {
						int len = byte_len / sizeof(wchar_t) + 1;
						*str = new char[len];
						strncpy(*str, CW2A(static_cast<wchar_t*>(CDynamicAccessor::GetValue(idx))), len);
						(*str)[len] = 0;
					} else {
						strncpy(*str, CW2A(static_cast<wchar_t*>(CDynamicAccessor::GetValue(idx))), buf_size);
						(*str)[buf_size - 1] = 0;
					}
					return true;
				case DBTYPE_BSTR:
					if(0 == *str) {
						int len = byte_len / sizeof(wchar_t) + 1;
						*str = new char[len];
						strncpy(*str, CW2A(static_cast<wchar_t*>(CDynamicAccessor::GetValue(idx))), len);
						(*str)[len] = 0;
					} else {
						strncpy(*str, CW2A(static_cast<wchar_t*>(CDynamicAccessor::GetValue(idx))), buf_size);
						(*str)[buf_size - 1] = 0;
					}
					return true;
				case DBTYPE_STR:
					if(0 == *str) {
	                    *str = new char[byte_len + 1];
						memcpy(*str, CDynamicAccessor::GetValue(idx), byte_len + 1);
					} else {
						strncpy(*str, static_cast<char*>(CDynamicAccessor::GetValue(idx)), buf_size);
						(*str)[buf_size - 1] = 0;
					}
					break;
				case DBTYPE_I1:
					if(0 == *str) {
						*str = new char[4];
						len = 4;
					} else
						len = buf_size;

					_snprintf(*str, len, "%3d", *(CHAR*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_I2:
					if(0 == *str) {
						*str = new char[6];
						len = 6;
					} else
						len = buf_size;

					_snprintf(*str, len, "%5d", *(SHORT*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_I4:
					if(0 == *str) {
						*str = new char[11];
						len = 11;
					} else
						len = buf_size;

					_snprintf(*str, len, "%10d", *(LONG32*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_I8:
					if(0 == *str) {
						*str = new char[21];
						len = 21;
					} else
						len = buf_size;

					_snprintf(*str, len, "%20I64d", *(LONG64*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_BOOL:
					if(0 == *str) {
						*str = new char[6];
						len = 6;
					} else
						len = buf_size;

					_snprintf(*str, len, "%s", *(VARIANT_BOOL*)CDynamicAccessor::GetValue(idx) == VARIANT_TRUE ? "true" : "false");
					break;
				case DBTYPE_DECIMAL:
					{
						BSTR bstr = NULL;
						if(FAILED(VarBstrFromDec((DECIMAL*)CDynamicAccessor::GetValue(idx), 1033 /* US Locale */, 0, &bstr)))
							return false;
						if(0 == *str) {
							len = ::SysStringLen(bstr) + 1;
							*str = new char[len];
						} else
							len = buf_size;
						strncpy(*str, CW2A(bstr), len);
						::SysFreeString(bstr);
					}
					break;
				case DBTYPE_GUID:
					{
						wchar_t* tmp_buf = new wchar_t[39];

						if(0 == *str) {
							len = 39;
							*str = new char[len];
						} else
							len = buf_size;

						if(StringFromGUID2(*(GUID*)CDynamicAccessor::GetValue(idx), tmp_buf, 39) != 0) {
							delete [] tmp_buf;
							return false;
						}

						strncpy(*str, CW2A(tmp_buf), len);
						delete [] tmp_buf;
					}
					break;
				case DBTYPE_DATE:
					{
						COleDateTime dt(*(DATE*)CDynamicAccessor::GetValue(idx));
						CString strDt = dt.Format(_T("%Y-%m-%d"));

						if(0 == *str) {
							len = strDt.GetLength();
							*str = new char[len + 1];
						} else
							len = buf_size;

						strncpy(*str, CT2A(strDt, CP_ACP), len);

						return true;
					}
					break;
#ifndef MFC_SOURCE					
				case DBTYPE_DBTIMESTAMP:
					{
						COleDateTime dt(*(DBTIMESTAMP*)CDynamicAccessor::GetValue(idx));
						CString strDt = dt.Format(_T("%Y-%m-%d %H:%M:%S"));

						if(0 == *str) {
							len = strDt.GetLength();
							*str = new char[len + 1];
						} else
							len = buf_size;

						strncpy(*str, CT2A(strDt, CP_ACP), len);

						return true;
					}
					break;
#endif // !MFC_SOURCE					
				case DBTYPE_DBDATE:
					{
						DBDATE* pDate = (DBDATE*)CDynamicAccessor::GetValue(idx);
						COleDateTime dt(pDate->year, pDate->month, pDate->day, 0, 0, 0);
						CString strDt = dt.Format(_T("%Y-%m-%d"));

						if(0 == *str) {
							len = strDt.GetLength();
							*str = new char[len + 1];
						} else
							len = buf_size;

						strncpy(*str, CT2A(strDt, CP_ACP), len);

						return true;
					}
					break;
				default:
					ATLASSERT(false);
					return false;
			}
			return true;
		}

		template <typename ToType>
		inline bool CastIntegerValue(DBORDINAL idx, ToType* pData) {
			DBTYPE type;

			if(!GetColumnType(idx, &type))
				return false;

			switch(type) {
				case DBTYPE_BOOL:
					*pData = static_cast<ToType>(*(VARIANT_BOOL*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_I1:
					*pData = static_cast<ToType>(*(CHAR*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_I2:
					*pData = static_cast<ToType>(*(SHORT*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_I4:
					*pData = static_cast<ToType>(*(LONG32*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_I8:
					*pData = static_cast<ToType>(*(LONG64*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_UI1:
					*pData = static_cast<ToType>(*(BYTE*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_UI2:
					*pData = static_cast<ToType>(*(USHORT*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_UI4:
					*pData = static_cast<ToType>(*(ULONG32*)CDynamicAccessor::GetValue(idx));
					break;
				case DBTYPE_UI8:
					*pData = static_cast<ToType>(*(ULONG64*)CDynamicAccessor::GetValue(idx));
					break;
				default:
					ATLASSERT(false);
					return false;
			}
			return true;
		}
	};
}