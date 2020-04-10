#include "ClientUtil.h"

void main()
{
	if ((UINT64)message != MAGIC_VALUE)
	{
		SetTarget(L"test.exe", NULL);

		PVOID addr = GetTargetBaseAddr();

		// do read and write operations here
	}
	else
	{
		printf("[-] Unable to communicate with driver\n");
	}
}