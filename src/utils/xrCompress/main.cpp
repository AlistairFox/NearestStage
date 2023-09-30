#include "stdafx.h"
#include "xrCompress.h"

#ifndef MOD_COMPRESS
	extern int				ProcessDifference();
#endif

#include <execution>

xr_vector<FS_File> filesMT;

xrCriticalSection cs;

void MT_UnpackFile()
{
	CTimer t;
	
	while(true)
	{
		cs.Enter();

		if (filesMT.empty())
		{
			break;
			cs.Leave();
		}

		FS_File s = filesMT.back();
		filesMT.pop_back();

		
		string_path file, file2; 
		FS.update_path(file, "$game_data$", s.name.c_str());
			
		if (!FS.path_exist("$game_unpacked$"))
		{
			FS.append_path("$game_unpacked$", "", "unpacked", 0);
			FS.update_path(file2, "$game_unpacked$", s.name.c_str());
		}
		else 
			FS.update_path(file2, "$game_unpacked$", s.name.c_str());

		cs.Leave();



		IReader* r = FS.r_open(file);
		int ms = 0;	
		if (r)
		{	
			IWriter* w = FS.w_open(file2);
			w->w(r->pointer(), r->length());
			cs.Enter();
			t.Start();
			FS.w_close(w);
			ms = t.GetElapsed_ms();
			cs.Leave();
		}
		
		FS.r_close(r);

		
		printf("S: %s ms: %d \n", s.name.c_str(), ms);
		Msg("S: %s ms: %d", s.name.c_str(), ms);
	}


}


#define MAX_THREADS 8


int __cdecl main	(int argc, char* argv[])
{
	Debug._initialize	(false);

	LPCSTR params = GetCommandLine();

	if(strstr(params,"-diff"))
		ProcessDifference	();
 
	if (strstr(params, "-unpack"))
	{	
		LPCSTR						fsgame_ltx_name = "-fsltx ";
		string_path					fsgame = "";
 		if (strstr(params, fsgame_ltx_name))
		{
			int						sz = xr_strlen(fsgame_ltx_name);
			sscanf(strstr(params, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
 		} 

		Core._initialize("xrCompress_Unpack", 0, TRUE, fsgame[0] ? fsgame : "NULL");

		FS_FileSet set;
		FS.file_list(set, "$game_data$");

		Msg("Start Save Files: %d", set.size());
		printf("Files Size: %d \n", set.size());

		for (auto s : set)
		{
			string_path file, file2; 
			FS.update_path(file, "$game_data$", s.name.c_str());
			
			if (!FS.path_exist("$game_unpacked$"))
			{
				FS.append_path("$game_unpacked$", "", "unpacked", 0);
				FS.update_path(file2, "$game_unpacked$", s.name.c_str());
			}
			else 
				FS.update_path(file2, "$game_unpacked$", s.name.c_str());

			printf("S: %s \n", s.name.c_str());
			Msg("S: %s", s.name.c_str());

			IReader* r = FS.r_open(file);
			
			if (r)
			{	
				IWriter* w = FS.w_open(file2);
				w->w(r->pointer(), r->length());
				FS.w_close(w);	
			}

			FS.r_close(r);
		}
	}
	else
	{
		Core._initialize("xrCompress", 0, FALSE);
		printf("\n\n");
 
		xrCompressor		C;


		if (strstr(params, "-encript"))
 			C.SetUseEncript(true);

		C.SetStoreFiles(NULL != strstr(params, "-store"));

		if (argc<2)	
		{
			printf("ERROR: u must pass folder name as parameter.\n");
			printf("-diff /? option to get information about creating difference.\n");
			printf("-fast	- fast compression.\n");
			printf("-store	- store files. No compression.\n");
			printf("-ltx <file_name.ltx> - pathes to compress.\n");
			printf("\n");
			printf("LTX format:\n");
			printf("	[config]\n");
			printf("	;<path>     = <recurse>\n");
			printf("	.\\         = false\n");
			printf("	textures    = true\n");
			
			Core._destroy();
			return 3;
		}

		string_path		folder;		
		strconcat		(sizeof(folder),folder,argv[1],"\\");
		_strlwr_s		(folder,sizeof(folder));
		printf			("\nCompressing files (%s)...\n\n",folder);

		FS._initialize	(CLocatorAPI::flTargetFolderOnly, folder);
		FS.append_path	("$working_folder$","",0,false);

		C.SetFastMode	(NULL!=strstr(params,"-fast"));
		C.SetTargetName	(argv[1]);

		if (strstr(params, "-max_size"))
		{
			u16 size = 0;
			LPCSTR size_archive = strstr(params, "-max_size ") + 10;
			sscanf(size_archive, "%d", &size);
			C.SetMaxVolumeSize(1024 * 1024 * size);
		}
		   

		LPCSTR p		= strstr(params,"-ltx");

		if(0!=p)
		{
			string64				ltx_name;
			sscanf					(strstr(params,"-ltx ")+5,"%[^ ] ", ltx_name);

			CInifile ini			(ltx_name);
			printf					("Processing LTX...\n");
			C.ProcessLTX			(ini);
		}
		else
		{
			string64				header_name;
			sscanf					(strstr(params,"-header ")+8,"%[^ ] ", header_name);
			C.SetPackHeaderName		(header_name);
			C.ProcessTargetFolder	();
		}
	}

	Core._destroy		();
	return 0;
}
