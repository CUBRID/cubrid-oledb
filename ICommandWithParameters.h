/*
 * Copyright (C) 2008 Search Solution Corporation. All rights reserved by Search Solution. 
 *
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met: 
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer. 
 *
 * - Redistributions in binary form must reproduce the above copyright notice, 
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution. 
 *
 * - Neither the name of the <ORGANIZATION> nor the names of its contributors 
 *   may be used to endorse or promote products derived from this software without 
 *   specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE. 
 *
 */

#pragma once

template <class T>
class ATL_NO_VTABLE ICommandWithParametersImpl : public ICommandWithParameters
{
public:
        HRESULT STDMETHODCALLTYPE GetParameterInfo( 
            DB_UPARAMS *pcParams,
            DBPARAMINFO **prgParamInfo,
            OLECHAR **ppNamesBuffer)
		{
			ATLTRACE(atlTraceDBProvider, 2, _T("ICommandWithParamtersImpl::GetParameterInfo\n"));

			T* pT = (T*)this;
			T::ObjectLock lock(pT);

			DBCOUNTITEM		cParams = pT->m_cParams; //�Ķ���� ����
			DBPARAMINFO*	pParamInfo = pT->m_pParamInfo; //�Ķ���� ���� ����ü �迭

			//�Ķ���� ���� �ʱ⿡ 0���� ����
			*pcParams = 0;

			//argument üũ
			if (pcParams == NULL || prgParamInfo == NULL)
				return E_INVALIDARG;

			//�ʱ�ȭ
			*prgParamInfo = NULL;
			
			//�Ķ���� �̸� �迭 NULL�� �ʱ�ȭ
			if (ppNamesBuffer != NULL)
				*ppNamesBuffer = NULL;

			//���õ� �Ķ���Ͱ� ���� ���
			if (cParams == 0)
				return DB_E_PARAMUNAVAILABLE;

			//�Ķ���� ���� ī��
			*pcParams = cParams;

			//�Ķ���� ���� ����ü �迭 ī��
			*prgParamInfo = (DBPARAMINFO*)CoTaskMemAlloc(cParams * sizeof(DBPARAMINFO));
			memcpy(*prgParamInfo, pParamInfo, cParams * sizeof(DBPARAMINFO));

			//�Ķ���� �̸��� ���õǾ� �ִ��� üũ
			if (pParamInfo[0].pwszName != NULL)
			{
				//�Ķ���� �̸� �迭�� ���� �� Byte ���̸� ���
				ULONG i;
				size_t cChars;
				size_t cBytes = 0;

				for (i = 0; i < cParams; i++)
				{
					//�迭���� ������ �̸� �ڿ��� null character�� �´�
					cChars = wcslen(pParamInfo[i].pwszName) + 1;

					//�Ķ���� �̸��� ���̸� �ӽ� ����
					(*prgParamInfo)[i].pwszName = (WCHAR*)cChars;
					
					cBytes += cChars * sizeof(WCHAR);
				}

				//��� �� ����Ʈ ��ŭ �޸� �Ҵ�
				*ppNamesBuffer = (WCHAR*)CoTaskMemAlloc(cBytes);

				//Fill out the string buffer
				WCHAR* pstrTemp = *ppNamesBuffer;
				for (i = 0; i < cParams; i++)
				{
					//�ӽ� ����� �Ķ���� �̸��� ����
					cChars = (size_t)(*prgParamInfo)[i].pwszName;

					//�Ķ���� �̸� ī��
					wcscpy(pstrTemp, pParamInfo[i].pwszName);
					(*prgParamInfo)[i].pwszName = pstrTemp;

					//�Ķ���� �̸� �迭���� �ε��� �̵�
					pstrTemp += cChars;
				}
			}

			return S_OK;
		}
        
        HRESULT STDMETHODCALLTYPE MapParameterNames( 
			DB_UPARAMS cParamNames,
			const OLECHAR *rgParamNames[],
			DB_LPARAMS rgParamOrdinals[])
		{
			ATLTRACE(atlTraceDBProvider, 2, _T("ICommandWithParamtersImpl::MapParameterNames\n"));

			T* pT = (T*)this;
			T::ObjectLock lock(pT);

			bool bParamFound = false;
			bool bParamNotFound = false;

			//cParamNames�� 0�̸� �׳� S_OK�� ����
			if (cParamNames == 0)
				return S_OK;

			//cParamNames�� 0�� �ƴϰ� rgParamNames Ȥ�� rgParamOrdinals�� NULL�̸�
			//E_INVALIDARG�� ����
			if (rgParamNames == NULL || rgParamOrdinals == NULL)
				return E_INVALIDARG;

			for (ULONG i = 0; i < cParamNames; i++)
			{
				bool bFound = false;
				for (ULONG j = 0; j < pT->m_cParams; j++)
				{
					//_wcsicmp -> lower case comparison
					if (_wcsicmp(rgParamNames[i], pT->m_pParamInfo[j].pwszName) == 0)
					{
						bFound = true;
						break;
					}
				}

				//�Ķ���� �̸��� ���� ���� �߰ߵǸ� bParamFound�� true�� �����ϰ�
				//�ϳ��� �߰ߵ��� ���ϸ� bParamNotFound�� true�� ����
				if (bFound)
				{
					rgParamOrdinals[i] = j + 1;
					bParamFound = true;
				}
				else
				{
					rgParamOrdinals[i] = 0;
					bParamNotFound = true;
				}

			}

			//�Ķ���� �̸��� ������ �Ķ���� �̸��� ��ġ�� ��
			if (bParamFound && !bParamNotFound)
				return S_OK;

			//�ϳ��� ��ġ���� ������
			if (bParamFound && bParamNotFound)
				return DB_S_ERRORSOCCURRED;

			return DB_E_ERRORSOCCURRED;
		}
        
        HRESULT STDMETHODCALLTYPE SetParameterInfo( 
			DB_UPARAMS cParams,
			const DB_UPARAMS rgParamOrdinals[],
			const DBPARAMBINDINFO rgParamBindInfo[])
		{
			ATLTRACE(atlTraceDBProvider, 2, _T("ICommandWithParamtersImpl::SetParameterInfo\n"));

			T* pT = (T*)this;
			T::ObjectLock lock(pT);
			
			ULONG i;
			DBPARAMINFO* pParamInfo = NULL;
			
			HRESULT hr = S_OK;
			DBCOUNTITEM nCount = cParams;

			//�Լ� argument üũ
			if (cParams != 0 && rgParamOrdinals == NULL)
				return E_INVALIDARG;

			if (cParams == 0)
				goto Success;
			
			
			//������ �Ķ���� ���� ������ �Ķ���ͼ��� ���Ѵ�
			if (pT->m_cParams != NULL)
				nCount += pT->m_cParams;

			//���� nCount��ŭ�� ���۸� �Ҵ��Ѵ�
			pParamInfo = (DBPARAMINFO*)CoTaskMemAlloc(nCount * sizeof(DBPARAMINFO));
			
			//������ �Ķ���� ������ �־��� �� ���� pParamInfo�� ī���Ѵ�
			if (pT->m_pParamInfo != NULL)
			{
				memcpy(pParamInfo, pT->m_pParamInfo, pT->m_cBindings * sizeof(DBPARAMINFO));

				for (i = 0; i < pT->m_cParams; i++)
					pParamInfo[i].pwszName = SysAllocString(pT->m_pParamInfo[i].pwszName);
			}
						
			ULONG j;
			nCount = pT->m_cParams;
			for (i = 0; i < cParams; i++)
			{
				//�Ķ���� Ordinal�� 0�̸� E_INVALIDARD ����
				if (rgParamOrdinals[i] == 0)
				{
					hr = E_INVALIDARG;
					goto Error;
				}

				//���ο� �Ķ������ ordinal�� ���� ������ ordinal�� ��ġ������ ����
				bool bFound = false;
				for (j = 0; j < nCount; j++)
				{
					if (pParamInfo[j].iOrdinal == rgParamOrdinals[i])
					{
						bFound = true;
						break;
					}
				}

				if (!bFound)
				{
					//�߰ߵ��� �ʾ����� ���ο� ordinal�� �ο�
					j = nCount;
					nCount++;
				}
				else
				{
					//������ �Ķ���� �̸� ����
					if (pParamInfo[j].pwszName != NULL)
						SysFreeString(pParamInfo[j].pwszName);
					
					//�Ķ���� ������ override�Ǿ����� ����
					hr = DB_S_TYPEINFOOVERRIDDEN; 
				}


				//���ο� �Ķ���� information ����
				if (rgParamBindInfo[i].pwszName == NULL || *rgParamBindInfo[i].pwszName == 0)
					pParamInfo[j].pwszName  = NULL;
				else
					pParamInfo[j].pwszName	= SysAllocString(rgParamBindInfo[i].pwszName);

				pParamInfo[j].dwFlags		= rgParamBindInfo[i].dwFlags;
				pParamInfo[j].iOrdinal		= rgParamOrdinals[i];
				pParamInfo[j].pTypeInfo		= NULL;
				pParamInfo[j].ulParamSize	= rgParamBindInfo[i].ulParamSize;
				pParamInfo[j].wType			= GetOledbTypeFromName(rgParamBindInfo[i].pwszDataSourceType);
				pParamInfo[j].bPrecision	= rgParamBindInfo[i].bPrecision;
				pParamInfo[j].bScale		= rgParamBindInfo[i].bScale;
			}

			//DBPARAMINFO ����ü �迭���� ��� �׸� �Ķ���� �̸��� ���õǾ� �ִ��� üũ
			for (i = 1; i < nCount; i++)
			{
				if ((pParamInfo[0].pwszName == NULL && pParamInfo[i].pwszName != NULL) ||
					(pParamInfo[0].pwszName != NULL && pParamInfo[i].pwszName == NULL)) 
				{
					hr = DB_E_BADPARAMETERNAME;
					goto Error;
				}
			}

		Success:
			//������ ���� ����
			if (pT->m_pParamInfo != NULL)
			{
				for (i = 0; i < pT->m_cParams; i++)
					SysFreeString(pT->m_pParamInfo[i].pwszName);

				CoTaskMemFree(pT->m_pParamInfo);
			}

			//�� ������ ����
			//m_cParams : �Ķ���� ����
			//m_cParamInfo : �Ķ���� ���� ����ü �迭
			pT->m_cParams = nCount;
			pT->m_pParamInfo = pParamInfo;

			return hr;

		Error:
			//���� �����Ϸ��� ���� ����
			for (i = 0; i < nCount; i++)
				SysFreeString(pParamInfo[i].pwszName);

			CoTaskMemFree(pParamInfo);
			
			return hr;
		}

};
