#pragma once
#include "ThreadPool.h"
#include "IJob.h"


class MyJob : public IJob
{

public:
	// Inherited via IJob
	virtual void Execute() override;
	virtual void Callback() override;

};