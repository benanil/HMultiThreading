#include <array>
#include <queue>

#include "JobSystem.hpp"

namespace JobSystem
{
	void JobWorkerThread(const int threadIndex);

	class Threads
	{
	public:
		std::thread& operator[](int index)
		{
			switch (index) {
			case 0: return t1; case 1: return t2;
			case 2: return t3; case 3: return t4;
			}
			return t1;
		}
	private:
		std::thread t1 = std::thread(JobWorkerThread, 0);
		std::thread t2 = std::thread(JobWorkerThread, 1);
		std::thread t3 = std::thread(JobWorkerThread, 2);
		std::thread t4 = std::thread(JobWorkerThread, 3);
	};

	std::array<std::queue<Description>, NumThreads> jobs;
	bool terminated[NumThreads];
	int currentThread;
	Threads threads;
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
	for (int i = 0; i < NumThreads; i++) terminated[i] = false;
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
