/*
create our own filesystem instead of boost::filesystem 
author: Lin Yu
*/
#ifndef MY_FILESYSTEM_H
#define MY_FILESYSTEM_H
#include "common.h"
#include <stdio.h>
#include "../log/log.h"
#ifdef WIN32	
	#include <boost/filesystem.hpp>
#else //unix
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <cstddef>
#ifdef ANDROID
    #include <sys/statfs.h>
#else
    #include <sys/statvfs.h>
#endif
#include <unistd.h>
#define FILE_NAME 128
#endif

namespace SH_filesystem
{
/*file operation*/
	bool create_file(const tstring& path);
	bool remove_file(const tstring& path);
	bool file_exist(const tstring& path);
	uint64_t file_size(const tstring& path);
	std::time_t file_last_write_time(const tstring& path);
/*directory operation*/
	bool is_dir(const tstring& path);
	bool dir_exist(const tstring& path);
	bool create_dir(const tstring& path);
	uint64_t dir_space(const tstring& path);
    vector<tstring> iterate_dir(const tstring& path);
};
#endif