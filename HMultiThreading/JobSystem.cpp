#include <array>
#include <queue>

#include "JobSystem.hpp"

namespace JobSystem
{
	void JobWorkerThread(const int threadIndex);

	std::thread threads[32];
	std::array<std::queue<Description>, 32> jobs;

	bool terminated[32];
	int currentThread;
	int NumThreads;
}

void JobSystem::JobWorkerThread(const int threadIndex)
{
	while (!terminated[threadIndex])
	{
		while (!jobs[threadIndex].empty())
		{
			const Description& desc = jobs[threadIndex].front();
			void* result = desc.func(desc.param);
			if (desc.callback) desc.callback(result);
			jobs[threadIndex].pop();
		}
		std::this_thread::yield();
	}
}

void JobSystem::Initialize()
{
#define HMIN(x, y) (x > y ? x : y)
	NumThreads = HMIN(std::thread::hardware_concurrency(), 32);
	for (int i = 0; i < NumThreads; i++)
	{
		terminated[i] = false;
		threads[i] = std::thread(JobWorkerThread, i);
	}
#undef HMIN
}

void JobSystem::Terminate()
{
	for (int i = 0; i < NumThreads; i++)
	{
		terminated[i] = true;
		threads[i].detach();
	}
}

void JobSystem::PushJob(const Description& description)
{
	jobs[currentThread++ % NumThreads].push(description);
}

void JobSystem::PushJobs(int numDescs, Description description[])
{
	for (int i = 0; i < numDescs; ++i)
		jobs[currentThread++ % NumThreads].push(description[i]);
}
