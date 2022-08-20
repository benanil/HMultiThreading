#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <condition_variable>
#include <queue>
#include <array>
#include <functional>
#include "JobSystem.hpp"
#include <cmath>
#include <semaphore>
#include <immintrin.h>
#include <chrono>

struct Timer
{
	std::chrono::time_point<std::chrono::high_resolution_clock> start_point;

	const char* message;

	Timer(const char* _message) : message(_message)
	{
		start_point = std::chrono::high_resolution_clock::now();
	}

	~Timer()
	{
		using namespace std::chrono;
		auto end_point = high_resolution_clock::now();
		auto start = time_point_cast<microseconds>(start_point).time_since_epoch().count();
		auto end = time_point_cast<microseconds>(end_point).time_since_epoch().count();
		auto _duration = end - start;
		float ms = _duration * 0.001;
		std::cout << message << ms << "ms" << std::endl;
	}
};

class Semaphore
{
public:
	explicit Semaphore(int _count) : count(_count) {}

	inline void P() { Take(); }
	inline void V() { Give(); }
	inline void Signal() { Take(); }
	inline void Wait() { Give(); }

	void Take()
	{
		std::unique_lock<std::mutex> lk(mutex);
		count++;
		cv.notify_one();
	}

	void Give()
	{
		std::unique_lock<std::mutex> lk(mutex);
		cv.wait(lk, [&]() { return count > 0; });
		count--;
	}
	int count;
private:
	std::condition_variable cv;
	mutable std::mutex mutex;
};

using uint = unsigned;

int t1Result[3];

std::atomic<int> counter{};

__m128i sseValues[12][8];
__m128  sseValuesf[12][8];

int   values [12][32];
float valuesf[12][32];


HJOB_ENTRY(Job1)
{
	int i = (int)args.p1;

	for (int s = 0; s < 512; s++)
	{
		for (int x = 0; x < 32; x++)
		{
			int& v = values[i][x];
			v = v + 128 * 33 + pow(v, 66) - (256 << (i % 36));

			float& vf = valuesf[i][x];
			vf += sin(sqrtf(vf * vf)) - sin(sqrtf(vf * vf + 0.01f));
		}
	}

	counter++;
	return (void*)1;
}

void CalculateAll()
{
	for (int i = 0; i < 12; i++)
	{
		for (int s = 0; s < 512; s++)
		{
			for (int x = 0; x < 32; x++)
			{
				int& v = values[i][x];
				v = v + 128 * 33 + pow(v, 66) - (256 << (i % 32));

				float& vf = valuesf[i][x];
				vf += sin(sqrtf(vf * vf)) - sin(sqrtf(vf * vf + 0.01f));
			}
		}
	}
}

HJOB_CALLBACK(JobCallback)
{
	std::cout << std::to_string((int)(void*)args);
}

#define HMultiThreadingTest

#ifdef HMultiThreadingTest

// instead of single for loop you can optimize your code with multiple cores
struct ZWorker final : JobSystem::Worker
{
	int data[12 * 41] = { 0 };

	HArgsStruct Process(int start, int end, JobSystem::RangeArgs rangeArgs) override
	{
		const int adition = HRangeArgsConsumer(rangeArgs).GetInt();
		for (int i = start; i < end; ++i)
		{
			data[i] += adition*10;
		}
		// thread's job done, do whatever you want after this line, 
		// for example you can reduce/increase atomic counter
		return nullptr;
	}
};

int main()	
{
	using namespace std::chrono_literals;
	
	constexpr bool MultiThreading = true;
	
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 32; j++) {
			values[i][j] = 33;
			valuesf[i][j] = 33.0f;
		}
	}

	if constexpr (MultiThreading)
	{
		JobSystem::Initialize();

		ZWorker worker{};
		HRangeArgs rangeArgs = HRangeArgsBuilder().AddInt(1).Create();
		JobSystem::PushRangeJob(worker, 12 * 40, rangeArgs);

		for (int i = 0; i < 12; i++)
		{
			HArgsStruct args;
			args.p1 = (void*)i;
			JobSystem::PushJob(HJobDesc(Job1, nullptr, args));
		}
		
		{
			Timer overall("overall: ");
			while (counter.load() < 12) std::this_thread::yield();
		}

		std::this_thread::sleep_for(1000ms);

		std::cout << "worker result: " << worker.data[0] << std::endl;

		JobSystem::Terminate();
	}
	else
	{
		Timer overall("overall: ");
		CalculateAll(); 
	}
	int resultIndex;
	std::cin >> resultIndex;
	
	std::cout << "f: " << valuesf[resultIndex][0] << std::endl;
	std::cout << "i: " << values[resultIndex][0] << std::endl;
	
	return values[resultIndex][0] + (int)valuesf[resultIndex][0];
}

// normal 
//    overall: 41.445ms
//    5
//    f: 33.002
//    i : -2147483648
// sse / multithreading
//    overall: 6.008ms
//    5
//    f : 33.002
//    i : -2147483648

#endif

// add compatibility with cmake
// import Hustle Library as a sub repository
// range based job system:
//	void ProcessJobs(T* arr, int count, void*(*processFunction));
