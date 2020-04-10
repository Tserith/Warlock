#pragma once
#include <Ntifs.h>

struct PsThread
{
public:
	PsThread(PKSTART_ROUTINE Func, PVOID Data)
	{
		HANDLE hThread = nullptr;

		InitializeObjectAttributes(
			&obAttributes,
			NULL,
			OBJ_KERNEL_HANDLE,
			NULL,
			NULL
		);

		status = PsCreateSystemThread(
			&hThread,
			THREAD_ALL_ACCESS,
			&obAttributes,
			NULL,
			NULL,
			Func,
			Data
		);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] PsCreateSystemThread (%x)\n", status);
		}

		if (!hThread)
			return;

		// need PETHREAD to wait for thread to end
		status = ObReferenceObjectByHandle(
			hThread,
			THREAD_ALL_ACCESS,
			NULL,
			KernelMode,
			(PVOID*)&pethread,
			NULL
		);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] ObReferenceObjectByHandle (%x)\n", status);
		}

		status = ZwClose(hThread);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] ZwClose (%x)\n", status);
		}
	}

	NTSTATUS GetLastStatus() const { return status; };

	~PsThread()
	{
		// call PsTerminateSystemThread at the end of thread loop

		// Driver must not unload before thread ends
		status = KeWaitForSingleObject(
			pethread,
			Executive,
			KernelMode,
			FALSE,
			NULL
		);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] KeWaitForSingleObject (%x)\n", status);
		}

		ObDereferenceObject(pethread);
	}
private:
	OBJECT_ATTRIBUTES obAttributes;
	PETHREAD pethread = nullptr;
	NTSTATUS status;
};