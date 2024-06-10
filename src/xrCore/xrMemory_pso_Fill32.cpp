#include "stdafx.h"
#pragma hdrstop

void __stdcall xrMemFill32_x86(LPVOID dest, u32 value, u32 count)
{
    u32* ptr = (u32*)dest;
    u32* end = ptr + count;
    for (; ptr != end;) *ptr++ = value;
}

#if defined(M_BORLAND) || defined(_M_AMD64)
void __stdcall xrMemFill32_MMX(LPVOID dest, u32 value, u32 count)
{
    u32* ptr = (u32*)dest;
    u32* end = ptr + count;
    for (; ptr != end;) *ptr++ = value;
}
#else
/*
block fill:fill a number of DWORDs at DWORD aligned destination
with DWORD initializer using cacheable stores
*/
#endif
