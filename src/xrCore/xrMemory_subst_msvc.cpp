#include "stdafx.h"
#pragma hdrstop

#define USE_POOL
MEMPOOL		mem_pools[mem_pools_count];


#define PURE_MEMORY_FILL_ZERO
#define PURE_MEMORY_ALIGNMENT 1 << 4

#include "xrMemory_align.h"

void* xrMemory::mem_alloc(size_t size)
{
	void* result = _aligned_malloc(size, PURE_MEMORY_ALIGNMENT);

	if (result)
		memset(result, 0, size);     // FILL ZERO


	return (result);
}

void	xrMemory::mem_free(void* P)
{
	_aligned_free(P);
	return;
}

extern BOOL	g_bDbgFillMemory;

void* xrMemory::mem_realloc(void* P, size_t size)
{
	size_t old_size = P ? _aligned_msize(P, PURE_MEMORY_ALIGNMENT, 0) : 0; // FILL ZERO

	void* result = _aligned_realloc(P, size, PURE_MEMORY_ALIGNMENT);

	if (result && size > old_size)
		memset((u8*)result + old_size, 0, size - old_size);  // FILL ZERO

	return (result);
}
