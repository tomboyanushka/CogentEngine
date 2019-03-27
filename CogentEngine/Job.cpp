#include "Job.h"


void MyJob::Execute()
{
	cout << 1;
	this_thread::sleep_for(50ms);
}

void MyJob::Callback()
{
	cout << "Test \n";
}

void UpdatePosJob::Execute()
{
	W = DirectX::XMMatrixTranslation(sin(totalTime) - 6, 0, 0);
	XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixTranspose(W));
	//pos = DirectX::XMFLOAT3(3, 2, 3 * x);
}

void UpdatePosJob::Callback()
{
	cout << "Job 2 \n";
}
