#pragma once
#include <ntifs.h>

struct Process
{
public:
	Process(HANDLE Id)
	{
		status = PsLookupProcessByProcessId(Id, &eprocess);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] PsLookupProcessByProcessId (%x)\n", status);
		}
	}

	NTSTATUS GetLastStatus() const { return status; };
	PEPROCESS GetEprocess() const { return eprocess; };

	~Process()
	{
		if (eprocess)
			ObDereferenceObject(eprocess);
	}
private:
	PEPROCESS eprocess = nullptr;
	NTSTATUS status;
};