#include "StdAfx.h"
#include "xrServer.h"
#include "xrMessages.h"
#include "game_sv_freemp.h"

void xrServer::Process_event_PDA_CHAT(NET_Packet& P, ClientID sender)
{
	u8 type;
	P.r_u8(type);
	if (type == 0)
	{
		u8 global;
		P.r_u8(global);
		if (global == 1)
		{
			ClientID GameID;
			P.r_clientID(GameID);
			shared_str news_caption;
			P.r_stringZ(news_caption);
			shared_str news_text;
			P.r_stringZ(news_text);
			shared_str texture_name;
			P.r_stringZ(texture_name);


			Msg("Игрок [%s] id[%d], отправил сообщение [%s] ", news_caption.c_str(), GameID.value(), news_text.c_str());

			NET_Packet Packet;
			Packet.w_begin(M_GAMEMESSAGE);
			Packet.w_u32(GE_PDA_CHAT);
			Packet.w_u8(global);

			//Packet.w_clientID(GameID);
			Packet.w_stringZ(news_caption);
			Packet.w_stringZ(news_text);
			Packet.w_stringZ(texture_name);

			SendBroadcast(GetServerClient()->ID, Packet, net_flags(true, true));
		}
		else
		{
			ClientID SecondID;
			P.r_clientID(SecondID);
			ClientID GameID;
			P.r_clientID(GameID);
			shared_str news_caption;
			P.r_stringZ(news_caption);
			shared_str news_text;
			P.r_stringZ(news_text);
			shared_str texture_name;
			P.r_stringZ(texture_name);

			NET_Packet Packet;
			Packet.w_begin(M_GAMEMESSAGE);
			Packet.w_u32(GE_PDA_CHAT);
			Packet.w_u8(global);
			Packet.w_clientID(SecondID);
			Packet.w_clientID(GameID);
			Packet.w_stringZ(news_caption);
			Packet.w_stringZ(news_text);
			Packet.w_stringZ(texture_name);

			game_PlayerState* ps_to = game->get_id(SecondID);

			if (ps_to)
				Msg("Игрок [%s], Игроку[%s] отправил сообщение [%s] ", news_caption.c_str(), ps_to->getName(), news_text.c_str());

			SendTo(SecondID, Packet, net_flags(true, true));
			SendTo(GameID, Packet, net_flags(true, true));
		}
	}
	else
	{
		u16 remove_money;
		u16 add_money;
		u32 money;

		P.r_u16(remove_money);
		P.r_u16(add_money);
		P.r_u32(money);

		game_sv_freemp* freemp = smart_cast<game_sv_freemp*>(game);
		game_PlayerState* ps_from = freemp->get_eid(remove_money);
		game_PlayerState* ps_to = freemp->get_eid(add_money);

		xrClientData* cl_from = (xrClientData*)freemp->get_client(remove_money);
		xrClientData* cl_to = (xrClientData*)freemp->get_client(add_money);

		if (!ps_from || !ps_to)
		{
			return;
		}

		if (freemp)
		{
			if (money <= 0 || ps_from->money_for_round < money)
			{
				return;
			}

			freemp->AddMoneyToPlayer(ps_from, -money);
			freemp->AddMoneyToPlayer(ps_to, money);
		}

		{
			NET_Packet packet;

			string32 tmp, money_str = { 0 };
			itoa(money, tmp, 10);
			xr_strcat(money_str, "Перевод денег: ");
			xr_strcat(money_str, tmp);

			{
				packet.w_begin(M_GAMEMESSAGE);
				packet.w_u32(GE_PDA_CHAT);
				packet.w_u8(0);
				ClientID GameID = cl_to->ID;
				ClientID SecondID = cl_from->ID;

				packet.w_clientID(SecondID);
				packet.w_clientID(GameID);
				shared_str news_caption;
				packet.w_stringZ(news_caption);
				shared_str news_text = money_str;
				packet.w_stringZ(news_text);
				shared_str texture_name = "ui_inGame2_Dengi_polucheni";
				packet.w_stringZ(texture_name);


				xrClientData* data = (xrClientData*)game->get_client(add_money);
				SendTo(data->ID, packet, net_flags(true, true));
			}
			{
				packet.w_begin(M_GAMEMESSAGE);
				packet.w_u32(GE_PDA_CHAT);
				packet.w_u8(0);
				ClientID GameID = cl_to->ID;
				ClientID SecondID = cl_from->ID;
				packet.w_clientID(SecondID);
				packet.w_clientID(GameID);
				shared_str news_caption;
				packet.w_stringZ(news_caption);
				shared_str news_text = money_str;
				packet.w_stringZ(news_text);
				shared_str texture_name = "ui_inGame2_Dengi_otdani";
				packet.w_stringZ(texture_name);

				xrClientData* remove = (xrClientData*)game->get_client(remove_money);
				SendTo(remove->ID, packet, net_flags(true, true));
			}

			Msg("Перевод денег игроку [%s] от игрока [%s] колво [%d]", ps_from->getName(), ps_to->getName(), money);
		}
	}
}
