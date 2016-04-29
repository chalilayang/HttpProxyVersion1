#include "buffer_pool.h"

boost::shared_ptr<BufferPool> BufferPool::pinst_;
boost::once_flag BufferPool::once_flag_ = BOOST_ONCE_INIT;
