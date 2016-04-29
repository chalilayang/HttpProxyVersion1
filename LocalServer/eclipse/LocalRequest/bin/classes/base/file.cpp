#include "file.h"

namespace SH_filesystem
{
	bool create_file(const tstring& path)
	{
		if(path.empty()) return false;
		FILE* h;
#ifdef UNICODE
		h=_wfopen(path.c_str(),_T("a+"));
#else
		h=fopen(path.c_str(),_T("a+"));
#endif
		fclose(h);
		return true;
	}

	bool remove_file(const tstring& path)
	{
		if(path.empty()) return false;
#ifdef UNICODE
		if(_wremove(path.c_str())!=0 )
#else
		if(remove(path.c_str()) !=0 )
#endif
		{
			ERROR_LOG("kernel",_T("remove file fail,path is %s\n"),path.c_str());
			return false;
		}
		return true;
	}

	bool file_exist(const tstring& path)
	{
		if(path.empty()) return false;
#ifdef UNICODE
		if(_waccess(path.c_str(),0)==-1)
#else	
		if(access(path.c_str(),0)==-1)
#endif
		{
			ERROR_LOG("kernel",_T("file not exist,path is %s"),path.c_str());
			return false;
		}
		return true;
	}

	bool is_dir(const tstring& path)
	{
		if(path.empty()) return false;
#ifdef WIN32
#ifdef UNICODE
		boost::filesystem::wpath boost_path(path);
#else
		boost::filesystem::path boost_path(path);
#endif
		return boost::filesystem::is_directory(boost_path);
#else//UNIX
		struct stat file_stat;
		::stat(path.c_str(),&file_stat);
		return S_ISDIR(file_stat.st_mode);
#endif
	}

	uint64_t file_size(const tstring& path)
	{
		if(path.empty()) return 0;
#ifdef WIN32
#ifdef UNICODE
		boost::filesystem::wpath boost_path(path.c_str());
#else
		boost::filesystem::path boost_path(path.c_str());
#endif

		return (uint64_t)boost::filesystem::file_size(boost_path);
#else//UNIX
		struct stat file_stat;  
		stat(path.c_str(),&file_stat);
		return file_stat.st_size;
#endif
	}

	bool dir_exist(const tstring& path)
	{
		if(path.empty()) return false;
#ifdef WIN32
#ifdef UNICODE
		boost::filesystem::wpath boost_path(path);
#else
		boost::filesystem::path boost_path(path);
#endif
		if (!boost::filesystem::exists(boost_path))
		{
			return false;
		}
		
#else//UNIX
		DIR * d=opendir(path.c_str());
		if(NULL==d)
		{
			return false;
		}
		closedir(d);
#endif
		
		return true;
	}

	bool create_dir(const tstring& path)
	{
		if(path.empty()) return false;
#ifdef WIN32
#ifdef UNICODE
		boost::filesystem::wpath boost_path(path);
#else
		boost::filesystem::path boost_path(path);
#endif
		if(!boost::filesystem::create_directories(boost_path))
		{
			return false;
		}
#else//UNIX
		if(mkdir(path.c_str(),0755)!=0)
		{
			return false;
		}
#endif
		return true;
	}

	uint64_t dir_space(const tstring& path)
	{
		if(path.empty()) return 0;
#ifdef WIN32
#ifdef UNICODE
		boost::filesystem::wpath boost_path(path);
#else
		boost::filesystem::path boost_path(path);
#endif
		struct boost::filesystem::space_info space=boost::filesystem::space(boost_path);
		return space.available;
#else//UNIX
#ifdef ANDROID
        struct statfs file_stat;
        if (statfs(path.c_str(), &file_stat)==0)
#else
        struct statvfs file_stat;
        if(statvfs(path.c_str(), &file_stat)==0)//success
#endif
        {
			return file_stat.f_bsize*file_stat.f_bavail;
        }
        else
        {
            return -1;
        }
#endif
		
	}

	std::time_t file_last_write_time(const tstring& path)
	{
#ifdef WIN32
		return boost::filesystem::last_write_time(path);
#else//UNIX
		struct stat file_stat;  
		stat(path.c_str(),&file_stat);
		return file_stat.st_mtime;
#endif
	}

	vector<tstring> iterate_dir(const tstring& path)
	{
		vector<tstring> ret;
#ifdef WIN32
#ifdef UNICODE
		boost::filesystem::wdirectory_iterator end_iter;  
		boost::filesystem::wdirectory_iterator dir_itr(path);
#else
		boost::filesystem::directory_iterator end_iter;  
		boost::filesystem::directory_iterator dir_itr(path);
#endif
		for(;dir_itr != end_iter;++dir_itr)
		{ 
			if (!boost::filesystem::is_directory( *dir_itr ) ) //只对文件处理
			{
				ret.push_back(dir_itr->leaf());
			}
		}
#else//UNIX
		DIR *db;              /*保存 打开目录类型文件信息的 结构体*/
		struct dirent *dir_info;        /*保存 目录类型文件属性信息的 结构体*/
		char buf[FILE_NAME];        /*文件名*/
		tstring filename;
		memset(buf,0,FILE_NAME);
		db=opendir(path.c_str());
		if(db==NULL){
			ERROR_LOG("kernel",_T("open dir fail,path is %s"),path.c_str());
			return ret;
		}
		while ((dir_info=readdir(db)))
		{
			if((strcmp(dir_info->d_name,".")==0)||(strcmp(dir_info->d_name,"..")==0))
				continue;
			else
			{
				sprintf(buf,"%s/%s",path.c_str(),dir_info->d_name);
				if(!is_dir(buf))
				{
					filename.assign(dir_info->d_name);
					ret.push_back(filename);
				}
				memset(buf,0,128);
			}
		}
#endif		
		return ret;
	}
}