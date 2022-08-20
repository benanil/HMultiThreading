
#include "JobSystem.hpp"
#include "../readerwriterqueue/readerwriterqueue.h"

#include <cassert>
#include <thread>

extern "C" {
	__declspec(dllimport) ulong __stdcall SetThreadAffinityMask(void* thread, ulong mask);
	__declspec(dllimport) int   __stdcall SetThreadPriority(void* thread, int priority);
	__declspec(dllimport) long  __stdcall SetThreadDescription(void* thread, const wchar_t* description);
}

namespace JobSystem
{
	void JobWorkerThread(const int threadIndex);

	moodycamel::ReaderWriterQueue<Description, 64>* jobs[MaxThreads];

	bool terminated[32];
	int currentThread = 0;
	int NumThreads;

	int GetNumThreads() { return NumThreads; }
}

HArgsStruct JobSystem::Worker::Work(ArgsStruct args)                          
{ 															   
	HArgsConsumer consumer(args);
	int begin = consumer.GetInt(), end = consumer.GetInt();
	return ((JobSystem::Worker*)consumer.GetPointer())->Process(begin, end, args.GetRangeArgs());
}

void JobSystem::JobWorkerThread(const int threadIndex)
{
	while (!terminated[threadIndex])
	{
		while (jobs[threadIndex]->size_approx())
		{
			Description desc; 
			while (!jobs[threadIndex]->try_dequeue(desc));
			
			ArgsStruct result = desc.func(desc.param);
			if (desc.callback) desc.callback(result);
		}
		std::this_thread::yield();
	}
}

void JobSystem::Initialize()
{
	NumThreads = hclamp((uint)std::thread::hardware_concurrency(), 4u, (uint)MaxThreads);
	for (int threadID = 0; threadID < NumThreads; threadID++)
	{
		terminated[threadID] = false;
		std::thread worker = std::thread(JobWorkerThread, threadID);
		jobs[threadID] = new moodycamel::ReaderWriterQueue<HJobDesc, 64>();
		
		{
			// https://github.com/turanszkij/JobSystem/blob/master/JobSystem.cpp#L111
			// Do Windows-specific thread setup:
			void* handle = (void*)worker.native_handle();
		
			// Put each thread on to dedicated core
			ulong affinityMask = 1ull << threadID;
			ulong affinity_result = SetThreadAffinityMask(handle, affinityMask);
			assert(affinity_result > 0);
		
			//// Increase thread priority:
			constexpr int THREAD_PRIORITY_HIGHEST = 2;
			bool priority_result = SetThreadPriority(handle, THREAD_PRIORITY_HIGHEST);
			assert(priority_result != 0);
		
			// Name the thread:
			std::wstringstream wss;
			wss << "JobSystem_" << threadID;
			long hr = SetThreadDescription(handle, wss.str().c_str());
			assert(hr >= 0);
			worker.detach(); // forget about this thread, let it do it's job in the infinite loop that we created above
		}
	}
}

void JobSystem::Terminate()
{
	for (int i = 0; i < NumThreads; i++)
	{
		terminated[i] = true;
	}
}

void JobSystem::PushJob(const Description& description)
{
	while(!jobs[currentThread++ % NumThreads]->try_enqueue(description));
}

void JobSystem::PushJobs(int numDescs, Description description[])
{
	for (int i = 0; i < numDescs; ++i)
		while(!jobs[currentThread++ % NumThreads]->try_enqueue(description[i]));
}


void JobSystem::PushRangeJob(const JobSystem::Worker& workerClass, int numElements, RangeArgs rangeArgs)
{
	// numelements must be greater than 0
	assert(numElements > 0);

	int numItemPerThread[MaxThreads] = { 0 };
	int itemPerThread = hmax(numElements / NumThreads, 1);
	int currThread = 0;

	while (numElements > 0)
	{
		while ((numElements - itemPerThread) >= 0)
		{
			numItemPerThread[currThread++ % NumThreads] += itemPerThread;
			numElements -= itemPerThread;
		}	
		itemPerThread = hmax(itemPerThread / NumThreads, numElements);
	}

	currThread = 0;
	int currElement = 0;

	while (currThread < NumThreads)
	{
		int begin = currElement;
		currElement += numItemPerThread[currThread];

		Description desc = Description(&JobSystem::Worker::Work, nullptr,
			HArgsBuilder()
			.AddRange(begin, currElement)
			.AddPointer((void*)&workerClass)
			.Add(rangeArgs).Create()
		);
		
		while (!jobs[currThread]->try_enqueue(desc));
		currThread++;
	}
}
