#pragma once

class ENGINE_API xrDiscordPresense
{

public:

	void Initialize();
	void Shutdown();

	void SetStatus();
	void SetStatus1();

	~xrDiscordPresense();

private:
	bool bInitialize = false;
	bool bGameRPCInfoInit = false;
};

extern ENGINE_API xrDiscordPresense g_discord;