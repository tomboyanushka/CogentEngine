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
	float x = sin(totalTime);
	//pos = DirectX::XMFLOAT3(3, 2, 3 * x);
}

void UpdatePosJob::Callback()
{
}
