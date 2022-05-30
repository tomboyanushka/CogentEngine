#pragma once
#include <string>
#include "Entity.h"
#include <DirectXMath.h>

class GameUtility
{

public:
	template<typename T>
	std::string GetEntityPosToString(T object);
	XMFLOAT3 CalculateLeftVector(XMFLOAT3 UpVector, XMFLOAT3 normal);

};


