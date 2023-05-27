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
	bGameRPCInfoInit = true;
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

void xrDiscordPresense::SetStatus1()
{

	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.startTimestamp = time(0);
	discordPresence.details = "In Main Menu";
	discordPresence.largeImageKey = "logo";
	discordPresence.largeImageText = "test2";
	discordPresence.smallImageKey = "logos";
	discordPresence.smallImageText = "test";
	Discord_UpdatePresence(&discordPresence);
}


void xrDiscordPresense::SetStatus()
{

	DiscordRichPresence discordPresence;
	memset(&discordPresence, 0, sizeof(discordPresence));
	discordPresence.startTimestamp = time(0);
	discordPresence.details = "Playing on Server";
	discordPresence.largeImageKey = "logo";
	discordPresence.largeImageText = "test2";
	discordPresence.smallImageKey = "logos";
	discordPresence.smallImageText = "test";
	Discord_UpdatePresence(&discordPresence);
}

xrDiscordPresense::~xrDiscordPresense()
{
	Shutdown();
}