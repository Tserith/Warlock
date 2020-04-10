#pragma once
#include <Ntifs.h>

// if you make a work item loop make sure to free
// stuff inside it instead of in the unload routine
// to avoid freeing something used in the work item
// before it finishes - I believe the driver will wait

struct WorkItem
{
public:
	WorkItem(PDEVICE_OBJECT DeviceObject)
	{
		item = IoAllocateWorkItem(DeviceObject);
	}

	// the work item must be freed at the end of the callback
	void Queue(PIO_WORKITEM_ROUTINE_EX Func, PVOID Data)
	{
		if (item)
		{
			IoQueueWorkItemEx(item, Func, DelayedWorkQueue, Data);
			item = nullptr;
		}
		else
		{
			DbgPrint("[-] Work Item\n");
		}
	}
private:
	PIO_WORKITEM item = nullptr;
};