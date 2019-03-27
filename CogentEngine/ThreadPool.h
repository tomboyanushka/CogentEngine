#pragma once
#include <functional>
#include <vector>
#include <iostream>
#include <future>
#include <thread> 
#include <condition_variable>
#include <mutex>
#include <queue>
#include "IJob.h"
#include "ConcurrentQueue.h"

using namespace std;

class ThreadPool
{
public:
	explicit ThreadPool(size_t numberOfThreads);
	~ThreadPool();

	using Task = function<void()>;

	auto Enqueue(IJob* task)-> future<void>
	{
		task->SetIsCompleted(false);
		auto wrapper = make_shared<packaged_task<void()>>([task] { task->Execute(); });

		{
			unique_lock<mutex> lock{ mtx };
			taskQueue.emplace([=]
			{
				(*wrapper)();
				task->SetIsCompleted(true);
				CallbackQueue.Push(task);
			});
		}
		cv.notify_one();
		return wrapper->get_future();
	}

	void ExecuteCallbacks();
private:
	vector<thread> threads;
	condition_variable cv;
	mutex mtx;
	bool isStopped = false;
	queue<Task> taskQueue;
	ConcurrentQueue<IJob*> CallbackQueue;

	void Start(size_t numberOfThreads);

	void Stop() noexcept;



};