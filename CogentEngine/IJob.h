#pragma once
#include <mutex>
class IJob
{
	std::mutex mutex;
public:
	virtual void Execute() {};
	virtual void Callback() {};
	void SetIsCompleted(bool value);
	bool IsCompleted();
	bool mIsCompleted = true;
};