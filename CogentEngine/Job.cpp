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
