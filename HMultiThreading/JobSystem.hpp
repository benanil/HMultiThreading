#pragma once
#include "../HContainers/HContainers/Common.hpp"
	
namespace JobSystem
{
	constexpr int MaxThreads = 32;

	struct Description
	{
		JobFunction func;
		JobCallback callback;
		ArgsStruct param;
		Description() : func(nullptr), callback(nullptr), param(nullptr) {}
		Description(JobFunction _func, JobCallback _callback = nullptr, ArgsStruct _param = nullptr) :
			func(_func), callback(_callback), param(_param)
		{}
	};

	// 16 byte data for arguments
	struct RangeArgs
	{
		void* a, *b;
		RangeArgs() : a(nullptr), b(nullptr) { }
		RangeArgs(void* x) : a(x) { }
		inline operator void* () const noexcept  { return a; }
		inline operator void** () const noexcept { return &a; }
	};

	// 32 byte data for arguments
	struct ArgsStruct 
	{
		union {
			struct { void* p1, *p2, *p3, *p4; };			
			struct { RangeArgs r1, r2; };			
			void* arr[4];
		};

		ArgsStruct() : p1(nullptr), p2(nullptr), p3(nullptr) { }
		ArgsStruct(void* x) : p1(x) { }
		
		RangeArgs GetRangeArgs() const { return r2; }

		void* operator[](int index) const { return arr[index];  }

		inline operator void** () const noexcept { return &p1; }
		inline operator void* () const noexcept { return p1; }
	};

	struct Worker
	{
		static ArgsStruct Work(ArgsStruct args);
		virtual ArgsStruct Process(int start, int end, RangeArgs) = 0;
	};

	template<typename args_t = ArgsStruct>
	struct ArgsBuilder
	{
		args_t data;
		char* bytes;
		char currentByte = 0;

		static ArgsBuilder Build() { return ArgsBuilder(); }
		
		ArgsBuilder() : data(args_t()) { bytes = (char*)&data; }

		args_t Create() { return data; }
		
		template<typename T>
		ArgsBuilder Add(T value) {
			*((T*)&currentByte[currentByte]) = value;
			currentByte += sizeof(T);
			return this;
		}

		FINLINE ArgsBuilder AddRange  (int begin, int end)   { return Add( PackIntegers(begin, end) ); }

		FINLINE ArgsBuilder AddInt    (int value)   { return Add(value); }
		FINLINE ArgsBuilder AddPointer(void* value) { return Add(value); }
		FINLINE ArgsBuilder AddFloat  (float value) { return Add(value); }
		FINLINE ArgsBuilder AddChar   (char value)  { return Add(value); }
	};

	template<typename args_t = ArgsStruct>
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

		FINLINE int   GetInt    () { return Get(value); }
		FINLINE void* GetPointer() { return Get(value); }
		FINLINE float GetFloat  () { return Get(value); }
		FINLINE char  GetChar   () { return Get(value); }
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
	template<typename RangeArgs_t = RangeArgs>
	void PushRangeJob(const Worker& workerClass, int numElements, RangeArgs rangeArgs = nullptr);
	
	void PushJobs(int numDescs, Description description[]);
}

typedef JobSystem::Description HJobDesc;
typedef JobSystem::ArgsStruct  HArgsStruct;
typedef JobSystem::RangeArgs   HRangeArgs;
typedef JobSystem::ArgsBuilder<HArgsStruct> HArgsBuilder;
typedef JobSystem::ArgsBuilder<HRangeArgs>  HRangeArgsBuilder;

#define HJOB_ENTRY(HFuncName) HArgsStruct HFuncName(HArgsStruct args) 
#define HJOB_CALLBACK(HFuncName) void HFuncName(HArgsStruct args) 

#define HJOB_CLASS_ENTRY(HFuncName, HClassName)  \
	HArgsStruct HFuncName(HArgsStruct args)                      \
	{                                                           \
		HClassName* instance = args[0];                         \
		JobSystem::JobFunction function = (JobFunction)args[1]; \
		return (*instance.*function)(args.GetRangeArgs());      \
	}

#define HJOB_CLASS_PUSH_WORK(HFuncName, ClassName, ClassFuncName, HClass, HArgs) \
	JobSystem::PushJob(JobDesc(HFuncName, nullptr, { HClass, &ClassName::ClassFuncName, HArgs}));
