//Reference: https://juanchopanzacpp.wordpress.com/2013/02/26/concurrent-queue-c11/
//Thread safe queue: https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html

#pragma once
#include <queue>
#include <condition_variable>
#include <mutex>
using namespace std;

template <typename T>
class ConcurrentQueue
{
	queue<T> queue;
	mutex mtx;
	condition_variable cv;
public:

	T Pop();
	void Push(const T& item);
	bool IsEmpty();

	ConcurrentQueue() {};
	~ConcurrentQueue() {};
};

template<typename T>
T ConcurrentQueue<T>::Pop()
{
	unique_lock<mutex> lock(mtx);
	while (queue.empty())
	{
		cv.wait(lock);
	}
	//copies the item queued by popping
	auto item = queue.front();
	queue.pop();
	return item;
}

template<typename T>
void ConcurrentQueue<T>::Push(const T &item)
{
	unique_lock<mutex> lock(mtx);
	queue.push(item);
	lock.unlock();
	cv.notify_one();
}

template<typename T>
bool ConcurrentQueue<T>::IsEmpty()
{
	std::unique_lock<mutex> lock(mtx);
	return queue.empty();
}


