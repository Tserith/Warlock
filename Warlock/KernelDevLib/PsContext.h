#pragma once
#include <Ntifs.h>
#include "Process.h"

struct PsContext
{
public:
	PsContext(const Process& Ps) : proc(Ps)
	{
		if (proc.GetEprocess())
			KeStackAttachProcess(proc.GetEprocess(), &state);
	}

	~PsContext()
	{
		if (proc.GetEprocess())
			KeUnstackDetachProcess(&state);
	}
private:
	const Process& proc;
	KAPC_STATE state;
};