#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <condition_variable>
#include <queue>
#include <array>
#include <functional>
#include "JobSystem.hpp"

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
	std::mutex mutex;
};

using uint = unsigned;

// very fast hash function + compile time
static inline constexpr uint KnuthHash(uint a, uint shift)
{
	const uint knuth_hash = 2654435769u;
	return ((a * knuth_hash) >> shift);
}

static inline constexpr void hash_combine(uint& s, const uint v, uint shift)
{
	s ^= KnuthHash(v, shift) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

// very fast hash function + compile time
static inline constexpr uint StringToHash(const char* string)
{
	uint hash = KnuthHash(string[0], 0);

	for (uint i = 1; i < __builtin_strlen(string); ++i) {
		hash_combine(hash, uint(string[i]), i);
	}

	return hash;
}

int t1Result[3];
std::atomic<int> atomic;
Semaphore semaphore(0);

constexpr int cResult0 = StringToHash("Hello From First thread, ");
constexpr int cResult1 = StringToHash("Hello From second thread, ");
constexpr int cResult2 = StringToHash("Hello From third thread");

void* Job1(void*)
{
	t1Result[0] = StringToHash("Hello From First thread, ");
	atomic++;
	semaphore.Take();
	semaphore.Give();
	return (void*)1;
}

void* Job2(void*)
{
	t1Result[1] = StringToHash("Hello From second thread, ");
	atomic++;
	semaphore.Take();
	semaphore.Give();
	return (void*)2;
}

void* Job3(void*)
{
	t1Result[2] = StringToHash("Hello From third thread");
	atomic++;
	semaphore.Take();
	semaphore.Give();
	return (void*)3;
}

void JobR(void* arg)
{
	std::cout << std::to_string((int)arg);
}

#define HMultiThreadingTest

#ifdef HMultiThreadingTest

int main()	
{
	JobSystem::Initialize();
	JobSystem::PushJob(JobDesc(Job1, JobR));
	JobSystem::PushJob(JobDesc(Job2, JobR));
	JobSystem::PushJob(JobDesc(Job3, JobR));

	while (atomic != 3) std::this_thread::yield();
	
	// using namespace std::chrono_literals;
	// std::this_thread::sleep_for(5ms);
	std::cout << std::endl;

	std::cout << t1Result[0] << " "
			  << t1Result[1] << " "
			  << t1Result[2] << " "; //<< t2Result << t3Result << std::endl;
	
	std::cout << "num threads: " << std::thread::hardware_concurrency() << std::endl;
	
	JobSystem::Terminate();

	return semaphore.count;
}

#endif

// add compatibility with cmake
// import Hustle Library as a sub repository
// range based job system:
//	void ProcessJobs(T* arr, int count, void*(*processFunction));