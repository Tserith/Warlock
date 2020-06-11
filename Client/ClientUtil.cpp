#include "ClientUtil.h"

// initialized by driver
volatile WarlockComms* message = (WarlockComms*)MAGIC_VALUE;

static void SendRequest()
{
	// invoke the driver's object callback
	HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, 0, GetCurrentProcessId());
}

// a NULL TargetPid will use the first process that loads the Target
void SetTarget(const wchar_t* Target, PVOID TargetPid)
{
	message->type = messageType::setTarget;
	message->target.pid = TargetPid;
	wcscpy_s((wchar_t*)message->target.targetImage, sizeof(message->target.targetImage) / 2, Target);
	
	SendRequest();
}

PVOID GetTargetBaseAddr()
{
	message->info.baseAddr = nullptr;

	while (!message->info.baseAddr)
	{
		message->type = messageType::getInfo;
		SendRequest();
	}

	return message->info.baseAddr;
}

PVOID GetTargetPid()
{
	message->info.pid = nullptr;

	while (!message->info.pid)
	{
		message->type = messageType::getInfo;
		SendRequest();
	}

	return message->info.pid;
}

UINT64 ReadMemory(PVOID Addr)
{
	message->type = messageType::rwRequest;
	message->request.type = requestType::read;
	message->request.addr = Addr;

	SendRequest();

	return message->request.value;
}

void WriteMemory(PVOID Addr, UINT64 Value)
{
	message->type = messageType::rwRequest;
	message->request.type = requestType::write;
	message->request.addr = Addr;
	message->request.value = Value;

	SendRequest();
}