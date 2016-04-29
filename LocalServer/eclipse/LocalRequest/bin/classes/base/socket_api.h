//
//文件名称：	SocketAPI.h
//功能描述：	对网络socket进行一个简单的封装操作，针对不同的操作系统
//				实现相同接口的调用
//修改情况：	2005-03-22 Ver 1.0.0 完成基本功能
//				
//
//
#ifndef __SOCKET_API_H__
#define __SOCKET_API_H__


#ifdef WIN32      
#include <WinSock.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#endif

#define _ESIZE 256

//////////////////////////////////////////////////
//
// 
// 
// 
//
//////////////////////////////////////////////////
#ifndef WIN32

typedef		int		SOCKET;
#define     INVALID_SOCKET   -1
#define		SOCKET_ERROR	 -1

#endif

static const int SOCKET_ERROR_WOULDBLOCK = -100;

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
static const unsigned int szSOCKADDR_IN = sizeof(SOCKADDR_IN);

//////////////////////////////////////////////////////////////////////
//
// Platform Independent Socket API Collection (exception based)
//
//////////////////////////////////////////////////////////////////////
namespace SocketAPI 
{

	//
	// exception version of socket ()
	//
	SOCKET socket_ex (int domain, int type, int protocol) ;


	//
	// exception version of bind ()
	//
	bool bind_ex (SOCKET s, const struct sockaddr* name, unsigned int namelen) ;


	//
	// exception version of connect ()
	//
	bool connect_ex (SOCKET s, const struct sockaddr* name, unsigned int namelen) ;

	//
	// exception version of listen ()
	//
	bool listen_ex (SOCKET s, unsigned int backlog) ;


	//
	// exception version of accept ()
	//
	SOCKET accept_ex (SOCKET s, struct sockaddr* addr, unsigned int* addrlen) ;


	//
	// exception version of getsockopt ()
	//
	bool getsockopt_ex (SOCKET s, int level, int optname, void* optval, unsigned int* optlen) ;

	unsigned int getsockopt_ex2 (SOCKET s, int level, int optname, void* optval, unsigned int* optlen) ;

	//
	// exception version of setsockopt ()
	//
	bool setsockopt_ex (SOCKET s, int level, int optname, const void* optval, unsigned int optlen) ;

	//
	// exception version of send()
	//
	unsigned int send_ex (SOCKET s, const void* buf, unsigned int len, unsigned int flags) ;


	//
	// exception version of sendto()
	//
	unsigned int sendto_ex (SOCKET s, const void* buf, int len, unsigned int flags, const struct sockaddr* to, int tolen) ;

	//
	// exception version of recv()
	//
	unsigned int recv_ex (SOCKET s, void* buf, unsigned int len, unsigned int flags) ;


	//
	// exception version of recvfrom()
	//
	unsigned int recvfrom_ex (SOCKET s, void* buf, int len, unsigned int flags, struct sockaddr* from, unsigned int* fromlen) ;
	 

	//
	// exception version of closesocket() 
	//
	// *CAUTION*
	//
	// in UNIX, close() used instead
	//
	bool closesocket_ex (SOCKET s) ;


	//
	// exception version of ioctlsocket()
	//
	// *CAUTION*
	//
	// in UNIX, ioctl() used instead
	//
	bool ioctlsocket_ex (SOCKET s, long cmd, unsigned long* argp) ;


	//
	// check if socket is nonblocking mode
	//
	bool getsocketnonblocking_ex (SOCKET s) ;


	//
	// make socket nonblocking mode
	//
	bool setsocketnonblocking_ex (SOCKET s, bool on) ;


	//
	// get amount of data in socket input buffer
	//
	unsigned int availablesocket_ex (SOCKET s) ;


	//
	// exception version of shutdown()
	//
	bool shutdown_ex (SOCKET s, unsigned int how) ;


	//
	// exception version of select()
	//
	int select_ex (int maxfdp1, fd_set* readset, fd_set* writeset, fd_set* exceptset, struct timeval* timeout) ;

    //
    //get local ip
    //
    int get_host_localip();

};//end of namespace 



#endif
