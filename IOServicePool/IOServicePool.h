#pragma once

#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <thread>
#include <vector>

#if defined(_WINDOWS) | defined(_WIN32) | defined(_WIN32)
	#ifdef IOSERVICEPOOL_EXPORTS
		#define IOSERVICEPOOL_API __declspec(dllexport)
	#else
		#pragma comment(lib,"IOServicePool.lib") 
		#define IOSERVICEPOOL_API __declspec(dllimport)
	#endif
#else
	#define IOSERVICEPOOL_API
#endif

class IOSERVICEPOOL_API IOServicePool : public boost::noncopyable
{
public:
	enum ServiceModel
	{
		Single = 0,
		Multi
	};

public:

	IOServicePool(ServiceModel In_Model);

	~IOServicePool();

	void Run(uint32_t In_ThreadCnt = std::thread::hardware_concurrency()) noexcept;

	//Blocking
	void Stop() noexcept;

	const uint32_t& GetThreadCnt() noexcept;

	const ServiceModel& GetServiceModel() noexcept;

	operator boost::asio::io_service& () noexcept;

	boost::asio::io_service& GetIOService(uint32_t In_idx) noexcept;

private:
	ServiceModel m_Model;
	uint32_t m_ThreadCnt, m_NextIO;

	struct io_worker;
	io_worker *m_IOWorkers;

#pragma warning(push)
#pragma warning( disable: 4251 )
	std::vector<std::thread *> m_Threads;
#pragma warning( pop )
};

