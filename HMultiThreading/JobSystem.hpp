#pragma once

#include <thread>

namespace JobSystem
{
	typedef void* (*JobFunction)(void* args);
	typedef void  (*JobCallback)(void* args);

	struct Description
	{
		JobFunction func;
		JobCallback callback;
		void* param;
		Description(JobFunction _func, JobCallback _callback = nullptr, void* _param = nullptr) :
			func(_func), callback(_callback), param(_param)
		{}
	};

	void Initialize();

	void Terminate();

	void PushJob(const Description& description);

	void PushJobs(int numDescs, Description description[]);
}

typedef JobSystem::Description JobDesc;
