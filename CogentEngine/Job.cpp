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

}

void UpdatePosJob::Callback()
{
	//cout << "Job 2 \n";
}

void PathFinder::Execute()
{

}

void PathFinder::Callback()
{

}
