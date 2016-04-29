 /*后续头文件中都include这个头文件*/

#ifndef _SHP2PSYSTEMCOMMON_H_
#define _SHP2PSYSTEMCOMMON_H_

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500 // windows 2000
#endif

#ifdef WIN32
// boost中跨平台\n的警告
#pragma warning(disable:4819)
// MSVC 对于stdio函数的警告
#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdarg>
#include <ctime>

#include <deque>
#include <limits>
#include <list>
#include <memory>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/format.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
//#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/shared_array.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread.hpp>
#include <boost/logic/tribool.hpp>
#include  <boost/any.hpp>

typedef boost::int8_t int8_t;
typedef boost::int16_t int16_t;
typedef boost::int32_t int32_t;
typedef boost::int64_t int64_t;

typedef boost::uint8_t uint8_t;
typedef boost::uint16_t uint16_t;
typedef boost::uint32_t uint32_t;
typedef boost::uint64_t uint64_t;

namespace std {
#ifdef UNICODE
    #define _T(x) L##x
	typedef wstring tstring;
    typedef wostringstream tostringstream;
    #define   _ttoi _wtoi
    typedef wchar_t  tchar_t;
#else
    #define _T(x) x
	typedef string tstring;
    typedef ostringstream tostringstream;
    #define _ttoi   atoi
    typedef char  tchar_t;
#endif

}

typedef std::map<std::string, boost::any> ArgMap;

#if _MSC_VER
#define snprintf sprintf_s
#endif

using namespace std;
#endif
