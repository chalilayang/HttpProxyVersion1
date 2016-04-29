 #ifndef _IOBUFFER_H_
#define _IOBUFFER_H_

#include "common.h"
#include <boost/shared_array.hpp>
#include "buffer_pool.h"

class IOBuffer
{
public:
	// The type of the bytes stored in the buffer.
	typedef char byte_type;

	// The type used for offsets into the buffer.
	typedef std::size_t size_type;

	// Constructor.
	explicit IOBuffer(size_type capacity = 0)
		: begin_offset_(0),
		end_offset_(capacity),
		buffer_capacity_(capacity)
	{
        if (false && capacity <= PACKET_LEN)
        {
            if (buffer_capacity_) buffer_.reset(BufferPool::inst()->alloc(),BufferPool::buffer_deleter<byte_type *>());
        }
        else
        {
            if (buffer_capacity_) buffer_.reset(new byte_type[buffer_capacity_]);
        }

	}

    explicit IOBuffer(const std::string& str)
    {
        buffer_capacity_ = str.length();

        if (false && str.length() <= PACKET_LEN)
        {
            if (buffer_capacity_)
            {
                buffer_.reset(BufferPool::inst()->alloc(),BufferPool::buffer_deleter<byte_type *>());
                memcpy(buffer_.get(), str.c_str(), str.length());
            }
        }
        else
        {
            if (buffer_capacity_)
            {
                buffer_.reset(new byte_type[str.length()]);
                memcpy(buffer_.get(), str.c_str(), str.length());
            }
        }

        begin_offset_ = 0;
        end_offset_ = buffer_capacity_;        
    }

    IOBuffer(const byte_type *bytes, size_t bytes_len)
        : buffer_capacity_(bytes_len)
    {
        begin_offset_ = 0;
        end_offset_ = bytes_len;

        if (false && bytes_len <= PACKET_LEN)
        {
            if (buffer_capacity_)
            {
                buffer_.reset(BufferPool::inst()->alloc(),BufferPool::buffer_deleter<byte_type *>());
                memmove(&buffer_[0], bytes, bytes_len);
            }
        }
        else
        {
            if (buffer_capacity_) 
            {
                buffer_.reset(new byte_type[buffer_capacity_]);
                memmove(&buffer_[0], bytes, bytes_len);
            }
        }
    }

    IOBuffer clone() const
    {
        IOBuffer newbuf(buffer_.get(), buffer_capacity_);
        newbuf.begin_offset_ = this->begin_offset_;
        newbuf.end_offset_ = this->end_offset_;
		return newbuf;
    }

    IOBuffer data_buffer()
    {
        return IOBuffer(data(), size());
    }

	/// Clear the buffer.
	void clear()
	{
		begin_offset_ = 0;
		end_offset_ = 0;
	}

	// Return a pointer to the beginning of the unread data.
	byte_type* data()
	{
		return &buffer_[0] + begin_offset_;
	}

	// Return a pointer to the beginning of the unread data.
	const byte_type* data() const
	{
		return &buffer_[0] + begin_offset_;
	}

    boost::shared_array<byte_type> share_data()
    {
        return buffer_;
    }

	// Is there no unread data in the buffer.
	bool empty() const
	{
		return begin_offset_ == end_offset_;
	}

	// Return the amount of unread data in the buffer.
	size_type size() const
	{
		return end_offset_ - begin_offset_;
	}

	// Resize the buffer to the specified length.
	void resize(size_type length)
	{
		BOOST_ASSERT(length <= capacity());
		if (begin_offset_ + length <= capacity())
		{
			end_offset_ = begin_offset_ + length;
		}
		else
		{
			using namespace std; // For memmove.
			memmove(&buffer_[0], &buffer_[0] + begin_offset_, size());
			end_offset_ = length;
			begin_offset_ = 0;
		}
	}

	// Return the maximum size for data in the buffer.
	size_type capacity() const
	{
		return buffer_capacity_;
	}

	// Return the amount of free space in the buffer.
	size_type freespace() const
	{
		return capacity() - end_offset_;
	}

	// Consume multiple bytes from the beginning of the buffer.
	void consume(size_type count)
	{
		BOOST_ASSERT(count <= size());
		begin_offset_ += count;
		if (empty())
		{
			clear();
		}
	}

	// Produce multiple bytes to the ending of the buffer.
	void produce(size_type count)
	{
		BOOST_ASSERT(count <= freespace());
		end_offset_ += count;
	}

	// Remove consumed bytes from the beginning of the buffer.
	void align()
	{
		if (begin_offset_ != 0)
		{
			if (begin_offset_ != end_offset_)
			{
				using namespace std; // For memmove.
				memmove(&buffer_[0], &buffer_[0] + begin_offset_, size());
				end_offset_ = size();
				begin_offset_ = 0;
			}
			else
			{
				clear();
			}
		}
	}

private:
	// The offset to the beginning of the unread data.
	size_type begin_offset_;

	// The offset to the end of the unread data.
	size_type end_offset_;

	// The data in the buffer.
	boost::shared_array<byte_type> buffer_;

    size_type buffer_capacity_;

};

#endif
