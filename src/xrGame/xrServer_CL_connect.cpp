#include "stdafx.h"
#include "xrserver.h"
#include "xrmessages.h"
#include "xrserver_objects.h"
#include "xrServer_Objects_Alife_Monsters.h"
#include "Level.h"


void xrServer::Perform_connect_spawn(CSE_Abstract* E, xrClientData* CL, NET_Packet& P)
{
	P.B.count = 0;
	xr_vector<u16>::iterator it = std::find(conn_spawned_ids.begin(), conn_spawned_ids.end(), E->ID);
	if(it != conn_spawned_ids.end())
	{
//.		Msg("Rejecting redundant SPAWN data [%d]", E->ID);
		return;
	}
	
	conn_spawned_ids.push_back(E->ID);
	
	if (E->net_Processed)						return;
	if (E->s_flags.is(M_SPAWN_OBJECT_PHANTOM))	return;

//.	Msg("Perform connect spawn [%d][%s]", E->ID, E->s_name.c_str());

	// Connectivity order
	CSE_Abstract* Parent = ID_to_entity	(E->ID_Parent);
	if (Parent)		Perform_connect_spawn	(Parent,CL,P);

	// Process
	Flags16			save = E->s_flags;
	//-------------------------------------------------
	E->s_flags.set	(M_SPAWN_UPDATE,TRUE);
	if (0==E->owner)	
	{
		// PROCESS NAME; Name this entity
		if (E->s_flags.is(M_SPAWN_OBJECT_ASPLAYER))
		{
			CL->owner			= E;
			VERIFY				(CL->ps);
			E->set_name_replace	(CL->ps->getName());
		}

		// Associate
		E->owner		= CL;
		E->Spawn_Write	(P,TRUE	);
		E->UPDATE_Write	(P);

		CSE_ALifeObject*	object = smart_cast<CSE_ALifeObject*>(E);
		VERIFY				(object);
		if (!object->keep_saved_data_anyway())
			object->client_data.clear	();
	}
	else				
	{
		E->Spawn_Write	(P, FALSE);
		E->UPDATE_Write	(P);
//		CSE_ALifeObject*	object = smart_cast<CSE_ALifeObject*>(E);
//		VERIFY				(object);
//		VERIFY				(object->client_data.empty());
	}
	//-----------------------------------------------------
	E->s_flags			= save;
	SendTo				(CL->ID,P,net_flags(TRUE,TRUE));
	E->net_Processed	= TRUE;
}

void xrServer::SendConfigFinished(ClientID const & clientId)
{
	NET_Packet	P;
	P.w_begin	(M_SV_CONFIG_FINISHED);
	SendTo		(clientId, P, net_flags(TRUE,TRUE));
}

void xrServer::SendConnectionData(IClient* _CL)
{
	conn_spawned_ids.clear();
	xrClientData*	CL				= (xrClientData*)_CL;
	NET_Packet		P;
	// Replicate current entities on to this client
	xrS_entities::iterator	I=entities.begin(),E=entities.end();
	for (; I!=E; ++I)						I->second->net_Processed	= FALSE;
	for (I=entities.begin(); I!=E; ++I)		Perform_connect_spawn		(I->second,CL,P);

	// Start to send server logo and rules
	SendServerInfoToClient			(CL->ID);

/*
	Msg("--- Our sended SPAWN IDs:");
	xr_vector<u16>::iterator it = conn_spawned_ids.begin();
	for (; it != conn_spawned_ids.end(); ++it)
	{
		Msg("%d", *it);
	}
	Msg("---- Our sended SPAWN END");
*/
};

void xrServer::OnCL_Connected		(IClient* _CL)
{
	xrClientData*	CL				= (xrClientData*)_CL;
	CL->net_Accepted = TRUE;
	/*if (Level().IsDemoPlay())
	{
		Level().StartPlayDemo();
		return;
	};*/
///	Server_Client_Check(CL);
	//csPlayers.Enter					();	//sychronized by a parent call
	Export_game_type(CL);
	Perform_game_export();
	SendConnectionData(CL);

	VERIFY2(CL->ps, "Player state not created");
	if (!CL->ps)
	{
		Msg("! ERROR: Player state not created - incorect message sequence!");
		return;
	}

	game->OnPlayerConnect(CL->ID);	
}

void	xrServer::SendConnectResult(IClient* CL, u8 res, u8 res1, char* ResultStr)
{
	NET_Packet	P;
	P.w_begin	(M_CLIENT_CONNECT_RESULT);
	P.w_u8		(res);
	P.w_u8		(res1);
	P.w_stringZ	(ResultStr);
	P.w_clientID(CL->ID);

	if (SV_Client && SV_Client == CL)
		P.w_u8(1);
	else
		P.w_u8(0);
	P.w_stringZ(Level().m_caServerOptions);
	
	SendTo		(CL->ID, P);

	if (!res)			//need disconnect 
	{
#ifdef MP_LOGGING
		Msg("* Server disconnecting client, resaon: %s", ResultStr);
#endif
		Flush_Clients_Buffers	();
		DisconnectClient		(CL, ResultStr);
	}

	if (Level().IsDemoPlay())
	{
		Level().StartPlayDemo();

		return;
	}
	
};

void xrServer::SendProfileCreationError(IClient* CL, char const * reason)
{
	VERIFY					(CL);
	
	NET_Packet	P;
	P.w_begin				(M_CLIENT_CONNECT_RESULT);
	P.w_u8					(0);
	P.w_u8					(ecr_profile_error);
	P.w_stringZ				(reason);
	P.w_clientID			(CL->ID);
	SendTo					(CL->ID, P);
	if (CL != GetServerClient())
	{
		Flush_Clients_Buffers	();
		DisconnectClient		(CL, reason);
	}
}

//this method response for client validation on connect state (CLevel::net_start_client2)
//the first validation is CDKEY, then gamedata checksum (NeedToCheckClient_BuildVersion), then 
//banned or not...
//WARNING ! if you will change this method see M_AUTH_CHALLENGE event handler
void xrServer::Check_GameSpy_CDKey_Success			(IClient* CL)
{
	if (NeedToCheckClient_BuildVersion(CL))
		return;
	//-------------------------------------------------------------
	RequestClientDigest(CL);
};

BOOL	g_SV_Disable_Auth_Check = FALSE;

bool xrServer::NeedToCheckClient_BuildVersion		(IClient* CL)	
{
/*#ifdef DEBUG

	return false; 

#endif*/
	xrClientData* tmp_client	= smart_cast<xrClientData*>(CL);
	VERIFY						(tmp_client);
	PerformSecretKeysSync		(tmp_client);


	if (g_SV_Disable_Auth_Check) return false;
	CL->flags.bVerified = FALSE;
	NET_Packet	P;
	P.w_begin	(M_AUTH_CHALLENGE);
	SendTo		(CL->ID, P);
	return true;
};

void xrServer::OnBuildVersionRespond				( IClient* CL, NET_Packet& P )
{
	u16 Type;
	P.r_begin( Type );
	u64 _our		=	23;
	u64 _him		=	P.r_u64();

	u8 reg, kit;

	shared_str login, password, comp_name, descript;  
	P.r_stringZ(login);
	P.r_stringZ(password);
	P.r_stringZ(comp_name);
	P.r_u8(reg);
	P.r_stringZ(descript);
	P.r_u8(kit);

	string256 game_version;
	sprintf(game_version, "Различные версии движка! Ваша: %d | Сервер: %d", _him, _our);

	string_path denied_reg;// filtering names
	string_path path_xray; // logins
	string_path banned_user; // hwid banlist
	string_path bad_register; // callback on uncorrect registration

	LPCSTR blockednames = login.c_str();

	FS.update_path(denied_reg, "$denied_accounts$", blockednames); // filtering names
	FS.update_path(path_xray, "$mp_saves_logins$", "logins.ltx"); // logins
	FS.update_path(banned_user, "$mp_banned_users$", "banned_list.ltx"); // hwid banlist
	FS.update_path(bad_register, "$mp_bad_register$", "bad_register.ltx"); // callback on uncorrect registration

	CInifile* file = xr_new<CInifile>(path_xray, true); // logins
	CInifile* banlist = xr_new<CInifile>(banned_user, true); // hwid banlist
	CInifile* bad_register_file = xr_new<CInifile>(bad_register, false, true);  // callback on uncorrect registration

	if (!CL->flags.bLocal)
	{

		Msg("--User HWID: %s, User Login: %s ", comp_name.c_str(), login.c_str());

		if (_our != _him)
		{
			SendConnectResult(CL, 0, ecr_data_verification_failed, game_version);
			Msg("!!ERROR Попытка входа с другой версии! Севрер: %d | Клиент: %d", _our, _him);
			xr_delete(file);
			xr_delete(banlist);
			xr_delete(bad_register_file);
			return;
		}

		if (banlist->line_exist("blocklist", comp_name.c_str()))
		{
			u8 ban_descr = banlist->r_u8("blocklist", comp_name.c_str());
			xr_delete(file);
			xr_delete(banlist);
			xr_delete(bad_register_file);
			if (ban_descr == 0)
			{
				Msg("!! ERROR: пользователь был заблокирован по причине 0.");
				SendConnectResult(CL, 0, ecr_data_verification_failed, "Вы были заблокированны, по причине: Оскорбление Администрации!");
				return;
			}
			else if (ban_descr == 1)
			{
				Msg("!! ERROR: пользователь был заблокирован по причине 1.");
				SendConnectResult(CL, 0, ecr_data_verification_failed, "Вы были заблокированны, по причине: Использование стороннего ПО, дающего преимущество в игре!");
				return;
			}
			else if (ban_descr == 2)
			{
				Msg("!! ERROR: пользователь был заблокирован по причине 2.");
				SendConnectResult(CL, 0, ecr_data_verification_failed, "Вы были заблокированны, по причине: Неадекватное поведение или оскорбления на сервере!");
				return;
			}
			else if (ban_descr == 3)
			{
				Msg("!! ERROR: пользователь был заблокирован по причине пидорас.");
				SendConnectResult(CL, 0, ecr_data_verification_failed, "Вы были заблокированны, по причине: Пидорас!");
				return;
			}
		}
		else
		{
			if (!bad_register_file->line_exist("bad_registration", comp_name.c_str())) 
			{
				if (reg == 0)
				{
					if (file->section_exist(login))
					{
						shared_str pass_check;

						if (file->line_exist(login, "password"))
						{
							pass_check = file->r_string(login, "password");

						}


						if (xr_strcmp(pass_check, password) != 0)
						{
							Msg("!! ERROR: Пользователь ввел неверный пароль");
							SendConnectResult(CL, 0, ecr_data_verification_failed, "Проверьте пароль.");
							return;
						}

						if (file->line_exist(login, "banned"))
						{
							SendConnectResult(CL, 0, ecr_data_verification_failed, "Вы забанены.");
							return;
						}

						if (Level().game)
							for (auto pl : Game().players)
							{
								if (!xr_strcmp(pl.second->getName(), login))
								{
									Msg("!! ERROR: Повторный вход с одного аккаунта");
									SendConnectResult(CL, 0, ecr_data_verification_failed, "Повторный вход с одного аккаунта.");
									return;
								}
							}
					}
					else
					{
						Msg("!! ERROR: Пользователь ввел неверный логин");
						SendConnectResult(CL, 0, ecr_data_verification_failed, "Неверный Логин.");
						return;
					}

				}
				else
				{
					LPCSTR hwid = comp_name.c_str();
					string_path reg_data;
					FS.update_path(reg_data, "$reg_data$", "hw_buffer.ltx");
					CInifile* reg_data_file = xr_new<CInifile>(reg_data, false, true);

					if (reg_data_file->line_exist("hwbuffer", hwid))
					{
						Msg("!! ERROR: Попытка повторного запроса на регистрацию от пользователя с HWid: %s", hwid);
						SendConnectResult(CL, 0, ecr_data_verification_failed, "У вас уже имеется зарегистрированный аккаунт!");
						xr_delete(reg_data_file);
						return;
					}
					else if (FS.exist(denied_reg))
					{
						Msg("!! ERROR: попытка регистрации некорректного никнейма!");
						SendConnectResult(CL, 0, ecr_data_verification_failed, "Заявка на регистрацию отклоненна: некорректный никнейм");
						xr_delete(reg_data_file);
						return;
					}
					else if (!file->section_exist(login))
					{
						xr_delete(reg_data_file);
						LPCSTR username = login.c_str();
						string_path path_registered;
						string256 transl;
						sprintf(transl, "%s.ltx", username);
						FS.update_path(path_registered, "$mp_acces_reg$", transl);
						if (FS.exist(path_registered))
						{
							Msg("!! ERROR: Попытка повторной регистрации аккаунта %s", username);
							SendConnectResult(CL, 0, ecr_data_verification_failed, "Данный никнейм уже ожидает регистрации.");
							return;
						}
						else
						{
							CInifile* regacc = xr_new<CInifile>(path_registered, false, true);
							if (regacc)
							{
								regacc->w_string("user_data", "username", username);
								regacc->w_string("user_data", "user_password", password.c_str());
								regacc->w_string("user_data", "hwid", comp_name.c_str());
								regacc->w_u8("user_data", "kit_numb", kit);
								regacc->w_string("user_data", "description", descript.c_str());
								regacc->save_as(path_registered);
								Msg("~ Пользователь %s подал запрос на регистрацию!", username);
							}
							xr_delete(regacc);
							SendConnectResult(CL, 0, ecr_data_verification_failed, "Вас Зарегистрируют в Ближайшее время!");
							return;
						}
					}
					else
					{
						xr_delete(reg_data_file);
						Msg("!! ERROR: Попытка регистрации занятого никнейма!");
						SendConnectResult(CL, 0, ecr_data_verification_failed, "Данный никнейм уже зарегистрирован!");
						return;
					}
					xr_delete(reg_data_file);
				}
			}
			else
			{
				u8 bad_register_descr = bad_register_file->r_u8("bad_registration", comp_name.c_str());
				bad_register_file->remove_line("bad_registration", comp_name.c_str());
				bad_register_file->save_as(bad_register);
				if (bad_register_descr == 0)
				{
					Msg("!! ERROR: пользователю была отказана регистрация по причине некорректный никнейм");
					SendConnectResult(CL, 0, ecr_data_verification_failed, "Отказ регистрации: смените никнейм!");
					return;
				}
				else if (bad_register_descr == 1)
				{
					Msg("!! ERROR: пользователю была отказана регистрация по причине некорректный пароль");
					SendConnectResult(CL, 0, ecr_data_verification_failed, "Отказ регистрации: смените пароль!");
					return;
				}
				else
				{
					Msg("!! ERROR: пользователю была отказана регистрация по причине %s", bad_register_descr);
					SendConnectResult(CL, 0, ecr_data_verification_failed, "Отказ регистрации: повторите попытку!");
					return;
				}
			}
		}

	}

	{				
		bool bAccessUser = false;
		string512 res_check;
		
		if ( !CL->flags.bLocal )
		{
			bAccessUser	= Check_ServerAccess( CL, res_check );
		}
				
		if( CL->flags.bLocal || bAccessUser )
		{
 			RequestClientDigest(CL);
		}
		else
		{
			Msg("* Client 0x%08x has an incorrect password", CL->ID.value());
			xr_strcat( res_check, "Invalid password.");
			SendConnectResult( CL, 0, ecr_password_verification_failed, res_check );
		}
	}
};

void xrServer::Check_BuildVersion_Success			( IClient* CL )
{
	CL->flags.bVerified = TRUE;
	SendConnectResult(CL, 1, 0, "All Ok");
};