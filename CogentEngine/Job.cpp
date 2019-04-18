#include "Job.h"


void MyJob::Execute()
{
	//cout << 1;
	this_thread::sleep_for(50ms);
}

void MyJob::Callback()
{
	//cout << "Test \n";
}

void UpdatePosJob::Execute()
{
	pos = DirectX::XMFLOAT3(sin(totalTime) - 6, 0, 0);
	/*W = DirectX::XMMatrixTranslation(sin(totalTime) - 6, 0, 0);
	XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixTranspose(W));*/
}

void UpdatePosJob::Callback()
{
	//cout << "Job 2 \n";
}
