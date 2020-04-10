#include "ClientUtil.h"

void main()
{
	if ((UINT64)message != MAGIC_VALUE)
	{
		SetTarget(L"test.exe", NULL);

		PVOID addr = GetTargetBaseAddr();

		WriteMemory((PUINT8)addr + 0x102d, 0x4141414141414141);
	}
	else
	{
		printf("[-] Unable to communicate with driver\n");
	}
}