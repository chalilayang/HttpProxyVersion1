#ifndef _IOSERVICEPOOL_H_
#define _IOSERVICEPOOL_H_

#include "../base/common.h"

class IOServicePool : private boost::noncopyable
{
public:
	explicit IOServicePool(std::size_t pool_size = 1) 
		: block_(false), next_ios_(0)
	{
		for (std::size_t i = 0; i != pool_size; ++i)
		{
			ios_ptr io_service(new boost::asio::io_service);
			ios_.push_back(io_service);
		}
	}

	~IOServicePool()
	{
		stop();

		for (std::size_t i = 0; i != ios_.size(); ++i)
		{
			ios_[i].reset();
		}
		ios_.clear();
	}

	void start()
	{
		if (threads_.size() != 0)
			return;

		// the io_service's reset
		for (std::size_t i = 0; i != ios_.size(); ++i)
		{
			ios_[i]->reset();
		}
	
		for (std::size_t i = 0; i < ios_.size(); ++i)
		{
			// Give the io_service work to do so that its run() functions will not
			// exit until work was explicitly destroyed.
			work_ptr work(new boost::asio::io_service::work(*ios_[i]));
			work_.push_back(work);

			// Create a thread to run the io_service.
			thread_ptr thread(new boost::thread(
				boost::bind(&boost::asio::io_service::run, ios_[i])));
			threads_.push_back(thread);
		}
	}

	void stop()
	{
		if (threads_.size() == 0)
			return;

		// Allow all operations and handlers to be finished normally,
		// the work object may be explicitly destroyed.

		// Destroy all work.
		for (std::size_t i = 0; i < work_.size(); ++i)
			work_[i].reset();
		work_.clear();

		for (std::size_t i = 0; i < ios_.size(); ++i)
		{
			ios_[i]->stop();
		}

		if (!block_)
			wait();
	}

	/// Get an io_service to use.
	boost::asio::io_service& get_ios()
	{
		boost::asio::io_service& io_service = *ios_[next_ios_];
		next_ios_ = (next_ios_+1)%ios_.size();
		return io_service;
	}

	/// Get the certain io_service to use.
	boost::asio::io_service& get_ios(std::size_t index)
	{
		if (index < ios_.size())
			return *ios_[index];
		else
			return get_ios();
	}

	void wait()
	{
		if (threads_.size() == 0)
			return;

		// Wait for all threads in the pool to exit.
		for (std::size_t i = 0; i < threads_.size(); ++i)
			threads_[i]->join();

		// Destroy all threads.
		threads_.clear();
	}

private:

	typedef boost::shared_ptr<boost::asio::io_service> ios_ptr;
	typedef boost::shared_ptr<boost::asio::io_service::work> work_ptr;
	typedef boost::shared_ptr<boost::thread> thread_ptr;

	/// Our io services.
	std::vector<ios_ptr> ios_;

	/// The work that keeps the io_services running.
	std::vector<work_ptr> work_;

	/// The pool of threads for running individual io_service.
	std::vector<thread_ptr> threads_;

	std::size_t next_ios_;

	/// Flag to indicate that start() functions will block or not.
	bool block_;
};

#endif
