#pragma once
#include <Ntifs.h>

struct UnicodeString
{
	// assumes String is null terminated
	BOOLEAN Set(PCWSTR String)
	{
		Free();

		return RtlCreateUnicodeString(&str, String);
	}

	void Free()
	{
		if (str.Buffer)
			RtlFreeUnicodeString(&str);
	}

	UNICODE_STRING str = { 0 };
};