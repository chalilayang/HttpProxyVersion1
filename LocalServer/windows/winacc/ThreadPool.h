#pragma once
#include "SohuToolExport.h"
#include "Singleton.h"
#include <list>
#include <vector>
#include <atlcore.h>

namespace SohuTool
{
	typedef boost::signals2::signal<void(void)> ThreadPoolSignal;
	inline void  AsyncExecute(const ThreadPoolSignal::slot_type slot);
	//
	class SOHUTOOL_API CThreadPool : public SingletonImpl<CThreadPool>
	{
		friend void  AsyncExecute(const ThreadPoolSignal::slot_type slot);
	public:
		bool Init(int nThreadNum = 0);
		void UnInit();
		void Stop();
		long GetIdelThreadNum()const;
	private:
		void   AsyncExecute(LPVOID pSignal);
		void*  GetSignal();
		void   ReleaseSignal(LPVOID pSignal);
		void   SignalConnect(LPVOID pSignal,LPVOID pSlot);
	private:
		static	DWORD WINAPI  ThreadProc(LPVOID param);
		void InnerThreadProc();
	private:
#pragma warning(push)
#pragma warning(disable:4251)
		ATL::CComAutoCriticalSection		  m_sec;
		std::list<ThreadPoolSignal* >		  m_signallist;
		std::vector<HANDLE>					  m_threads;
#pragma warning(pop)
		HANDLE								  m_workEvent;
		HANDLE								  m_endEvent;
		bool								  m_bEnd;
		long								  m_idelThreadNum;
	};
	//“Ï≤Ω÷¥––
	inline void  AsyncExecute(const ThreadPoolSignal::slot_type slot)
	{
		ThreadPoolSignal* pSignal = static_cast<ThreadPoolSignal*>(CThreadPool::Instance().GetSignal());
		if(pSignal)
		{
			CThreadPool::Instance().SignalConnect(pSignal,(LPVOID)&slot);
			CThreadPool::Instance().AsyncExecute(pSignal);
		}
	}

}
