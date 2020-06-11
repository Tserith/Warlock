#pragma once
#include "KernelDevLib.h"
#include "Common.h"
#include "ntimage.h"

UNICODE_STRING clientImageName = RTL_CONSTANT_STRING(CLIENT_NAME);
ObCallback* callback = nullptr;

struct
{
	Mutex mutex;
	UnicodeString targetImageName;
	HANDLE clientPid = nullptr;
	HANDLE targetPid = nullptr;
	PVOID targetBaseAddr = nullptr;
	WarlockComms* mailbox = nullptr;
}task;

PVOID FindUserModule(PEPROCESS Eprocess, PUNICODE_STRING Module, PSIZE_T Size)
{
	if (Eprocess)
	{
		if (PsGetProcessWow64Process(Eprocess))
		{
			DbgPrint("[-] Warlock does not support 32 bit processes\n");
			goto failure;
		}

		auto peb = PsGetProcessPeb(Eprocess);

		__try
		{
			if (peb && peb->Ldr)
			{
				for (PLIST_ENTRY listEntry = peb->Ldr->InLoadOrderModuleList.Flink;
					listEntry != &peb->Ldr->InLoadOrderModuleList;
					listEntry = listEntry->Flink)
				{
					auto entry = CONTAINING_RECORD(listEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

					if (!RtlCompareUnicodeString(&entry->BaseDllName, Module, FALSE))
					{
						*Size = entry->SizeOfImage;
						return entry->DllBase;
					}
				}
			}
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			DbgPrint("[-] Unable to find target module\n");
		}
	}

failure:
	*Size = 0;
	return nullptr;
}

OB_PREOP_CALLBACK_STATUS HandleRequest(PVOID, POB_PRE_OPERATION_INFORMATION)
{
	LockGuard lock(task.mutex);

	if (PsGetCurrentProcessId() == task.clientPid)
	{
		if (task.mailbox && task.clientPid)
		{
			MmMdl clientMdl(task.mailbox, sizeof(WarlockComms));
			MmPages clientPages(clientMdl, IoReadAccess);
			auto message = (WarlockComms*)clientPages.GetMdlVa();

			if (message)
			{
				__try
				{
					switch (message->type)
					{
					case messageType::setTarget:
						task.targetPid = message->target.pid;
						task.targetBaseAddr = nullptr;

						// ensure null termination
						message->target.targetImage[(sizeof(message->target.targetImage) / sizeof(wchar_t)) - 1] = '\0';
						task.targetImageName.Set(message->target.targetImage);

						DbgPrint("[+] Target set to %wZ\n", &task.targetImageName);
						break;
					case messageType::getInfo:
						if (task.targetPid)
						{
							if (!task.targetBaseAddr && task.targetImageName.str.Length)
							{
								Process proc(task.targetPid);
								PsContext procCtx(proc);
								SIZE_T imageSize;

								task.targetBaseAddr = FindUserModule(proc.GetEprocess(), &task.targetImageName.str, &imageSize);
							}

							if (task.targetBaseAddr)
							{
								message->info.baseAddr = task.targetBaseAddr;
								message->info.pid = task.targetPid;
							}
							else
							{
								message->info.baseAddr = nullptr;
								message->info.pid = nullptr;
							}
							break;
						}
					case messageType::rwRequest:
					{
						if (task.targetPid && task.targetBaseAddr)
						{
							auto access = IoReadAccess;
							if (message->request.type == requestType::write)
								access = IoWriteAccess;

							Process target(task.targetPid);
							PsContext targetCtx(target);
							MmMdl targetMdl(message->request.addr, sizeof(PVOID));
							MmPages targetPages(targetMdl, access);
							auto targetAddr = (PUINT64)targetPages.GetMdlVa();

							if (targetAddr)
							{
								if (message->request.type == requestType::read)
								{
									message->request.value = *targetAddr;
								}
								else if (message->request.type == requestType::write)
								{
									*targetAddr = message->request.value;
								}
							}
							else
							{
								DbgPrint("[-] Unable to access requested page\n");
								message->request.value = 0;
							}
						}
						break;
					}
					}

					message->type = messageType::received;
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("[-] Error reading/writing within image\n");
				}
			}
			else
			{
				DbgPrint("[-] Unable to access request buffer\n");
			}
		}
	}

	return OB_PREOP_SUCCESS;
}

void InspectNewProcess(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)
{
	UNREFERENCED_PARAMETER(ParentId);

	LockGuard lock(task.mutex);

	// reset if client is destroyed
	if (task.clientPid == ProcessId && !Create)
	{
		task.targetImageName.Set(L"");
		task.clientPid = nullptr;
		task.targetPid = nullptr;
		task.targetBaseAddr = nullptr;
		task.mailbox = nullptr;

		DbgPrint("[+] Client has unloaded\n");
	}
}

void InspectNewImage(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO ImageInfo)
{
	// not guaranteed
	if (FullImageName)
	{
		LockGuard lock(task.mutex);

		// only supports one client
		if (!task.clientPid && FullImageName->Length >= clientImageName.Length)
		{
			USHORT difference = FullImageName->Length - clientImageName.Length;

			FullImageName->Buffer += difference / 2;
			FullImageName->Length -= difference;

			if (!RtlCompareUnicodeString(FullImageName, &clientImageName, FALSE))
			{
				PVOID clientMailbox = nullptr;
				SIZE_T mailboxSize = sizeof(WarlockComms);

				NTSTATUS status = ZwAllocateVirtualMemory(
					ZwCurrentProcess(),
					&clientMailbox,
					0,
					&mailboxSize,
					MEM_COMMIT,
					PAGE_READWRITE
				);

				__try
				{
					auto headerOffset = ((PIMAGE_DOS_HEADER)ImageInfo->ImageBase)->e_lfanew;
					auto ntHeader = (PIMAGE_NT_HEADERS)((PUINT8)ImageInfo->ImageBase + headerOffset);
					auto section = IMAGE_FIRST_SECTION(ntHeader);

					if (!NT_SUCCESS(status))
					{
						DbgPrint("[-] ZwAllocateVirtualMemory (%x)\n", status);
						return;
					}

					for (int i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
					{
						if (!strncmp((PCHAR)section->Name, ".data", sizeof(section->Name)))
						{
							auto dataMemory = (PVOID*)((PUINT8)ImageInfo->ImageBase + section->VirtualAddress);

							// scan memory looking for magic value
							for (UINT64 j = 0; j < section->SizeOfRawData / sizeof(PVOID); j++)
							{
								if (*dataMemory == (PVOID*)MAGIC_VALUE)
								{
									MmMdl mdl(dataMemory, sizeof(PVOID));
									MmPages pages(mdl, IoWriteAccess);
									auto dataAddr = (PVOID*)pages.GetMdlVa();

									if (dataAddr)
									{
										*dataAddr = clientMailbox;
									}
									break;
								}

								if (j + 1 == section->SizeOfRawData / sizeof(PVOID))
								{
									DbgPrint("[-] Unable to find client pointer\n");
								}

								dataMemory++;
							}

							RtlZeroMemory(clientMailbox, mailboxSize);
							break;
						}

						if (i + 1 == ntHeader->FileHeader.NumberOfSections)
						{
							DbgPrint("[-] No .data section found\n");
							return;
						}

						section++;
					}
				}
				__except (EXCEPTION_EXECUTE_HANDLER)
				{
					DbgPrint("[-] Error reading/writing within image\n");
					return;
				}

				task.clientPid = ProcessId;
				task.mailbox = (WarlockComms*)clientMailbox;
			}
		}
		else if (!task.targetPid && task.targetImageName.str.Length &&
			FullImageName->Length >= task.targetImageName.str.Length &&
			(!task.targetPid || task.targetPid == ProcessId))
		{
			USHORT difference = FullImageName->Length - task.targetImageName.str.Length;

			FullImageName->Buffer += difference / 2;
			FullImageName->Length -= difference;

			if (!RtlCompareUnicodeString(FullImageName, &task.targetImageName.str, FALSE))
			{
				task.targetPid = ProcessId;

				DbgPrint("[+] Target loaded into process %lld\n", (UINT64)ProcessId);
			}
		}
	}
}

void WarlockUnload(PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	// only errors on redundant calls (which are acceptable)
	PsSetCreateProcessNotifyRoutine(InspectNewProcess, TRUE);
	PsRemoveLoadImageNotifyRoutine(InspectNewImage);

	callback->~ObCallback();
	task.targetImageName.Free();

	DbgPrint("[+] Warlock Unloaded!\n");
}

extern "C"
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	NTSTATUS status;

	DriverObject->DriverUnload = WarlockUnload;

	DbgPrint("[+] Warlock Loaded!\n");

	// set flag to bypass altitude checks - thanks for the writeup Daax!
	((PKLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection)->Flags |= 0x20;

	callback = new ObCallback(PsProcessType, HandleRequest);

	status = PsSetCreateProcessNotifyRoutine(InspectNewProcess, FALSE);

	if (!NT_SUCCESS(status))
	{
		DbgPrint("[-] PsSetCreateProcessNotifyRoutine (%x)\n", status);
		return status;
	}

	status = PsSetLoadImageNotifyRoutine(InspectNewImage);

	if (!NT_SUCCESS(status))
	{
		DbgPrint("[-] PsSetLoadImageNotifyRoutine (%x)\n", status);
		goto NotifyCleanup;
	}

	task.mutex.Init();

	return STATUS_SUCCESS;

NotifyCleanup:
	PsSetCreateProcessNotifyRoutine(InspectNewProcess, TRUE);
	return status;
}