#pragma once
#include <minwinbase.h>
#include <synchapi.h>
#include <heapapi.h>

LPVOID myHeapAlloc(int len)
{
	CRITICAL_SECTION critSection;
	EnterCriticalSection(&critSection);
	LPVOID lpMem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len + 64);
	LeaveCriticalSection(&critSection);
	return lpMem;
}
