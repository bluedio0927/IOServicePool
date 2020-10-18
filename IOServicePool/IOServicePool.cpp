#include "IOServicePool.h"

#ifdef _WINDOWS
#include <Windows.h>
#endif

struct IOServicePool::io_worker
{
	boost::asio::io_service  ios;
	boost::asio::io_service::work *worker;

	io_worker()
		:worker(new boost::asio::io_service::work(ios))
	{
	}

	void stop() noexcept
	{
		if (worker)
		{
			delete worker;
			worker = nullptr;
			ios.stop();
		}
	}
};

IOServicePool::IOServicePool(ServiceModel In_Model)
	:m_Model(In_Model), m_ThreadCnt(0), m_NextIO(0), m_IOWorkers(nullptr)
{
}

IOServicePool::~IOServicePool()
{
	Stop();
}

void IOServicePool::Run(uint32_t In_ThreadCnt) noexcept
{
	Stop();
	if (In_ThreadCnt == 0)
		m_ThreadCnt = std::thread::hardware_concurrency();
	else
		m_ThreadCnt = In_ThreadCnt;

	m_Threads.resize(m_ThreadCnt);

	if (m_Model == Single)
	{
		m_IOWorkers = new io_worker();
		for (uint32_t i = 0; i < m_ThreadCnt; ++i)
			m_Threads[i] = new std::thread([this] {m_IOWorkers->ios.run(); });
	}
	else
	{
		m_IOWorkers = new io_worker[m_ThreadCnt];
		for (uint32_t i = 0; i < m_ThreadCnt; ++i)
			m_Threads[i] = new std::thread([this, i] {m_IOWorkers[i].ios.run(); });
	}

#ifdef _WINDOWS
	if (m_ThreadCnt == std::thread::hardware_concurrency())
	{
		for (uint32_t i = 0; i < m_ThreadCnt; ++i)
			SetThreadAffinityMask(m_Threads[i]->native_handle(), 1 << i);
	}
#endif
}

void IOServicePool::Stop() noexcept
{
	if (m_Threads.empty())
		return;

	if (m_Model == Single)
	{
		m_IOWorkers->stop();
		delete m_IOWorkers;
	}
	else
	{
		for (uint32_t i = 0; i < m_ThreadCnt; ++i)
			m_IOWorkers[i].stop();
		delete[]m_IOWorkers;
	}

	for (auto &iter : m_Threads)
	{
		iter->join();
		delete iter;
	}

	m_Threads.clear();
	m_ThreadCnt = 0;
	m_NextIO = 0;
}

const uint32_t & IOServicePool::GetThreadCnt() noexcept
{
	return m_ThreadCnt;
}

const IOServicePool::ServiceModel & IOServicePool::GetServiceModel() noexcept
{
	return m_Model;
}

IOServicePool::operator boost::asio::io_service&() noexcept
{
	if (m_Model == Single)
		return m_IOWorkers->ios;
	else
		return m_IOWorkers[++m_NextIO % m_ThreadCnt].ios;
}

boost::asio::io_service & IOServicePool::GetIOService(uint32_t In_idx) noexcept
{
	if (m_Model == Single)
		return	m_IOWorkers->ios;
	else
		return m_IOWorkers[In_idx].ios;
}
