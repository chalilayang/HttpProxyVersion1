#pragma once
#include "SohuToolExport.h"
#include "Singleton.h"
#include <wininet.h>
#include <vector>
#include <set>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
//#include "smart_ptr/enable_shared_from_this.hpp"
#include <boost/smart_ptr/enable_shared_from_this.hpp>
#include <atlstr.h>
#include <atlcore.h>
#include <boost/signals2.hpp>

using namespace std;

namespace SohuTool
{
	enum EHttpRequestType
	{
		EHttpRequest_Post= 0,
		EHttpRequest_Get,
		EHttpRequest_PostMultiPartsData
	};

	typedef struct tagInnerHttpHeader
	{
		CAtlString strName;
		CAtlString strValue;
		int		modify;
		tagInnerHttpHeader()
		{
			modify = HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE;
		}
		tagInnerHttpHeader(LPCTSTR szName,LPCTSTR szValue,int nModify = HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE)
		{
			strName = szName;
			strValue= szValue;
			modify	= nModify;
		}
	}InnerHttpHeader;

	typedef struct tagHttpHeader
	{
		LPCTSTR szName;
		LPCTSTR szValue;
		int		modify;
		tagHttpHeader()
		{
			modify = HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE;
			szName = szValue = NULL;
		}
	}HttpHeader;

	typedef struct  tagPostArgument
	{
		CAtlString	    strPartName;
		CAtlString		strFileName;
		CAtlString		strContentType;
		string				postContent;
		void Reset()
		{
			strPartName.Empty();
			strFileName.Empty();
			strContentType.Empty();
			postContent.clear();
		}
	}PostArgument;

	typedef struct  tagPostBinaryArg
	{
		CAtlString		strPartName;
		CAtlString		strFileName;
		CAtlString		strContentType;
		string			postContent;
		CAtlString		strFilePath;
		void Reset()
		{
			strPartName.Empty();
			strFileName.Empty();
			strFilePath.Empty();
			strContentType.Empty();
			postContent.clear();
		}
	}PostBinaryArg;

	typedef boost::signals2::signal<void(DWORD errorCode,DWORD statusCode,LPCSTR data,UINT len)>	HttpRecvDataSignal;
	typedef boost::signals2::signal<void(DWORD errorCode,DWORD statusCode,DWORD dwPos)>				HttpPostDataSignal;
	typedef boost::signals2::signal<void(DWORD errorCode,DWORD statusCode)>							HttpRequestSignal;

	class CHttpFactory;

	class SOHUTOOL_API CHttpClient
	{
		friend class CHttpFactory;
		friend std::string GetHttpRspContent(const boost::shared_ptr<CHttpClient>& httpPtr);
		friend std::string GetHttpRspContent(const CHttpClient* httpPtr);
	public:
		CHttpClient(void);
		CHttpClient(CHttpFactory* factory);
		virtual ~CHttpClient(void);
	public:
		void		AddHeader(LPCTSTR szName,LPCTSTR szValue,int modify = HTTP_ADDREQ_FLAG_ADD|HTTP_ADDREQ_FLAG_REPLACE);
		void		ClearHeader();
		void		SetProxy(LPCTSTR szName,LPCTSTR szUserName,LPCTSTR szPassword);
		void		SetBoundaryName(LPCTSTR szBoundaryName);
		LPCTSTR		GetBoundaryName();
		void		SetAgent(LPCTSTR szAgent);
		LPCTSTR		GetAgent();
		void		SetReferer(LPCTSTR szReferer);
		LPCTSTR		GetReferer();
		void		SetCookie(LPCTSTR szCookie);
		BOOL		Request(EHttpRequestType type, LPCTSTR szUrl,const HttpRequestSignal::slot_type& slot);
		BOOL		Request(EHttpRequestType type, LPCTSTR szUrl);
		void		SetPostFile(LPCTSTR szPartName,LPCTSTR szFileName,LPCSTR szContent,DWORD dwLen,LPCTSTR szContentType);
		void		SetPostFile(LPCTSTR szPartName,LPCTSTR szFileName,LPCTSTR szFilePath,LPCTSTR szContentType);
		void		SetPostData(LPVOID data,int len);
		void		Close();
		//
		void		SetRecvCallbck(const HttpRecvDataSignal::slot_type& slot);
		void		SetPostCallbck(const HttpPostDataSignal::slot_type& slot);
		//
		DWORD		GetStatusCode()const;
		DWORD		GetContentLength()const;
		DWORD		GetErrorCode()const;
		int			GetRspHeaderCount();
		HttpHeader	GetRspHeader(int index);
		LPCTSTR		GetRspHeader();
		int			GetHeaderCount();
		HttpHeader  GetHeader(int index);
		LPCTSTR		GetRequestUrl();
		//设置每次recv时块的大小
		void		SetRecvBlockLen(int blockLen);
		int			GetRecvBlockLen()const;
		BOOL		IfEncryption() { return m_ifEncryption;}
		void		SetRecvCallbck(LPVOID recvSlot);
		void		SetPostCallbck(LPVOID postSlot);
	protected:
		LPCSTR		GetRspContent(UINT& len)const
		{
			len = m_response.size();
			return m_response.c_str();
		}
		BOOL		Request(EHttpRequestType type, LPCTSTR szUrl,LPVOID requestSlot);
		BOOL		OpenSession();
		BOOL		OpenConnect();
		BOOL		OpenRequest();
		BOOL		RecvResponse();
		BOOL		RecvHeader();
		BOOL		Request();
		BOOL		Post();
		BOOL		PostFile();
		BOOL		PostData();
		BOOL		Get();
		void		DoRecvCallback(const LPVOID data,DWORD dwSize);
		void		DoPostCallback(DWORD dwPos);
		void		DoRequestCallback();
		void		ParseHeader();
		void		Reset();
	private:
		ATL::CAtlString GetIEUserAgent(void);
	protected:
#pragma warning(push)
#pragma warning(disable:4251)
		HINTERNET 			 m_hSeccion;
		HINTERNET 			 m_hConnect;
		HINTERNET 			 m_hRequest;
		CAtlString		 m_strUrl;
		CAtlString		 m_strRealUrl;
		CAtlString		 m_strAgent;
		CAtlString		 m_strScheme;
		CAtlString		 m_strReferer;
		CAtlString		 m_strContentType;
		CAtlString		 m_strBoundaryName;
		CAtlString		 m_strConnectIp;
		CAtlString		 m_strCookie;
		PostBinaryArg	 m_postBinaryArg;
		//
		INTERNET_PORT		 m_port;
		INTERNET_SCHEME		 m_scheme;
		EHttpRequestType	 m_requestType;
		vector<InnerHttpHeader>	 m_headerList;
		//
		boost::shared_ptr<HttpRecvDataSignal> m_recvSignal;
		boost::shared_ptr<HttpPostDataSignal> m_postSignal;
		boost::shared_ptr<HttpRequestSignal>  m_requestSignal;
		//response
		vector<InnerHttpHeader>	 m_rspHeaderList;
		DWORD				 m_contentLen;
		DWORD				 m_statusCode;
		DWORD				 m_errorCode;
		string				 m_response;
		CAtlString			 m_rspHeader;
		volatile BOOL		 m_requestCompleted;
		volatile int		 m_recvBlockLen;
		volatile BOOL		 m_ifEncryption;
		long				 m_useCount;
		//
		boost::shared_ptr<ATL::CComAutoCriticalSection>	m_sec;
		//
		boost::function<void(void)>		m_requestExec;
	#pragma warning(pop)
	};

	typedef boost::shared_ptr<CHttpClient> HttpClientPtr;
	inline HttpClientPtr CreateWebRequest();
	class SOHUTOOL_API CHttpFactory : public SingletonImpl<CHttpFactory>
	{
		friend class CHttpClient;
		friend HttpClientPtr CreateWebRequest();
	public:
		~CHttpFactory(void);
		CHttpFactory(void);
	public:
		void Init();
		void UnInit();
		void ReleaseWebRequest(CHttpClient* pHttpClient);
#pragma warning(push)
#pragma warning(disable:4251)
	private:
		void OnRequest(CHttpClient* requestPtr);
		void InnerRequest(CHttpClient* request);
		CHttpClient* CreateWebRequest();
	private:
		set<CHttpClient* >			  m_requestSet;
		ATL::CComAutoCriticalSection  m_sec;
#pragma warning(pop)
		bool						  m_bExist;
	};

	inline void ReleaseWebRequest(CHttpClient* pHttpClient)
	{
		CHttpFactory::Instance().ReleaseWebRequest(pHttpClient);
	}

	inline HttpClientPtr CreateWebRequest()
	{
		return boost::shared_ptr<CHttpClient>(CHttpFactory::Instance().CreateWebRequest(),ReleaseWebRequest);
	}

	inline std::string GetHttpRspContent(const HttpClientPtr& httpPtr)
	{
		std::string strRsp;
		if(httpPtr.get())
		{
			UINT	len = 0;
			LPCSTR  szRsp = httpPtr->GetRspContent(len);
			strRsp.append(szRsp,len);
		}
		return strRsp;
	}

	inline std::string GetHttpRspContent(const CHttpClient* httpPtr)
	{
		std::string strRsp;
		if(httpPtr)
		{
			UINT	len = 0;
			LPCSTR  szRsp = httpPtr->GetRspContent(len);
			strRsp.append(szRsp,len);
		}
		return strRsp;
	}

	inline void EmptyCallBack(const HttpClientPtr& httpPtr,DWORD,DWORD)
	{

	}
}

#define MAKE_EMPTY_HTTP_CALLBACK(httpPtr) \
	boost::bind(&SohuTool::EmptyCallBack,httpPtr,_1,_2)

