#pragma once

class ENGINE_API xrDiscordPresense
{

public:

	void Initialize();
	void Shutdown();

	void SetStatusOnServer();
	void SetStatusMainMenu();
	void SetStatusHostServer();

	~xrDiscordPresense();

private:
	bool bInitialize = false;
};

extern ENGINE_API xrDiscordPresense g_discord;