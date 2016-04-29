#ifndef _SHXMLPARSER_H__
#define _SHXMLPARSER_H__

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "../../p2pcommon/base/algorithm.h"
#include <sstream>

enum sh_xml_encode_type
{
	sh_xml_encode_type_gb2312 = 0,
	sh_xml_encode_type_utf8
};

template<int v>
class Int2Type
{
	enum{value = v};
};

typedef rapidxml::xml_node<tchar_t>			sh_xml_node;
typedef rapidxml::xml_attribute<tchar_t>	sh_xml_attribute;

template<sh_xml_encode_type encode_type>
class sh_xml_document : public rapidxml::xml_document<tchar_t>
{
public:
	bool open(const std::tstring& szPath)
	{
		std::basic_ifstream<char, char_traits<char> > xml_file(szPath.c_str());
		if (!xml_file.is_open())
		{
			return false;
		}
		string buffer((istreambuf_iterator<char>(xml_file)), istreambuf_iterator<char>());
		//buffer.push_back(0);
		if(buffer.size() == 0)
			return false;
		//如果是gb2312先换成utf-8
		string::size_type pos = buffer.find("encoding=\"GB2312\"");
		if(pos != string::npos)
		{
			buffer.replace(pos,strlen("encoding=\"GB2312\""),"encoding=\"utf-8\"");
#ifdef WIN32
			wstring wbuffer = b2w(buffer);
			buffer = Wide2Utf8(wbuffer);
#endif
			std::basic_ofstream<char, char_traits<char> > xml_file(szPath.c_str());
			if (!xml_file.is_open())
			{
				return false;
			}
			xml_file.write(buffer.data(),buffer.size());
		}
		//去掉utf8标识
		char szHead[]="\xEF\xBB\xBF\x00";
		if(buffer.size() > 3 && memcmp(&buffer[0],szHead,3) == 0)
			buffer.erase(0,3);
		return parse(&buffer[0]);
	}
	

	void save(const std::tstring& szPath)
	{
		save(szPath,Int2Type<(int)encode_type>());
	}

	/*void save(LPCTSTR szPath,Int2Type<(int)sh_xml_encode_type_gb2312>)
	{
		save_gb2312(szPath);
	}*/

	void save(const std::tstring& szPath,Int2Type<(int)sh_xml_encode_type_utf8>)
	{
		save_utf8(szPath);
	}

	/*bool parse(LPCSTR szXml,Int2Type<(int)sh_xml_encode_type_gb2312>)
	{
		return inneropen(GBKToUnicode(szXml).c_str());
	}*/

	bool parse(const std::string& szXml,Int2Type<(int)sh_xml_encode_type_utf8>)
	{
		std::tstring strContent;
#ifdef WIN32
		strContent = Utf82Wide(szXml);
#else
		strContent = szXml;
#endif
		return inneropen(strContent);
	}

	bool parse(const std::string& szXml)
	{
		try
		{
			return parse(szXml,Int2Type<(int)encode_type>());
		}
		catch (...)
		{
			return false;
		}
	}

	sh_xml_node *allocate_node(rapidxml::node_type type, tchar_t* szName = NULL, tchar_t* szValue = NULL)
	{
		tchar_t* szAllocName  = szName ? allocate_string(szName): NULL;
		tchar_t* szAllocValue = szValue ? allocate_string(szValue): NULL;
		return memory_pool<tchar_t>::allocate_node(type,szAllocName,szAllocValue);
	}
	//
	sh_xml_attribute *allocate_attribute(tchar_t* szName = NULL, tchar_t* szValue = NULL)
	{
		tchar_t* szAllocName  = szName ? allocate_string(szName): NULL;
		tchar_t* szAllocValue = szValue ? allocate_string(szValue): NULL;
		return memory_pool<tchar_t>::allocate_attribute(szAllocName,szAllocValue);
	}
	//
	sh_xml_attribute *allocate_attribute(tchar_t* szName, long value)
	{
		std::tostringstream strValue;
		strValue << value;
		tchar_t* szAllocName  = szName ? allocate_string(szName): NULL;
		tchar_t* szAllocValue = allocate_string(strValue.str().c_str());
		return memory_pool<tchar_t>::allocate_attribute(szAllocName,szAllocValue);
	}
	//
private:
	std::tstring m_strXml;
private:
	/*void save_gb2312(std::tstring szPath)
	{
		try
		{
			std::wostringstream stream;
			rapidxml::print((std::basic_ostream<wchar_t>&)stream, *this, 0);
			wstring buffer = stream.str();
			//
			string strContent;
			int len = WideCharToMultiByte(936,0,&buffer[0],buffer.size(),NULL,0,NULL,NULL);
			strContent.resize(len);
			WideCharToMultiByte(936,0,&buffer[0],buffer.size(),(char*)strContent.c_str(),len,NULL,NULL);
			//
			save(szPath,strContent.c_str());
		}
		catch (...)
		{
		}
	}*/

	void save_utf8(const std::tstring& szPath)
	{
		try
		{
			std::string strContent;
			std::tostringstream stream;
			rapidxml::print((std::basic_ostream<tchar_t>&)stream, *this, 0);
#ifdef WIN32
			strContent = Wide2Utf8(stream.str());
#else
			strContent = stream.str();
#endif
			save(szPath,strContent.c_str());
		}
		catch (...)
		{
		}
	}
	bool inneropen(const std::tstring& szXml)
	{
		try
		{
			m_strXml = szXml;
			rapidxml::xml_document<tchar_t>::parse<rapidxml::parse_full>((tchar_t*)m_strXml.c_str());
			return true;
		}
		catch (...)
		{
			return false;
		}
	}
	void save(const std::tstring& szPath,const std::string& szXml)
	{
		try
		{
			std::basic_ofstream<char, char_traits<char> > xml_file(szPath.c_str());
			if (!xml_file.is_open())
			{
				return;
			}
			xml_file<<szXml;
			xml_file.close();
		}
		catch (...)
		{
		}
	}

};

#endif

