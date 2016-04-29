#ifndef _HTTPHEADER_H_
#define _HTTPHEADER_H_

#include "./base/common.h"

struct Header
{
	Header() {}

	Header(const std::string & i_name, const std::string & i_value)
		: name(i_name), val(i_value) {}

	std::string name;
	std::string val;
};

struct HeaderEqualTo : public std::unary_function<Header, bool>
{
    HeaderEqualTo(const std::string& i_name) : name(i_name) {}

    bool operator()(const Header& header)
    {
        return name == header.name;
    }

    std::string name;
};

typedef std::list<Header> HeaderFields;

struct HeaderAppender
{
    HeaderAppender(std::string & i_headers)
        : headers(i_headers)
    { /* no-op */ }

    void operator()(const Header & header)
    {
        boost::format fmt("%1%: %2%\r\n");
        
        fmt % header.name % header.val;

        headers += fmt.str();

        //return *this;
    }

    std::string& headers;
};

#endif

