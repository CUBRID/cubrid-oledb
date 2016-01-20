/*!
 * $RCSfile: dynamiccommand.h,v $
 * \file oledb/dynamiccommand.h
 *
 * \brief Defines a dynamic command class for use with OLE DB.
 *
 * \author J&ouml;rgen Sigvardsson &lt;jorgen@profitab.com&gt;
 * \version $Revision: 1.15 $
 */
#pragma once

#include <vector>
#include <algorithm>
#include <string>
#include "stringutils.h"
#include "pairutils.h"

/*!
 * \brief Namespace for OLE DB classes and functions.
 */
namespace oledb {
		
	/*!
	 * \brief OLE DB Command class which tries to mimic the <tt>java.sql.Statement</tt> class.
	 *
	 * Statements are written on the form <tt>"SELECT x, y, z FROM table WHERE w = ? AND v LIKE ?"</tt>.
	 * The markers, <tt>?</tt>, are then replace subsequently with values by using any of the 
	 * overloaded <tt>SetParamValue()</tt> functions. The class automatically handles strings, dates, 
	 * etc. It even escapes the strings.
	 *
	 * \author J&ouml;rgen Sigvardsson \<jorgen@profitab.com\>
	 */
#ifndef DOXYGEN_SKIP_THIS
	template <class TAccessor = CNoAccessor, template <typename T> class TRowset = CRowset, 
			  class TMultiple = CNoMultipleResults, TCHAR PARAM_CHAR = _T('?')> 
#endif 			  
	class CDynamicCommand : public CCommand<TAccessor, TRowset, TMultiple> {
		typedef CCommand<TAccessor, TRowset, TMultiple> super;
		typedef std::vector<std::pair<bool, stdex::tstring> > StringVector;
	


		StringVector	 m_vecParamValues;
		stdex::tstring   m_strCmdTemplate;
	
	private:
	
		// Escape a string value.
		// Rules: 
		//    Any ' becomes ''
		stdex::tstring& escape_string(stdex::tstring& str) {
			return stdex::escape(str, _T('\''), _T('\''));
		}
	
		int CountParameters(LPCTSTR lpszCommand) {
			return (int)std::count(lpszCommand, lpszCommand + lstrlen(lpszCommand), PARAM_CHAR);
		}
	
		// Constructs the "real" command string based on 
		HRESULT ConstructCommand(stdex::tstring& strCommand) {
			strCommand.clear();
	
			if(m_strCmdTemplate.find(PARAM_CHAR, 0) == stdex::tstring::npos) {
				strCommand = m_strCmdTemplate; // There were no parameters
			} else {
				int     nParam = 0;
				size_t  begin = 0;
		
				do {
					size_t i = m_strCmdTemplate.find(PARAM_CHAR, begin);
					// Copy from begining upto found param character (or end if end of string was found)
					strCommand.append(m_strCmdTemplate, begin, (i == stdex::tstring::npos ? m_strCmdTemplate.size() : i) - begin);
		
					// If we didn't hit the end of string, then i points to a param character
					if(i != stdex::tstring::npos) {
						// if it's escaped, then we should not take any action but appending the param character
						// onto the command string as if nothing had happened
						if(i > 0 && m_strCmdTemplate[i - 1] == _T('\\')) { // it's an escaped ?
							strCommand.append(&m_strCmdTemplate[i], 1);
						} else {
							// We've got a parameter character unescaped. 
		
							// First check if the parameter has been initialized by SetParamValue()
							if(!m_vecParamValues[nParam].first) { // Uninitialized
								return E_FAIL;
							}
			
							// All well, just put the parameter value on command instead of parameter character
							strCommand.append(m_vecParamValues[nParam].second);
		
							nParam++;
						}
						// Step over parameter character..
						++i;
					}
		
					begin = i;
				} while(begin != stdex::tstring::npos);  
			}
			
			return S_OK;
		}
	
	public:
	
		// Set the command template.. I.e. SELECT * FROM table WHERE column = ?
		// Please not that allthough the default parameter character is '?', it can be specified 
		// by using the template parameter PARAM_CHAR
		/*!
		 * \brief Sets the command template.
		 *
		 * Example: <tt>SetCommandTemplate(_T("SELECT * FROM table WHERE x = ?"))</tt>
		 *
		 * \param lpszCommand the command template
		 */
		void SetCommandTemplate(LPCTSTR lpszCommand) {
			m_strCmdTemplate = lpszCommand;
			m_vecParamValues.clear();
	
			int nNumParams = CountParameters(lpszCommand);
			if(nNumParams == 0)
				return;
	
			m_vecParamValues.resize(nNumParams);
	
			// Mark each string entry as "untouched"
			std::for_each(m_vecParamValues.begin(), m_vecParamValues.end(), stdex::set_first(false));
		}
	
	#define ASSERT_PARAM_INDEX(nParam)									\
		do {															\
			ATLASSERT(nParam >= 0 && nParam < m_vecParamValues.size());	\
			if(nParam < 0 || nParam >= m_vecParamValues.size())			\
				return E_INVALIDARG;									\
		} while(0)
	
		/*!
		 * \brief Replaces parameter with a <tt>long</tt> value.
		 *
		 * \param nParam index of parameter, 0-based index
		 * \param nValue value to replace parameter
		 * \retval E_INVALIDARG if index is out of range
		 * \retval S_OK on success
		 */
		HRESULT SetParamValue(size_t nParam, long nValue) {
			ASSERT_PARAM_INDEX(nParam);
	
			m_vecParamValues[nParam].first = true;
			stdex::tstringstream strm;
			strm << nValue;
			m_vecParamValues[nParam].second = strm.str();
	
			return S_OK;
		}
	
		/*!
		 * \brief Replaces parameter with a string value.
		 *
		 * \param nParam index of parameter, 0-based index
		 * \param lpszValue value to replace parameter
		 * \retval E_INVALIDARG if index is out of range
		 * \retval S_OK on success
		 */
		HRESULT SetParamValue(size_t nParam, LPCSTR lpszValue) {
			ASSERT_PARAM_INDEX(nParam);
	
			m_vecParamValues[nParam].first = true;
			m_vecParamValues[nParam].second = _T("'");
			m_vecParamValues[nParam].second.append(escape_string(stdex::tstring(CA2CT(lpszValue))));
			m_vecParamValues[nParam].second.append(_T("'"));
	
			return S_OK;
		}
	
		/*!
		 * \brief Replaces parameter with a string value.
		 *
		 * \param nParam index of parameter, 0-based index
		 * \param bstrValue value to replace parameter
		 * \retval E_INVALIDARG if index is out of range
		 * \retval S_OK on success
		 */
		HRESULT SetParamValue(size_t nParam, LPCOLESTR bstrValue) {
			ASSERT_PARAM_INDEX(nParam);
	
			m_vecParamValues[nParam].first = true;
			m_vecParamValues[nParam].second = _T("'");
			m_vecParamValues[nParam].second.append(escape_string(stdex::tstring(CW2CT(bstrValue))));
			m_vecParamValues[nParam].second.append(_T("'"));
	
			return S_OK;
		}
	
		/*!
		 * \brief Replaces parameter with a date value.
		 *
		 * \param nParam index of parameter, 0-based index
		 * \param dtValue value to replace parameter
		 * \retval E_INVALIDARG if index is out of range
		 * \retval S_OK on success
		 */	
		HRESULT SetParamValue(size_t nParam, const DATE& dtValue) {
			ASSERT_PARAM_INDEX(nParam);
	
			COleDateTime dt(dtValue);
		
			if(!(dt.GetHour() || dt.GetMinute() || dt.GetSecond()))
				m_vecParamValues[nParam].second = dt.Format(_T("'%m/%d/%Y'"));
			else
				m_vecParamValues[nParam].second = dt.Format(_T("'%m/%d/%Y %H:%M:%S'"));
	
			m_vecParamValues[nParam].first = true;
	
			return S_OK;
		}
	
	
		/*!
		 * \brief Replaces parameter with a <tt>bool</tt> value.
		 *
		 * \param nParam index of parameter, 0-based index
		 * \param bValue value to replace parameter
		 * \retval E_INVALIDARG if index is out of range
		 * \retval S_OK on success
		 */
		HRESULT SetParamValue(size_t nParam, bool bValue) {
			ASSERT_PARAM_INDEX(nParam);
	
			m_vecParamValues[nParam].first = true;
			m_vecParamValues[nParam].second = (bValue == true ? _T("1") : _T("0"));
	
			return S_OK;
		}
		
		/*! 
		 * \brief Replaces parameter with the value inside a <tt>VARIANT</tt>.
		 *
		 * \note Variants of type VT_NULL will replace parameter with the SQL NULL value.
		 *
		 * \param nParam index of parameter, 0-based index
		 * \param varValue value to replace parameter
		 * \retval E_INVALIDARG if index is out of range
		 * \retval S_OK on success
		 */
		HRESULT SetParamValue(size_t nParam, const VARIANT& varValue) {
			ASSERT_PARAM_INDEX(nParam);
			
			HRESULT hr;
			switch(varValue.vt) {
			case VT_NULL:
				hr = SetNull(nParam);
				break;
			case VT_I4:
				hr = SetParamValue(nParam, varValue.lVal);
				break;
			case VT_I2:
				hr = SetParamValue(nParam, (long)varValue.iVal);
				break;
			case VT_I1:
				hr = SetParamValue(nParam, (long)varValue.cVal);
				break;
			case VT_BSTR:
				hr = SetParamValue(nParam, (LPCOLESTR)varValue.bstrVal);
				break;
			case VT_BOOL:
				hr = SetParamValue(nParam, VARIANT_BOOL_TO_BOOL(varValue.boolVal));
				break;
			case VT_DATE:
				hr = SetParamValue(nParam, varValue.date);
				break;
			case VT_DECIMAL:
				hr = SetParamValue(nParam, varValue.decVal);
				break;
			default:
				ATLASSERT(false);
				hr = E_INVALIDARG;	
			}
			
			if(hr == S_OK)
				m_vecParamValues[nParam].first = true;
			
			return hr;
		}
		
		/*!
		 * \brief Replaces parameter with a null value.
		 * 
		 * \param nParam index of parameter, 0-based index
		 * \retval E_INVALIDARG if index is out of range
		 * \retval S_OK on success
		 */
		HRESULT SetNull(size_t nParam) {
			ASSERT_PARAM_INDEX(nParam);
			
			m_vecParamValues[nParam].first = true;
			m_vecParamValues[nParam].second = _T("NULL");
			
			return S_OK;
		}
	
	#undef ASSERT_PARAM_INDEX
	
		/*!
		 * \brief Opens the command using a session.
		 *
		 * See <tt>ATL::CCommand::Open()</tt> for more information.
		 */
		HRESULT Open(const CSession& session, DBPROPSET* pPropSet = NULL, DBROWCOUNT* pRowsAffected = NULL,
		             REFGUID guidCommand = DBGUID_DEFAULT, bool bBind = true,
					 ULONG ulPropSets = 0) throw() {
	
			 stdex::tstring strCommand;
			 HRESULT hr = ConstructCommand(strCommand);
				 if(FAILED(hr)) return hr;
	
			 return super::Open(session, strCommand.c_str(), pPropSet, pRowsAffected, guidCommand, bBind, ulPropSets);
		}
	};
}