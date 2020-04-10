#pragma once
#include <Ntifs.h>

struct Mutex
{
public:
	void Init()
	{
		ExInitializeFastMutex(&mutex);
	}
	void Lock()
	{
		ExAcquireFastMutex(&mutex);
	}
	void Unlock()
	{
		ExReleaseFastMutex(&mutex);
	}
private:
	FAST_MUTEX mutex;
};

struct LockGuard
{
public:
	LockGuard(Mutex& Mutex) : mutex(Mutex)
	{
		mutex.Lock();
	}

	~LockGuard()
	{
		mutex.Unlock();
	}
private:
	Mutex& mutex;
};