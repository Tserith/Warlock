#pragma once
#include <Ntifs.h>

struct MmMdl
{
public:
	MmMdl(PVOID Vaddr, ULONG Length)
	{
		mdl = IoAllocateMdl(Vaddr, Length, FALSE, FALSE, NULL);
	}

	PMDL GetMdl() const { return mdl; };

	~MmMdl()
	{
		if (mdl)
			IoFreeMdl(mdl);
	}
private:
	PMDL mdl;
};