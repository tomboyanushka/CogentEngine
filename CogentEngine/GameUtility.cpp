#include "GameUtility.h"


template<typename T>
std::string GameUtility::GetEntityPosToString(T object)
{
	std::string result;
	result = " x:" + std::to_string(object) + " y: " + std::to_string(position.y) + " z: " + std::to_string(position.z) + "\n";
	return result;
	return std::string();
}

XMFLOAT3 GameUtility::CalculateLeftVector(XMFLOAT3 upVector, XMFLOAT3 normal)
{
	auto up = XMLoadFloat3(&upVector);
	auto forward = XMLoadFloat3(&normal);

	auto leftVector = -XMVector3Cross(up, forward);
	XMFLOAT3 left(0, 0, 0);
	XMStoreFloat3(&left, leftVector);

	return left;
}
