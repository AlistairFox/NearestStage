#include "stdafx.h"
#include "VoiceSender.h"
#include "game_cl_mp.h"
#include"game_sv_mp.h"
#include "../xrSound/SoundVoiceChat.h"
#include "Actor.h"
#include "Inventory.h"
#include "WalkieTalkie.h"

void CVoiceSender::Send(VoicePacket** packets, u8 count)
{
	bool radio_on = false;
	u16 radio_hz = 0;
	float radio_max_distance = 0;
	if (Actor())
	{
		CWalkieTalkie* itm = smart_cast<CWalkieTalkie*>(Actor()->inventory().ItemFromSlot(RADIO_SLOT));
		if (itm)
		{
			if (itm->IsInHand())
			{
				radio_on = itm->SayNow;
				radio_hz = itm->CurrentHZ;
				radio_max_distance = itm->MaxDistance;
			}
		}
	}


	NET_Packet P;
	P.w_begin(M_VOICE_MESSAGE);
	P.w_u8(m_distance);
	P.w_u16(Level().game->local_player->GameID);
	P.w_u8(radio_on);
	P.w_u16(radio_hz);
	P.w_u16(radio_max_distance);
	P.w_u8(count);

	for (int i = 0; i < count; ++i)
	{
		VoicePacket* packet = packets[i];
		P.w_u32(packet->length);
		P.w(packet->data, packet->length);
	}

	Level().Send(P, net_flags(FALSE, TRUE, TRUE, TRUE));
}
