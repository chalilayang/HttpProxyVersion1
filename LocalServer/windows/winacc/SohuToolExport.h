#pragma once

#ifdef SOHUTOOL_EXPORTS
#define SOHUTOOL_API __declspec(dllexport)
#elif defined SOHUTOOL_LIB
#define SOHUTOOL_API
#else
#define SOHUTOOL_API __declspec(dllimport)
#endif


#ifndef SOHUTOOL_EXPORTS
#pragma comment(lib,"SohuTool.lib")
#endif