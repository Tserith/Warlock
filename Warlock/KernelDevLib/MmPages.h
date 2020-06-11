#pragma once
#include "MmMdl.h"

extern "C"
NTSYSAPI
NTSTATUS
NTAPI
ZwProtectVirtualMemory(
	_In_ HANDLE ProcessHandle,
	_Inout_ PVOID * BaseAddress,
	_Inout_ PSIZE_T RegionSize,
	_In_ ULONG NewProtect,
	_Out_ PULONG OldProtect
);

struct MmPages
{
public:
	MmPages(MmMdl& Mdl, enum _LOCK_OPERATION Access)
	{
		mdl = Mdl.GetMdl();

		if (mdl)
		{
			__try
			{
				MmProbeAndLockPages(mdl, UserMode, Access);
			}
			__except (EXCEPTION_EXECUTE_HANDLER)
			{
				if (Access)
				{
					// force copy-on-write and try again
					SetCowAndLock();
				}
				else
				{
					mdl = nullptr;

					DbgPrint("[-] MmProbeAndLockPages\n");
				}
			}
		}
	}

	PVOID GetMdlVa() const
	{
		if (!mdl)
			return nullptr;

		return MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);
	}

	void Unlock()
	{
		if (mdl)
		{
			MmUnlockPages(mdl);
			mdl = nullptr;
		}
	}

	~MmPages()
	{
		if (mdl)
			MmUnlockPages(mdl);
	}
private:
	void SetCowAndLock()
	{
		auto vAddr = MmGetMdlVirtualAddress(mdl);
		auto size = (SIZE_T)MmGetMdlByteCount(mdl);
		ULONG oldProtect;

		NTSTATUS status = ZwProtectVirtualMemory(
			ZwCurrentProcess(),
			&vAddr,
			&size,
			PAGE_EXECUTE_READWRITE,
			&oldProtect
		);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] ZwProtectVirtualMemory (%x)\n", status);
		}

		// dirty page
		__try
		{
			MmProbeAndLockPages(mdl, UserMode, IoWriteAccess);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			mdl = nullptr;

			DbgPrint("[-] MmProbeAndLockPages\n");
		}

		status = ZwProtectVirtualMemory(
			ZwCurrentProcess(),
			&vAddr,
			&size,
			oldProtect,
			&oldProtect
		);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] ZwProtectVirtualMemory (%x)\n", status);
		}
	}
	PMDL mdl;
};