#ifndef _BUFFERPOOL_H_
#define _BUFFERPOOL_H_

#include "common.h"
#include <boost/thread/once.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/unordered_map.hpp>

const int PACKET_LEN = 1500;

class BufferPool
        : private boost::noncopyable
{
public:
    typedef boost::shared_ptr<BufferPool> p;
    static boost::shared_ptr<BufferPool> inst()
    {
        boost::call_once(BufferPool::init_it, once_flag_);
        return pinst_;
    }
    ~BufferPool()
    {   
        for (boost::unordered_map<char *, bool>::iterator iter = buffer_map_.begin(); 
            iter != buffer_map_.end(); iter++)
        {
            if ((*iter).second == true)
            {
                delete []((*iter).first);
            }
        }
        buffer_map_.clear();
        free_buffer_.clear();
    }  

public:
    char *alloc()
    {   
        boost::mutex::scoped_lock lock(allocate_mutex_);

        char *buffer = NULL;
        if(free_buffer_.empty())
        {   
            buffer = new char[PACKET_LEN];
            buffer_map_[buffer] = true;
        }   
        else
        {   
            buffer = free_buffer_.front();
            free_buffer_.pop_front();
            buffer_map_[buffer] = true;
        }   
        return buffer; 
    }

    void free(char *buffer)
    {
        boost::mutex::scoped_lock lock(allocate_mutex_);
        buffer_map_[buffer] = false;
        free_buffer_.push_back(buffer);
    }
    void reduce_space()
    {
		boost::mutex::scoped_lock lock(allocate_mutex_);
        for (std::deque<char *>::iterator iter = free_buffer_.begin(); 
            iter != free_buffer_.end(); iter++)
        {
            buffer_map_.erase(*iter);
            delete [](*iter);
        }
        free_buffer_.clear();
    }

    void get_pool_info(std::pair<std::size_t,std::size_t> &info)
    {
        boost::mutex::scoped_lock lock(allocate_mutex_);
        info.first = buffer_map_.size();
        info.second = free_buffer_.size();
    }
    template< typename T > struct buffer_deleter
    {   
        void operator ()( T const  p)  
        {   
            BufferPool::inst()->free(p); 
        }   
    };  
private:
    BufferPool()
    {

    }
    static void init_it()
    {
        pinst_.reset(new BufferPool);
    }
private:
    boost::mutex    allocate_mutex_;
    
    boost::unordered_map<char *, bool> buffer_map_;
    std::deque<char *>  free_buffer_;

    static boost::shared_ptr<BufferPool> pinst_;
    static boost::once_flag once_flag_;
};

#endif
