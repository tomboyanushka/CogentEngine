#include "IJob.h"

void IJob::SetIsCompleted(bool value)
{
	std::unique_lock<std::mutex> lock{ mutex };
	mIsCompleted = value;
}

bool IJob::IsCompleted()
{
	std::unique_lock<std::mutex> lock{ mutex };
	return mIsCompleted;
}
