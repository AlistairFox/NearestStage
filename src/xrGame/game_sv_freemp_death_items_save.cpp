#include "StdAfx.h"
#include "game_sv_freemp.h"
#include "Level.h"
#include "CustomDetector.h"
#include "CustomOutfit.h"
#include "Actor.h"
#include "Inventory.h"

void game_sv_freemp::SavePlayerOutfits(game_PlayerState* ps, CInifile* outfsFile)
{
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);
	if (!actor)
		return;

	if (smart_cast<CInventoryOwner*>(obj))
	{
		CCustomOutfit* pOutfit = smart_cast <CCustomOutfit*>(actor->inventory().ItemFromSlot(OUTFIT_SLOT));
		if (pOutfit)
		{
			outfits data;
			data.player_name = ps->getName();
			data.outfit_cond = pOutfit->GetCondition();
			data.outfit_name = pOutfit->m_section_id.c_str();

			auto PS = std::find_if(save_outfits.begin(), save_outfits.end(), [&](outfits data)
				{
					if (strstr(data.player_name, ps->getName()))
						return true;
					else
						return false;
				});

			if (PS == save_outfits.end())
				save_outfits.push_back(data);
			else
				(*PS) = data;
		}
		else
		{
			auto it =
				std::find_if(save_outfits.begin(), save_outfits.end(), [&](outfits data)
					{
						if (strstr(data.player_name, ps->getName()))
							return true;
						else
							return false;
					});
			if (it != save_outfits.end())
				save_outfits.erase(it);
		}
	}
}

void game_sv_freemp::SavePlayerDetectors(game_PlayerState* ps, CInifile* detsFile)
{
	CObject* obj = Level().Objects.net_Find(ps->GameID);
	CActor* actor = smart_cast<CActor*>(obj);
	if (!actor)
		return;
	if (smart_cast<CInventoryOwner*>(obj))
	{
		CCustomDetector* pDet = smart_cast<CCustomDetector*>(actor->inventory().ItemFromSlot(DETECTOR_SLOT));
		if (pDet)
		{
			detectors data;
			data.player_name = ps->getName();
			data.detector_cond = pDet->GetCondition();
			data.detector_name = pDet->m_section_id.c_str();

			auto DS = std::find_if(save_detectors.begin(), save_detectors.end(), [&](detectors data)
				{
					if (strstr(data.player_name, ps->getName()))
						return true;
					else
						return false;
				});

			if (DS == save_detectors.end())
				save_detectors.push_back(data);
			else
				(*DS) = data;
		}
		else
		{
			auto name =
				std::find_if(save_detectors.begin(), save_detectors.end(), [&](detectors data)
					{

						if (strstr(data.player_name, ps->getName()))
							return true;
						else
							return false;
					});
			if (name != save_detectors.end())
				save_detectors.erase(name);
		}
	}
}


void game_sv_freemp::LoadPlayerOtfits(game_PlayerState* ps, CInifile* outfsFile)
{
	auto PN = std::find_if(save_outfits.begin(), save_outfits.end(), [&](outfits data)
		{
			if (strstr(data.player_name, ps->getName()))
				return true;
			else
				return false;
		});

	if (PN == save_outfits.end())
		return;


	LPCSTR section = (*PN).outfit_name;
	float cond = (*PN).outfit_cond;

	Msg("%s Load Outfit Sect: %s, Cond: %f ", ps->getName(), section, cond);

	if (ps->testFlag(GAME_PLAYER_MP_SAVE_LOADED))
	{
		cond /= Random.randF(1.1, 2);
	}

	CSE_Abstract* E = spawn_begin(section);
	CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
	if (item)
	{
		item->m_fCondition = cond;
		item->ID_Parent = ps->GameID;
		spawn_end(item, m_server->GetServerClient()->ID);
	}
}

void game_sv_freemp::LoadPlayerDetectors(game_PlayerState* ps, CInifile* detsFile)
{
	auto PD = std::find_if(save_detectors.begin(), save_detectors.end(), [&](detectors data)
		{
			if (strstr(data.player_name, ps->getName()))
				return true;
			else
				return false;
		});

	if (PD == save_detectors.end())
		return;

	LPCSTR section = (*PD).detector_name;
	float cond = (*PD).detector_cond;
	if (cond > 1)
		cond = 1;

	Msg("%s Load Detector: %s", ps->getName(), section);

	CSE_Abstract* E = spawn_begin(section);
	CSE_ALifeItem* item = smart_cast<CSE_ALifeItem*>(E);
	if (item)
	{
		item->ID_Parent = ps->GameID;
		item->m_fCondition = cond;
		spawn_end(item, m_server->GetServerClient()->ID);
	}
}