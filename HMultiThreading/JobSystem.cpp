#include <array>
#include <queue>
#include <thread>

#include "JobSystem.hpp"
#include "../readerwriterqueue/readerwriterqueue.h"

namespace JobSystem
{
	void JobWorkerThread(const int threadIndex);

	std::thread threads[32];
	std::array<moodycamel::ReaderWriterQueue<Description, 1024>, 32> jobs;

	bool terminated[32];
	int currentThread;
	int NumThreads;

	int GetNumThreads() { return NumThreads; }
}

HArgsStruct JobSystem::Worker::Work(ArgsStruct args)                          
{ 															   
	const int start = JobSystem::UnpackLowerInteger(args[0]);  
	const int end   = JobSystem::UnpackHigherInteger(args[0]);    
	return ((JobSystem::Worker*)args[1])->Process(start, end, args.GetRangeArgs());
}

void JobSystem::JobWorkerThread(const int threadIndex)
{
	while (!terminated[threadIndex])
	{
		while (jobs[threadIndex].size_approx())
		{
			Description desc; 
			while (!jobs[threadIndex].try_dequeue(desc));
			
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
	while(!jobs[currentThread++ % NumThreads].try_enqueue(description));
}

void JobSystem::PushJobs(int numDescs, Description description[])
{
	for (int i = 0; i < numDescs; ++i)
		while(!jobs[currentThread++ % NumThreads].try_enqueue(description[i]));
}

template<typename RangeArgs_t>
void JobSystem::PushRangeJob(const Worker& workerClass, int numElements, RangeArgs rangeArgs)
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