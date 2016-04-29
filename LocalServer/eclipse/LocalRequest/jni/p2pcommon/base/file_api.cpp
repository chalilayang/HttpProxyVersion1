
//////////////////////////////////////////////////////////////////////
//
// FileAPI.cpp
//
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////
// include files
//////////////////////////////////////////////////

#include "file_api.h"


#ifdef WIN32
#include <io.h>			// for _open()
#include <fcntl.h>		// for _open()/_close()/_read()/_write()...
#include <string.h>		// for memcpy()
#else
#include <sys/types.h>	// for open()
#include <sys/stat.h>	// for open()
#include <unistd.h>		// for fcntl()
#include <fcntl.h>		// for fcntl()
#include <sys/ioctl.h>	// for ioctl()
#include <errno.h>		// for errno
#include <stdbool.h>
#endif


//////////////////////////////////////////////////
// external variables
//////////////////////////////////////////////////
#ifndef WIN32
#ifdef ANDROID
    extern  volatile int errno;
#else
    extern int errno;
#endif
#endif

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
int FileAPI::open_ex ( const char * filename , int flags )
{
#ifndef WIN32
	int fd = open(filename,flags);
#else
	int fd = _open(filename,flags);
#endif
	if ( fd < 0 ) {

#ifndef WIN32
		switch ( errno ) {
		case EEXIST : 
		case ENOENT  : 
		case EISDIR : 
		case EACCES : 
		case ENAMETOOLONG : 
		case ENOTDIR : 
		case ENXIO   : 
		case ENODEV  : 
		case EROFS   : 
		case ETXTBSY : 
		case EFAULT  : 
		case ELOOP   : 
		case ENOSPC  : 
		case ENOMEM  : 
		case EMFILE  : 
		case ENFILE  : 
		default :
			{
				break;
			}
		}//end of switch
#else
	// ...
#endif
	}

	return fd;	
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
int FileAPI::open_ex ( const char * filename , int flags , int mode )
{
	#ifndef WIN32
	int fd = open(filename,flags,mode);
#else
	int fd = _open(filename,flags,mode);
#endif

	if ( fd < 0 ) {
#ifndef WIN32
		switch ( errno ) {
		case EEXIST : 
		case EISDIR : 
		case EACCES : 
		case ENAMETOOLONG : 
		case ENOENT  : 
		case ENOTDIR : 
		case ENXIO   : 
		case ENODEV  : 
		case EROFS   : 
		case ETXTBSY : 
		case EFAULT  : 
		case ELOOP   : 
		case ENOSPC  : 
		case ENOMEM  : 
		case EMFILE  : 
		case ENFILE  : 
		default :
			{
				break;
			}
		}//end of switch
#else
	// ...
#endif
	}

	return fd;	
}


//////////////////////////////////////////////////////////////////////
//
// unsigned int FileAPI::read_ex ( int fd , void * buf , unsigned int len ) 
//
//
// exception version of read()
//
// Parameters 
//     fd  - file descriptor
//     buf - reading buffer
//     len - reading length
//
// Return
//     length of reading bytes
//
//
//////////////////////////////////////////////////////////////////////
unsigned int FileAPI::read_ex ( int fd , void * buf , unsigned int len ) 
{
#ifndef WIN32
	int result = read ( fd , buf , len );
#else
	int result = _read ( fd , buf , len );
#endif

	if ( result < 0 ) {

#ifndef WIN32
		switch ( errno ) {
			case EINTR : 
			case EAGAIN : 
			case EBADF : 
			case EIO : 
			case EISDIR : 
			case EINVAL : 
			case EFAULT : 
			case ECONNRESET :
			default : 
				{
					break;
				}
		}
#else
	// ...
#endif
	} else if ( result == 0 ) {
	}

	return result;	
}

//////////////////////////////////////////////////////////////////////
//
// unsigned int FileAPI::write_ex ( int fd , void * buf , unsigned int len ) 
//
//
// exception version of write()
//
// Parameters 
//     fd  - file descriptor
//     buf - writing buffer
//     len - writing length
//
// Return
//     length of reading bytes
//
//
//////////////////////////////////////////////////////////////////////
unsigned int FileAPI::write_ex ( int fd , const void * buf , unsigned int len ) 
{
#ifndef WIN32
	int result = write ( fd , buf , len );
#else
	int result = _write ( fd , buf , len );
#endif

	if ( result < 0 ) {
		
#ifndef WIN32
		switch ( errno ) {
			case EAGAIN : 
			case EINTR : 
			case EBADF : 
			case EPIPE : 
			case EINVAL: 
			case EFAULT: 
			case ENOSPC : 
			case EIO : 
			case ECONNRESET :
			default : 
				{
					break;
				}
		}
#else
	//...
#endif
	}

	return result;	
}


//////////////////////////////////////////////////////////////////////
//
// void FileAPI::close_ex ( int fd ) 
//
//
// exception version of close()
//
// Parameters
//     fd - file descriptor
//
// Return
//     none
//
//
//////////////////////////////////////////////////////////////////////
void FileAPI::close_ex ( int fd ) 
{
	if ( close(fd) < 0 ) {
#ifndef WIN32
		switch ( errno ) {
			case EBADF : 
			default :
				{
					break;
				}
		}
#else
#endif
	}	
}

//////////////////////////////////////////////////////////////////////
//
// int FileAPI::fcntl_ex ( int fd , int cmd ) 
//
//
// Parameters
//     fd  - file descriptor
//     cmd - file control command
//
// Return
//     various according to cmd
//
//
//////////////////////////////////////////////////////////////////////
int FileAPI::fcntl_ex ( int fd , int cmd ) 
{
#ifndef WIN32
	int result = fcntl ( fd , cmd );
	if ( result < 0 ) {
		switch ( errno ) {
			case EINTR : 
			case EBADF : 
			case EACCES : 
			case EAGAIN : 
			case EDEADLK : 
			case EMFILE  : 
			case ENOLCK : 
			default : 
				{
					break;
				}
		}
	}
	return result;
#else
	return 0 ;
#endif	
}

//////////////////////////////////////////////////////////////////////
//
// int FileAPI::fcntl_ex ( int fd , int cmd , long arg ) 
//
//
// Parameters
//     fd  - file descriptor
//     cmd - file control command
//     arg - command argument
//
// Return
//     various according to cmd
//
//
//////////////////////////////////////////////////////////////////////
int FileAPI::fcntl_ex ( int fd , int cmd , long arg ) 
{	
#ifndef WIN32
	int result = fcntl ( fd , cmd , arg );
	if ( result < 0 ) {
		switch ( errno ) {
			case EINTR : 
			case EINVAL : 
			case EBADF : 
			case EACCES : 
			case EAGAIN : 
			case EDEADLK : 
			case EMFILE  : 
			case ENOLCK : 
			default : 
				{
					break;
				}
		}
	}
	return result;
#else
	return 0 ;
#endif	
}


//////////////////////////////////////////////////////////////////////
//
// bool getfilenonblocking_ex ( int fd ) 
//
//
// check if this file is nonblocking mode
//
// Parameters
//     fd - file descriptor
//
// Return
//     TRUE if nonblocking, FALSE if blocking
//
//
//////////////////////////////////////////////////////////////////////
bool FileAPI::getfilenonblocking_ex ( int fd ) 
{
#ifndef WIN32
	int flags = fcntl_ex( fd , F_GETFL , 0 );
	return flags | O_NONBLOCK;
#else
	return FALSE;
#endif	
}

//////////////////////////////////////////////////////////////////////
//
// void setfilenonblocking_ex ( int fd , bool on ) 
//
//
// make this file blocking/nonblocking
//
// Parameters
//     fd - file descriptor
//     on - TRUE if nonblocking, FALSE if blocking
//
// Return
//     none
//
//
//////////////////////////////////////////////////////////////////////
void FileAPI::setfilenonblocking_ex ( int fd , bool on ) 
{
#ifndef WIN32
	int flags = fcntl_ex( fd , F_GETFL , 0 );

	if ( on )
		// make nonblocking fd
		flags |= O_NONBLOCK;
	else
		// make blocking fd
		flags &= ~O_NONBLOCK;

	fcntl_ex( fd , F_SETFL , flags );
#else
#endif	
}

//////////////////////////////////////////////////////////////////////
//
// void FileAPI::ioctl_ex ( int fd , int request , void * argp )
//
//
// exception version of ioctl()
//
// Parameters
//     fd      - file descriptor
//     request - i/o control request
//     argp    - argument
//
// Return
//     none
//
//
//////////////////////////////////////////////////////////////////////
void FileAPI::ioctl_ex ( int fd , int request , void * argp )
{	
#ifndef WIN32
	if ( ioctl(fd,request,argp) < 0 ) {
		switch ( errno ) {
		case EBADF : 
		case ENOTTY : 
		case EINVAL : 
		default :
			{
				break;
			}
		}
	}
#else
#endif	
}



//////////////////////////////////////////////////////////////////////
//
// void FileAPI::setfilenonblocking_ex2 ( int fd , bool on )
//
//
// make this stream blocking/nonblocking using ioctl_ex()
//
// Parameters
//     fd - file descriptor
//     on - TRUE if nonblocking, FALSE else
//
// Return
//     none
//
//
//////////////////////////////////////////////////////////////////////
void FileAPI::setfilenonblocking_ex2 ( int fd , bool on )
{
#ifndef WIN32
	unsigned long arg;
    if (on)
    {
        arg = 1;
    } else
    {
        arg = 0;
    }
	ioctl_ex(fd,FIONBIO,&arg);
#else
#endif
}


//////////////////////////////////////////////////////////////////////
//
// unsigned int FileAPI::available_ex ( int fd )
//
//
// how much bytes available in this stream? using ioctl_ex()
//
// Parameters
//     fd - file descriptor
//
// Return
//     #bytes available
//
//
//////////////////////////////////////////////////////////////////////
unsigned int FileAPI::availablefile_ex ( int fd )
{
#ifndef WIN32
	unsigned int arg = 0;
	ioctl_ex(fd,FIONREAD,&arg);
	return arg;
#else
	return 0;
#endif	
}


//////////////////////////////////////////////////////////////////////
//
// int FileAPI::dup_ex ( int fd )
//
//
//////////////////////////////////////////////////////////////////////
int FileAPI::dup_ex ( int fd )
{
#ifndef WIN32
	int newfd = dup(fd);
#else
	int newfd = _dup(fd);
#endif

	if ( newfd < 0 ) {
#ifndef WIN32
		switch ( errno ) {
		case EBADF : 
		case EMFILE : 
		default :
			{
				break;
			}
		}//end of switch
#else
#endif
	}

	return newfd;	
}


//////////////////////////////////////////////////////////////////////
//
// long FileAPI::lseek_ex ( int fd , long offset , int whence )
//
//
//////////////////////////////////////////////////////////////////////
long FileAPI::lseek_ex ( int fd , long offset , int whence )
{
#ifndef WIN32
	long result = lseek(fd,offset,whence);
	if ( result < 0 ) {
		switch ( errno ) {
		case EBADF : 
		case ESPIPE : 
		case EINVAL : 
		default :
			{
				break;
			}
		}
	}
#else
	long result = _lseek(fd,offset,whence);
	if ( result < 0 ) {
	}
#endif

	return result;
}

long FileAPI::tell_ex( int fd )
{
#ifndef WIN32
	long result;
#else
	long result = _tell(fd);
	if ( result < 0 ) {
	}
#endif

	return result ;	
}

