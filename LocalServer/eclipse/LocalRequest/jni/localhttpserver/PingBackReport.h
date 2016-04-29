#pragma once
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/once.hpp>
#include "../p2pcommon/base/iobuffer.h"

class CPingBackReport : private boost::noncopyable, public boost::enable_shared_from_this<CPingBackReport>
{
public:
	CPingBackReport(void);
	~CPingBackReport(void);

	static boost::shared_ptr<CPingBackReport> Instance(void)
	{
		boost::call_once(CPingBackReport::Create, s_once_flag_);
		return m_sp_pingback;
	}

private:
	static void Create()
	{
		m_sp_pingback.reset(new CPingBackReport);
	}

	void inner_request(const std::wstring &str_url);

	void on_ping_back_callback(const IOBuffer& io_buf, const boost::system::error_code& error) {}


private:
	static boost::shared_ptr<CPingBackReport> m_sp_pingback;
	static boost::once_flag s_once_flag_;
};
