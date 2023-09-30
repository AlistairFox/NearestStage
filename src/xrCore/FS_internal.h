#ifndef FS_internalH
#define FS_internalH
#pragma once

#include "lzhuf.h"
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <share.h>

void*			FileDownload	(LPCSTR fn, u32* pdwSize=NULL);
void			FileCompress	(const char *fn, const char* sign, void* data, u32 size);
void * 			FileDecompress	(const char *fn, const char* sign, u32* size=NULL);

XRCORE_API extern void encryptDecryptXOR(u8* data, u32 size);

class CFileWriter : public IWriter
{
private:
	FILE*			hf;
	xr_vector<u8>	dataenc_buffer;


public:
	CFileWriter		(const char *name, bool exclusive)
	{
		R_ASSERT	(name && name[0]);
		fName		= name;
		VerifyPath	(*fName);
        if (exclusive)
		{
    		int handle	= _sopen(*fName,_O_WRONLY|_O_TRUNC|_O_CREAT|_O_BINARY,SH_DENYWR);
#ifdef _EDITOR
    		if (handle==-1)
    			Msg	("!Can't create file: '%s'. Error: '%s'.",*fName,_sys_errlist[errno]);
#endif
    		hf		= _fdopen(handle,"wb");
        }
		else
		{
			hf			= fopen(*fName,"wb");
			if (hf==0)
				Msg		("!Can't write file: '%s'. Error: '%s'.",*fName,_sys_errlist[errno]);
		}
	}

	virtual 		~CFileWriter()
	{
		if (0!=hf)
		{	
        	fclose				(hf);
        	// release RO attrib
	        DWORD dwAttr 		= GetFileAttributes(*fName);
	        if ((dwAttr != u32(-1))&&(dwAttr&FILE_ATTRIBUTE_READONLY)){
                dwAttr 			&=~ FILE_ATTRIBUTE_READONLY;
                SetFileAttributes(*fName, dwAttr);
            }
        }
	}
	// kernel
	virtual void	w			(const void* _ptr, u32 count) 
    { 
		if ((0!=hf) && (0!=count))
		{
			const u32 mb_sz = 0x1000000;
			u8* ptr 		= (u8*)_ptr;
			int req_size;
			for (req_size = count; req_size>mb_sz; req_size-=mb_sz, ptr+=mb_sz){
				size_t W = fwrite(ptr,mb_sz,1,hf);
				R_ASSERT3(W==1,"Can't write mem block to file. Disk maybe full.",_sys_errlist[errno]);
			}
			if (req_size)	{
				size_t W = fwrite(ptr,req_size,1,hf); 
				R_ASSERT3(W==1,"Can't write mem block to file. Disk maybe full.",_sys_errlist[errno]);
			}
		}
    };

	virtual void	seek		(u32 pos)	{	if (0!=hf) fseek(hf,pos,SEEK_SET);		};
	virtual u32		tell		()			{	return (0!=hf)?ftell(hf):0;				};
	virtual bool	valid		()			{	return (0!=hf);}
	virtual	void	flush		()			{	if (hf)	fflush(hf);						};

	// Encripting
	virtual void w_encrypt(const void* __ptr, u32 count)
	{
		if (count)
		{
			u8* ptr = (u8*)__ptr;
			for (auto i = 0; i < count; i++)
				dataenc_buffer.push_back(ptr[i]);
		}
	};

	virtual void data_encript()
	{
 		encryptDecryptXOR(dataenc_buffer.data(), dataenc_buffer.size());
		w(dataenc_buffer.data(), dataenc_buffer.size());
	};
		
	virtual u32		enc_tell() { return dataenc_buffer.size(); };
};

// It automatically frees memory after destruction
class CTempReader : public IReader
{
public:
				CTempReader(void *_data, int _size, int _iterpos) : IReader(_data,_size,_iterpos)	{}
	virtual		~CTempReader();
};
class CPackReader : public IReader
{
	void*		base_address;
public:
				CPackReader(void* _base, void* _data, int _size) : IReader(_data,_size){base_address=_base;}
	virtual		~CPackReader();
};
class XRCORE_API CFileReader : public IReader
{
public:
				CFileReader(const char *name);
	virtual		~CFileReader();
};
class CCompressedReader : public IReader
{
public:
				CCompressedReader(const char *name, const char *sign);
	virtual		~CCompressedReader();
};
class CVirtualFileReader : public IReader
{
private:
   void			*hSrcFile, *hSrcMap;
public:
				CVirtualFileReader(const char *cFileName);
	virtual		~CVirtualFileReader();
};

#endif