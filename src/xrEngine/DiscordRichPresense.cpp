#include "stdafx.h"
#include "DiscordRichPresense.h"
#include "DiscordRichPresense/discord_register.h"
#include "DiscordRichPresense/discord_rpc.h"
#include "x_ray.h"
#include "defines.h"

ENGINE_API xrDiscordPresense g_discord;
DiscordRichPresence presenseInfo;

void xrDiscordPresense::Initialize()
{
	LPCSTR discord_app_id;
	discord_app_id = READ_IF_EXISTS(pSettings, r_string, "global", "discord_app_id", "1108397538018599035");
	// We don't have multiplayer mode, so no need to invite system to support
	DiscordEventHandlers nullHandlers;
	ZeroMemory(&nullHandlers, sizeof(nullHandlers));
	Discord_Initialize(discord_app_id, &nullHandlers, TRUE, nullptr);
	presenseInfo.startTimestamp = time(0);
	bInitialize = true;
}

void xrDiscordPresense::Shutdown()
{
	if (bInitialize)
	{
		Discord_ClearPresence();
		Discord_Shutdown();
		bInitialize = false;
	}
}

void xrDiscordPresense::SetStatusMainMenu()
{

	memset(&presenseInfo, 0, sizeof(presenseInfo));
	presenseInfo.startTimestamp = time(0);
	presenseInfo.details = "In Main Menu";
	presenseInfo.largeImageKey = "logo";
	presenseInfo.largeImageText = "Pre-Alpha testing";
	presenseInfo.smallImageKey = "logos";
	presenseInfo.smallImageText = "Public-Version 1.0";
	Discord_UpdatePresence(&presenseInfo);
}


void xrDiscordPresense::SetStatusOnServer()
{
	memset(&presenseInfo, 0, sizeof(presenseInfo));
	presenseInfo.startTimestamp = time(0);
	presenseInfo.details = "Playing on Server";
	presenseInfo.largeImageKey = "logo";
	presenseInfo.largeImageText = "Pre-Alpha testing";
	presenseInfo.smallImageKey = "logos";
	presenseInfo.smallImageText = "Public-Version 1.0";
	Discord_UpdatePresence(&presenseInfo);
}


void xrDiscordPresense::SetStatusHostServer()
{
	memset(&presenseInfo, 0, sizeof(presenseInfo));
	presenseInfo.startTimestamp = time(0);
	presenseInfo.details = "Hosted Server";
	presenseInfo.largeImageKey = "logo";
	presenseInfo.largeImageText = "Pre-Alpha testing";
	presenseInfo.smallImageKey = "logos";
	presenseInfo.smallImageText = "Public-Version 1.0";
	Discord_UpdatePresence(&presenseInfo);
}

xrDiscordPresense::~xrDiscordPresense()
{
	Shutdown();
}