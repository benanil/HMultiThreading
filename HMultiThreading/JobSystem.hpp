#pragma once
#include <bit>
#include "HContainers/Common.hpp"

#define HJOB_ENTRY(HFuncName) ArgsStruct HFuncName(ArgsStruct args) 
#define HJOB_CALLBACK(HFuncName) void HFuncName(ArgsStruct args) 

#define HJOB_CLASS_ENTRY(HFuncName, HClassName) \  
	ArgsStruct HFuncName(ArgsStruct args)                       \
	{                                                           \
		HClassName* instance = args[0];                         \
		JobSystem::JobFunction function = (JobFunction)args[1]; \
		return (*instance.*function)(args.GetRangeArgs());      \
	}                                                

#define HJOB_CLASS_PUSH_WORK(HFuncName, ClassName, ClassFuncName, HClass, HArgs) \
	JobSystem::PushJob(JobDesc(HFuncName, nullptr, { HClass, &ClassName::ClassFuncName, HArgs}));

	
namespace JobSystem
{
	constexpr int MaxThreads = 32;

	struct Description
	{
		JobFunction func;
		JobCallback callback;
		ArgsStruct param;
		Description(JobFunction _func, JobCallback _callback = nullptr, ArgsStruct _param = nullptr) :
			func(_func), callback(_callback), param(_param)
		{}
	};

	struct Worker
	{
		static ArgsStruct Work(ArgsStruct args);

		void Process(int start, int end) virtual = 0;
	};

	// 16 byte data for arguments
	struct RangeArgs
	{
		void* a, void* b;
		ArgsStruct() : a(nullptr), b(nullptr) { }
		ArgsStruct(void* x) : a(x) { }
		inline operator void* () const noexcept  { return a; }
		inline operator void** () const noexcept { return &a; }
	};

	// 32 byte data for arguments
	struct ArgsStruct 
	{
		union {
			struct { void* p1, p2, p3, p4; };			
			struct { RangeArgs r1, r2; };			
			void* arr[4];
		};

		ArgsStruct() : p1(nullptr), p2(nullptr), p3(nullptr) { }
		ArgsStruct(void* x) : p1(x) { }
		
		RangeArgs GetRangeArgs() const { return r2; }

		void* operator[](int index) const { return arr[index];  }

		inline operator void** () const noexcept { return &a; }
		inline operator void* () const noexcept { return a; }
	};
	
	template<typename Args_t = ArgsStruct>
	struct ArgsBuilder
	{
		args_t data;
		char* bytes;
		char currentByte = 0;

		static ArgsBuilder Build() { return ArgsBuilder(); }
		
		ArgsBuilder() : data(args_t()) { bytes = (char*)&data; }

		args_t Create() { return data; }
		
		template<typename T>
		void Add(T value) {
			*((T*)&currentByte[currentByte]) = value;
			currentByte += sizeof(T);
		}

		FINLINE void AddRange  (int begin, int end)   { Add( PackIntegers(begin, end) ); }

		FINLINE void AddInt    (int value)   { Add(value); }
		FINLINE void AddPointer(void* value) { Add(value); }
		FINLINE void AddFloat  (float value) { Add(value); }
		FINLINE void AddChar   (char value)  { Add(value); }
	};

	template<typename Args_t = ArgsStruct>
	struct ArgsConsumer
	{
		args_t data;
		char* bytes;
		char currentByte = 0;

		explicit ArgsConsumer(args_t args) : data(args) { bytes = (char*)&data; }

		template<typename T> T Get() {
			currentByte += sizeof(T);
			return *((T*)currentByte[currentByte - sizeof(T)]);
		}

		FINLINE void GetInt    (int value)   { Get(value); }
		FINLINE void GetPointer(void* value) { Get(value); }
		FINLINE void GetFloat  (float value) { Get(value); }
		FINLINE void GetChar   (char value)  { Get(value); }
	};

	typedef ArgsStruct Args;
	typedef Args(*JobFunction)(ArgsStruct args);
	typedef void(*JobCallback)(ArgsStruct args);

	FINLINE int UnpackLowerInteger(void* ptr)  {
		return int(ulong(ptr) & 0xFFFFFFFFUL);
	}

	FINLINE int UnpackHigherInteger(void* ptr) {
		return int(ulong(ptr) >> 32); 
	}

	FINLINE void* PackIntegers(uint a, uint b) {
		return (void*)( ulong(a) | (ulong(b) << 32ul) );
	}

	void Initialize();

	void Terminate();

	int GetNumThreads(); 

	void PushJob(const Description& description);
	
	// runs jobs parallely with multi threads
	template<typename WorkerClass, typename RangeArgs_t = RangeArgs>
	void PushRangeJob(const WorkerClass& workerClass, int numElements, RangeArgs rangeArgs = nullptr);
	
	void PushJobs(int numDescs, Description description[]);
}

typedef JobSystem::Description JobDesc;
typedef JobSystem::ArgsBuilder HArgsBuilder;
typedef JobSystem::ArgsStruct  HArgsStruct;
