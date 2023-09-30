#include "stdafx.h"
#include "FS.h"
 
bool CheckForByte(u8 byte)
{
	if (0x0A == byte)
		return false;
	else 
		return true;
}

extern void encryptDecryptXOR_OLD(u8* data, u32 size)
{
	// бюфмн ме ялнфел пюяоюйнбюрэ еякх саепел хке онлемъел
	std::vector<u8> buffer = { 8, 16, 24, 32, 40, 48, 56, 64 };   

 
	for (size_t i = 0; i < size; ++i)
	{
		u8 d = u8(data[i]);
		u8 encripted = d ^ buffer[i % buffer.size()];
		
		if (CheckForByte(d) && CheckForByte(encripted) )
			data[i] = encripted;
	}
}

 
extern void encryptDecryptXOR(u8* data, u32 size)
{
	// бюфмн ме ялнфел пюяоюйнбюрэ еякх саепел хке онлемъел
	std::vector<u8> buffer = { 8, 16, 24, 32, 40, 48, 56, 64 };   

 
	for (size_t i = 0; i < size; ++i)
	{
		u8 d = u8(data[i]);
		u8 encripted = d ^ buffer[i % buffer.size()];
		
		if (CheckForByte(d) && CheckForByte(encripted) )
			data[i] = encripted;
	}
}

extern void encryptDecryptXOR_offset(u8* data, u32 size, u32 offset)
{
	// бюфмн ме ялнфел пюяоюйнбюрэ еякх саепел хке онлемъел
	std::vector<u8> buffer = { 8, 16, 24, 32, 40, 48, 56, 64 };

	for (size_t i = 0; i < size; ++i)
	{
		u8 d = u8(data[i]);
		u8 encripted = d ^ buffer[(offset + i) % buffer.size()];

		if (CheckForByte(d) && CheckForByte(encripted))
			data[i] = encripted;
	}
}
 
void CMemoryWriter::data_encript()
{
 	u8* ptr = (u8*)tell() - chunk_size();
	encryptDecryptXOR(ptr, chunk_size());
}


void CMemoryWriter::w_encrypt(const void* ptr, u32 count)
{
 	//ECRIPT
	u8* data_encript;
	data_encript = (BYTE*)Memory.mem_alloc(count);

	encryptDecryptXOR(data_encript, count);


	if (position + count > mem_size)
	{
		// reallocate
		if (mem_size == 0)
			mem_size = 128;
		while (mem_size <= (position + count))
			mem_size *= 2;

		if (0 == data)
			data = (BYTE*)Memory.mem_alloc(mem_size);
		else
			data = (BYTE*)Memory.mem_realloc(data, mem_size);
	}

	CopyMemory(data_encript + position, data_encript, count);
	position += count;
	if (position > file_size)
		file_size = position;
}


