#pragma once
#include "ThreadPool.h"
#include "IJob.h"
#include "DXCore.h"
#include <DirectXMath.h>
#include "AStar.h"


class MyJob : public IJob
{

public:
	// Inherited via IJob
	virtual void Execute() override;
	virtual void Callback() override;

};

class UpdatePosJob : public IJob
{


public:
	DirectX::XMFLOAT3 pos;
	DirectX::XMMATRIX W;
	DirectX::XMFLOAT4X4 worldMatrix;
	float totalTime;
	// Inherited via IJob
	virtual void Execute() override;
	virtual void Callback() override;

};

class PathFinder : public IJob
{
public:

	DirectX::XMFLOAT3 currentPos;
	DirectX::XMFLOAT3 targetPos;
	AStar::CoordinateList path;
	AStar::Generator* generator;

	// Inherited via IJob
	virtual void Execute() override;
	virtual void Callback() override;

};