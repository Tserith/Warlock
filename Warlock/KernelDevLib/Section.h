#pragma once
#include <Ntifs.h>

struct Section
{
public:
	Section()
	{
		maxSize.QuadPart = PAGE_SIZE;

		InitializeObjectAttributes(
			&obAttributes,
			NULL,
			OBJ_INHERIT,
			NULL,
			NULL
		);

		status = ZwCreateSection(
			&hSection,
			SECTION_ALL_ACCESS,
			&obAttributes,
			&maxSize,
			PAGE_READWRITE,
			SEC_COMMIT,
			NULL
		);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] ZwCreateSection (%x)\n", status);
		}

		if (!hSection)
			return;

		status = ZwMapViewOfSection(
			hSection,
			ZwCurrentProcess(),
			&vaddr,
			NULL,
			maxSize.QuadPart,
			NULL,
			&size,
			ViewUnmap,
			NULL,
			PAGE_READWRITE
		);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] ZwMapViewOfSection (%x)\n", status);
		}
	}

	NTSTATUS GetLastStatus() const { return status; };

	~Section()
	{
		status = ZwUnmapViewOfSection(ZwCurrentProcess(), vaddr);

		if (!NT_SUCCESS(status))
		{
			DbgPrint("[-] ZwUnmapViewOfSection (%x)\n", status);
		}

		if (hSection)
			ZwClose(hSection);
	}
private:
	OBJECT_ATTRIBUTES obAttributes;
	HANDLE hSection = nullptr;
	PVOID vaddr = nullptr;
	SIZE_T size = 0;
	LARGE_INTEGER maxSize;
	NTSTATUS status;
};