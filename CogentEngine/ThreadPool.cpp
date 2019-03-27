#include "ThreadPool.h"


ThreadPool::ThreadPool(size_t numberOfThreads)
{
	Start(numberOfThreads);
}

ThreadPool::~ThreadPool()
{
	Stop();
}

void ThreadPool::Start(size_t numberOfThreads)
{
	for (auto i = 0; i < numberOfThreads; ++i)
	{

		threads.emplace_back([&]
		{
			while (true)
			{
				Task task;
				{
					unique_lock<mutex> lock{ mtx };
					cv.wait(lock, [=] { return isStopped || !taskQueue.empty(); });

					if (isStopped && taskQueue.empty())
					{
						break;
					}

					task = move(taskQueue.front());
					taskQueue.pop();
				}
				task();
			}
		});
	}
}

void ThreadPool::Stop() noexcept
{
	{
		unique_lock<mutex> lock{ mtx };
		isStopped = true;
	}

	cv.notify_all();

	for (auto &thread : threads)
	{
		thread.join();
	}
}

void ThreadPool::ExecuteCallbacks()
{
	while (!CallbackQueue.IsEmpty())
	{
		auto popped = CallbackQueue.Pop();
		popped->Callback();
	}
}
