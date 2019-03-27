#pragma once
#include "ThreadPool.h"
#include "IJob.h"
#include "DXCore.h"
#include <DirectXMath.h>


class MyJob : public IJob
{

public:
	// Inherited via IJob
	virtual void Execute() override;
	virtual void Callback() override;

};

class UpdatePosJob : public IJob
{
	DirectX::XMFLOAT3 pos;
	float totalTime;

public:
	// Inherited via IJob
	virtual void Execute() override;
	virtual void Callback() override;

};