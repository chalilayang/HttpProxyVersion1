// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LOCALSERVER_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LOCALSERVER_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
#ifdef LOCALREQUEST_EXPORTS
#define LOCALSERVER_API __declspec(dllexport)
#else
#define LOCALSERVER_API __declspec(dllimport)
#endif
#define LOCALSERVER_CALL __stdcall
#define LOCALSERVER_CALLBACK __stdcall

#else
#define LOCALSERVER_API
#define LOCALSERVER_CALL
#define LOCALSERVER_CALLBACK
#endif
