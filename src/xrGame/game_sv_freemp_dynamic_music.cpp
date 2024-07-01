#include "StdAfx.h"
#include "game_sv_freemp.h"
#include "alife_simulator.h"
#include "alife_spawn_registry.h"
#include <Level.h>



void game_sv_freemp::DynamicMusicUpdate()
{
	/// Calculate Sync Radio Music
	if (set_next_music)
	{
		lenght = 0;
		set_next_music = FALSE;
		need_stop_music = true;
	}

	if (Level().game && !first_play && need_stop_music && lenght <= Device.dwTimeGlobal)
	{
		need_stop_music = false;
		need_next_snd = true;
		stop_timer = Device.dwTimeGlobal + 5000;
		//stop_timer = 0;
		NET_Packet P;
		P.w_begin(M_MUSIC_STOP);
		server().SendBroadcast(BroadcastCID, P, net_flags(true, true));
	}

	if (Level().game && need_next_snd && stop_timer <= Device.dwTimeGlobal)
	{
		i = Random.randI(1, MusicCount);

		need_next_snd = false;

		for (auto entity : phy_objects_cse)
		{
			CSE_ALifeObjectPhysic* phy = smart_cast<CSE_ALifeObjectPhysic*>(entity.second);
			if (phy)
			{
				numb += 1;

				if (numb == obj_count + 1)
					numb = 1;
				need_next_snd = false;
				lenght = 0;
				if (af_debug_loggining)
					Msg("%d", numb);
				MusicPlay(phy, i, numb);
			}
		}

		need_stop_music = true;
		first_play = false;
	}
	/// Calculate Sync Radio Music
}

void game_sv_freemp::DynamicMusicFileCreate()
{
	string_path music_path;
	FS.update_path(music_path, "$game_config$", "alife\\music.ltx");
	Music = xr_new<CInifile>(music_path, true, true);
	MusicCount = Music->r_u8("music", "sound_count");
}

/// Dynamic Music
void game_sv_freemp::MusicPlay(CSE_ALifeObjectPhysic* obj, int pass, int obj_num)
{
	LPCSTR musicfile = obj->m_ini_string.c_str();
	CInifile ini(&IReader((void*)(musicfile), xr_strlen(musicfile)), FS.get_path("$game_config$")->m_Path);

	CInifile* tmp = NULL;
	shared_str music_name;
	string128 line_name;
	xr_sprintf(line_name, sizeof(line_name), "sound%d", pass);
	if (ini.section_exist("play_bar_snd"))
	{
		tmp = Music;
	}

	if (tmp != nullptr)
	{
		if (tmp->section_exist("music"))
		{
			music_name = tmp->r_string("music", line_name);

			Fvector pos = obj->Position();
			NET_Packet P;

			if (af_debug_loggining)
			{
				Msg("- Server: MusicStart: %s", music_name.c_str());
				Msg("- object pos: %f, %f, %f", VPUSH(pos));
			}
			P.w_begin(M_MUSIC_UPDATE);
			P.w_stringZ(music_name);
			P.w_vec3(pos);
			P.w_u8(obj_num);
			server().SendBroadcast(BroadcastCID, P, net_flags(true, true));

			snd.create(music_name.c_str(), st_Music, sg_SourceType);
			snd.play(NULL, sm_2D);
			lenght = Device.dwTimeGlobal + (snd.get_length_sec() * 1000) + 10000;
			if (af_debug_loggining)
			{
				Msg("lenght: %f", snd.get_length_sec());
			}
			snd.destroy();
		}
	}
}