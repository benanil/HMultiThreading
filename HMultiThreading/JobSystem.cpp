#include <array>
#include <queue>
#include <thread>

#include "JobSystem.hpp"

namespace JobSystem
{
	void JobWorkerThread(const int threadIndex);

	std::thread threads[32];
	std::array<std::queue<Description>, 32> jobs;

	bool terminated[32];
	int currentThread;
	int NumThreads;

	int GetNumThreads() { return NumThreads; }
}

ArgsStruct JobSystem::Worker::Work(ArgsStruct args)                          
{ 															   
	const int start = JobSystem::UnpackLowerInteger(args[0]);  
	const int end = JobSystem::UnpackUpperInteger(args[0]);    
	return (WorkerClass*)(args[1])->Process(start, end, args.GetRangeArgs()); 	   
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
	NumThreads = HMIN(std::thread::hardware_concurrency(), 32);
	for (int i = 0; i < NumThreads; i++)
	{
		terminated[i] = false;
		threads[i] = std::thread(JobWorkerThread, i);
	}
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

template<typename WorkerClass, typename RangeArgs_t = RangeArgs>
inline void JobSystem::PushRangeJob(const WorkerClass& workerClass, int numElements, RangeArgs rangeArgs = nullptr)
{
	// numelements must be greater than 0
	assert(numElements < 1);

	int numItemPerThread[MaxThreads] = { 0 };
	int itemPerThread = HMAX(numElements / GetNumThreads(), 1);
	int currThread = 0;

	while (numElements > 0)
	{
		while ((numElements - itemPerThread) >= 0)
		{
			numItemPerThread[currThread++ % NumThreads] = itemPerThread;
			numElements -= itemPerThread;
		}	
		itemPerThread = HMAX(itemPerThread / NumThreads, numElements);
	}

	currThread = 0;
	int currElement = 0;

	while (numItemPerThread[currThread])
	{
		currElement += numItemPerThread[currThread];
		
		PushJob(WorkerClass::Work, ArgsBuilder::Build()
				    .AddRange(currElement, numItemPerThread[currThread]curr)
					.AddPointer(&workerClass)
					.Add(rangeArgs).Create()
				);
	}
}