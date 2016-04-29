#include "PingBackReport.h"
#include "../p2pcommon/get_http_file.h"
#include "local_http_server.h"

boost::shared_ptr<CPingBackReport> CPingBackReport::m_sp_pingback;
boost::once_flag CPingBackReport::s_once_flag_= BOOST_ONCE_INIT;

CPingBackReport::CPingBackReport(void)
{

}

CPingBackReport::~CPingBackReport(void)
{

}

void CPingBackReport::inner_request(const wstring &str_url)
{
	GetHttpFile::Ptr spRequest = GetHttpFile::create(LocalHttpServer::Inst()->get_io_service(K_THREAD_LISTEN));
	spRequest->get_data(boost::bind(&CPingBackReport::on_ping_back_callback, shared_from_this(), _1, _2), w2b(str_url));
}
