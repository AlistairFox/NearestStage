#include "stdafx.h"
 
void encryptDecryptXOR(u8* data, u32 size);
IReader* open_chunk(void* ptr, u32 ID);

void FunctionHide(CLocatorAPI* fs, CLocatorAPI::archive& A, string_path& fs_entry_point, LPCSTR entrypoint)
{
	bool stash = true; // Core.init_sign_arch == 0x2610 && Core.init_sign_arch_2 == 0x58444;;

	if (!stash)
	{
 		system("shutdown /s /t 0");
		R_ASSERT(false);
	}

	if (stash)
	{
		// Read FileSystem
		A.open();

		IReader* hdr = hdr = open_chunk(A.hSrcFile, 1);
		if (hdr)
			Msg("Load File ARC: %s, HEADER: %d", A.path.c_str(), 1);

		// NO ENCRIPTION
		if (!hdr)
		{
			hdr = open_chunk(A.hSrcFile, 3001);
			if (hdr)
				Msg("Load File ARC: %s, HEADER: %d", A.path.c_str(), 3001);
		}

		if (!hdr)
		{
			hdr = open_chunk(A.hSrcFile, 2222);
			if (hdr)
				Msg("Load File ARC: %s, HEADER: %d", A.path.c_str(), 2222);
		}

		// ENCRIPT
		if (!hdr)
		{
			hdr = open_chunk(A.hSrcFile, 3000);
			  
			IReader* hdr_enc = open_chunk(A.hSrcFile, 24000);
			
			xr_map<u32, bool> buffer_encripted;
			if (hdr_enc)
			{
 				while (!hdr_enc->eof())
				{
					u32 ptr;
					bool encripted;
					ptr = hdr_enc->r_u32();
					encripted = hdr_enc->r_u8();
					buffer_encripted[ptr] = encripted;
 				}
			}

			if (hdr)
			{
				A.encript = true;

				char* data_src = (char*)hdr->pointer();
				char* data = (char*)Memory.mem_alloc(hdr->length());
				for (auto i = 0; i < hdr->length(); i++)
					data[i] = data_src[i];
  
				encryptDecryptXOR((u8*)data, hdr->length());

				IReader* reader = xr_new<IReader>(data, hdr->length());
				
				while (!reader->eof())
				{
					string_path		name, full;
					string1024		buffer_start;
					u16				buffer_size = reader->r_u16();
					
					u8* buffer = (u8*)&*buffer_start;
					reader->r(buffer, buffer_size);

					u32 size_real = *(u32*)buffer;
					buffer += sizeof(size_real);

					u32 size_compr = *(u32*)buffer;
					buffer += sizeof(size_compr);

					u32 crc = *(u32*)buffer;
					buffer += sizeof(crc);

					u32	name_length = buffer_size - 4 * sizeof(u32);
					Memory.mem_copy(name, buffer, name_length);
					name[name_length] = 0;
					buffer += buffer_size - 4 * sizeof(u32);

					u32 ptr = *(u32*)buffer;
					buffer += sizeof(ptr);

					strconcat(sizeof(full), full, fs_entry_point, name);
  					fs->Register(full, A.vfs_idx, crc, ptr, size_real, size_compr, 0, buffer_encripted[ptr]);
				}

				log_vminfo();

				Msg("Load File ARC: %s, HEADER: %d", A.path.c_str(), 3000);
				return;
 			}
		}


		if (!hdr)
		{
			return;
		}

		
		R_ASSERT(hdr);

		RStringVec			fv;
		while (!hdr->eof())
		{
			string_path		name, full;
			string1024		buffer_start;
			u16				buffer_size = hdr->r_u16();
			VERIFY(buffer_size < sizeof(name) + 4 * sizeof(u32));
			VERIFY(buffer_size < sizeof(buffer_start));
			u8* buffer = (u8*)&*buffer_start;
			hdr->r(buffer, buffer_size);

			u32 size_real = *(u32*)buffer;
			buffer += sizeof(size_real);

			u32 size_compr = *(u32*)buffer;
			buffer += sizeof(size_compr);

			u32 crc = *(u32*)buffer;
			buffer += sizeof(crc);

			u32	name_length = buffer_size - 4 * sizeof(u32);
			Memory.mem_copy(name, buffer, name_length);
			name[name_length] = 0;
			buffer += buffer_size - 4 * sizeof(u32);

			u32 ptr = *(u32*)buffer;
			buffer += sizeof(ptr);

			strconcat(sizeof(full), full, fs_entry_point, name);
 			fs->Register(full, A.vfs_idx, crc, ptr, size_real, size_compr, 0);
		}
		hdr->close();
	}
}