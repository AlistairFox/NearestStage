#include "stdafx.h"
#pragma hdrstop

#define USE_POOL
MEMPOOL		mem_pools[mem_pools_count];

#ifdef USE_POOL 

#include "xrMemory_align.h"

// MSVC
ICF	u8* acc_header(void* P) { u8* _P = (u8*)P;	return	_P - 1; }
ICF	u32		get_header(void* P) { return	(u32)*acc_header(P); }
ICF	u32		get_pool(size_t size)
{
	u32		pid = u32(size / mem_pools_ebase);
	if (pid >= mem_pools_count)	return mem_generic;
	else						return pid;
}

void* xrMemory::mem_alloc(size_t size)
{
	stat_calls++;

	u32		_footer = 0;
	void* _ptr = 0;

	if (!mem_initialized)
	{
		void* _real = xr_aligned_offset_malloc(1 + size + _footer, 16, 0x1);
		_ptr = (void*)(((u8*)_real) + 1);
		*acc_header(_ptr) = mem_generic;
	}
	else
	{
		u32	pool = get_pool(1 + size + _footer);
		if (mem_generic == pool)
		{
			void* _real = xr_aligned_offset_malloc(1 + size + _footer, 16, 0x1);
			_ptr = (void*)(((u8*)_real) + 1);
			*acc_header(_ptr) = mem_generic;
		}
		else
		{
			void* _real = mem_pools[pool].create();
			_ptr = (void*)(((u8*)_real) + 1);
			*acc_header(_ptr) = (u8)pool;
		}
	}
	memset(_ptr, 0, size);
	return	_ptr;
}

void	xrMemory::mem_free(void* P)
{
	stat_calls++;

	u32	pool = get_header(P);
	void* _real = (void*)(((u8*)P) - 1);
	if (mem_generic == pool)
	{
		// generic
		xr_aligned_free(_real);
	}
	else
	{
		// pooled
		VERIFY2(pool < mem_pools_count, "Memory corruption");
		mem_pools[pool].destroy(_real);
	}
}

extern BOOL	g_bDbgFillMemory;

void* xrMemory::mem_realloc(void* P, size_t size)
{
	stat_calls++;

	if (0 == P)
		return mem_alloc(size);

	u32		p_current = get_header(P);
	u32		p_new = get_pool(1 + size + 0);
	u32		p_mode;

	if (mem_generic == p_current) {
		if (p_new < p_current)		p_mode = 2;
		else						p_mode = 0;
	}
	else 							p_mode = 1;

	void* _real = (void*)(((u8*)P) - 1);
	void* _ptr = NULL;
	if (0 == p_mode)
	{
		u32		_footer = 0;
		void* _real2 = xr_aligned_offset_realloc(_real, 1 + size + _footer, 16, 0x1);
		_ptr = (void*)(((u8*)_real2) + 1);
		*acc_header(_ptr) = mem_generic;
	}
	else if (1 == p_mode)
	{
		R_ASSERT2(p_current < mem_pools_count, "Memory corruption");
		u32		s_current = mem_pools[p_current].get_element();
		u32		s_dest = (u32)size;
		void* p_old = P;

		void* p_new = mem_alloc(size);

		mem_copy(p_new, p_old, _min(s_current - 1, s_dest));
		mem_free(p_old);
		_ptr = p_new;
	}
	else if (2 == p_mode)
	{
		void* p_old = P;
		void* p_new = mem_alloc(size);
		mem_copy(p_new, p_old, (u32)size);
		mem_free(p_old);
		_ptr = p_new;
	}
	return	_ptr;
}

#else

#include <malloc.h>

void* xrMemory::mem_alloc(size_t	size)
{
	return malloc(size);
};
void* xrMemory::mem_realloc(void* p, size_t size)
{
	return realloc(p, size);
};
void  xrMemory::mem_free(void* p)
{
	if (p)
		free(p);
}
#endif