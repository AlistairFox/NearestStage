#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable:4995)
void __stdcall xrMemCopy_x86(LPVOID dest, const void* src, u32 n)
{
    memcpy(dest, src, n);
}

#if defined(M_BORLAND) || defined(_M_AMD64)
void __stdcall xrMemCopy_MMX(LPVOID dest, const void* src, u32 n)
{
    memcpy(dest, src, n);
}
#else
//-------------------------------------------------------------------------------------------------
#define TINY_BLOCK_COPY 64 //upper limit for movsd type copy
//The smallest copy uses the X86 "movsd"instruction,in an optimized
//form which is an "unrolled loop".
#define IN_CACHE_COPY 64 *1024 //upper limit for movq/movq copy w/SW prefetch
//Next is a copy that uses the MMX registers to copy 8 bytes at a time,
//also using the "unrolled loop"optimization.This code uses
//the software prefetch instruction to get the data into the cache.
#define UNCACHED_COPY 197 *1024 //upper limit for movq/movntq w/SW prefetch
//For larger blocks,which will spill beyond the cache,it ’s faster to
//use the Streaming Store instruction MOVNTQ.This write instruction
//bypasses the cache and writes straight to main memory.This code also
//uses the software prefetch instruction to pre-read the data.
//USE 64 *1024 FOR THIS VALUE IF YOU ’RE ALWAYS FILLING A "CLEAN CACHE"
#define BLOCK_PREFETCH_COPY infinity //no limit for movq/movntq w/block prefetch
#define CACHEBLOCK 80h //#of 64-byte blocks (cache lines)for block prefetch
//For the largest size blocks,a special technique called Block Prefetch
//can be used to accelerate the read operations.Block Prefetch reads
//one address per cache line,for a series of cache lines,in a short loop.
//This is faster than using software prefetch.The technique is great for
//getting maximum read bandwidth,especially in DDR memory systems.
#endif
