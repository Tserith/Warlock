#pragma once
#include "Common.h"
#include <windows.h>
#include <stdio.h>

extern volatile WarlockComms* message;

void SetTarget(const wchar_t* Target, PVOID TargetPid);
PVOID GetTargetBaseAddr();
PVOID GetTargetPid();
UINT64 ReadMemory(PVOID addr);
void WriteMemory(PVOID addr, UINT64 value);