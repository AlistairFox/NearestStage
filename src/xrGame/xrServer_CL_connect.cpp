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

	SendConfigFinished(CL->ID);
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

	BOOL IsRegistration =  P.r_u8();
	shared_str login, password, comp_name, descript;
	u8 kit;
	P.r_stringZ(login);
	P.r_stringZ(password);
	P.r_stringZ(comp_name);
	P.r_stringZ(descript);
	P.r_u8(kit);

	std::string SLogin = login.c_str();
	std::string SPassword = password.c_str();
	std::string SHwid = comp_name.c_str();
	std::string SDescr = descript.c_str();

	if (!CL->flags.bLocal)
	{
		Msg("--User HWID: %s, User Login: %s ", comp_name.c_str(), login.c_str());

		string256 game_version;
		sprintf(game_version, "Различные версии движка! Ваша: %d | Сервер: %d", _him, _our);
		if (_our != _him)
		{
			SendConnectResult(CL, 0, ecr_data_verification_failed, game_version);
			Msg("!!ERROR Попытка входа с другой версии! Севрер: %d | Клиент: %d", _our, _him);
			return;
		}

		FS.update_path(JsonFilePath, "$mp_saves_logins$", "logins.json");
		if (IsRegistration)
		{
			if (!Registration(CL, SLogin, SPassword, SHwid, SDescr, kit))
			{
				return;
			}
			else
			{
				SendConnectResult(CL, 0, ecr_data_verification_failed, "Вас зарегистрируют в ближайшее время!!");
				Msg("-- Пользователь: %s подал запрос на регистрацию!", SLogin.c_str());
				return;
			}
		}
		else
		{
			if (!Loggining(CL, SLogin, SPassword, SHwid))
			{
				return;
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
}

bool xrServer::Registration(IClient* CL, std::string Login, std::string Password, std::string Hwid, std::string Descr, u8 kit)
{
	if (!containsOnlyDigits(Password))
	{
		Msg("!! ERROR: пользователь ввел запрещенные символы в пароль!");
		SendConnectResult(CL, 0, ecr_data_verification_failed, "Пароль может содержать только цифры!");
		return false;
	}

	if (containsOnlyDigits(Login))
	{
		Msg("!! ERROR: пользователь ввел запрещенные символы в логин!");
		SendConnectResult(CL, 0, ecr_data_verification_failed, "Логин не может содержать цифры!");
		return false;
	}

	if (!containsRestrictedChars(Login))
	{
		Msg("!! ERROR: пользователь ввел запрещенные символы в логин!");
		SendConnectResult(CL, 0, ecr_data_verification_failed, "Логин не может содержать что-либо кроме буквенных символов!");
		return false;
	}

	if (containsSpaces(Login))
	{
		Msg("!! ERROR: пользователь ввел запрещенные символы в логин!");
		SendConnectResult(CL, 0, ecr_data_verification_failed, "Логин не может содержать пробелы!");
		return false;
	}


	Object JsonMain;
	Array RegIns;
	std::ifstream ifile(JsonFilePath);

	std::string str((std::istreambuf_iterator<char>(ifile)), std::istreambuf_iterator<char>());
	JsonMain.parse(str);
	ifile.close();

	if (!CheckHwidBlock(CL, Hwid, JsonMain))
		return false;

	if (!CheckAlreadyExistAccount(CL, Login, Hwid, JsonMain))
		return false;

	if (JsonMain.has<Array>("REGISTRATION REQUESTS:"))
	{
		RegIns = JsonMain.get<Array>("REGISTRATION REQUESTS:");

		for (int i = 0; i != RegIns.size(); i++)
		{
			Object RegData = RegIns.get<Object>(i);

			if (RegData.has<String>("login"))
			{
				std::string NowCheckHwid = RegData.get<String>("hwid");
				if (NowCheckHwid == Hwid)
				{
					Msg("!! ERROR: Попытка повторной регистрации аккаунта %s", Login.c_str());
					SendConnectResult(CL, 0, ecr_data_verification_failed, "У вас уже имеется аккаунт ожидающий регистрации!");
					return false;
				}

				std::string NowCheckLogin = RegData.get<String>("login");
				if (NowCheckLogin == Login)
				{
					Msg("!! ERROR: Попытка повторной регистрации аккаунта %s", Login.c_str());
					SendConnectResult(CL, 0, ecr_data_verification_failed, "Данный никнейм уже ожидает регистрации.");
					return false;
				}
			}
		}
		
	}

	Object NowRegistryAccount;
	NowRegistryAccount << "login" << Login;
	NowRegistryAccount << "password" << Password;
	NowRegistryAccount << "hwid" << Hwid;
	NowRegistryAccount << "Descr" << Descr;
	NowRegistryAccount << "kit" << kit;
	RegIns << NowRegistryAccount;
	JsonMain << "REGISTRATION REQUESTS:" << RegIns;

	IWriter* writer = FS.w_open(JsonFilePath);
	if (writer)
	{
		writer->w_string(JsonMain.json().c_str());
		FS.w_close(writer);
	}

	return true;
}

bool xrServer::Loggining(IClient* CL, std::string Login, std::string Password, std::string Hwid)
{
	Object JsonMain;

	Array LogIns;
	std::ifstream ifile(JsonFilePath);

	std::string str((std::istreambuf_iterator<char>(ifile)), std::istreambuf_iterator<char>());
	JsonMain.parse(str);
	ifile.close();

	if (!CheckHwidBlock(CL, Hwid, JsonMain))
		return false;

	if (Level().game)
		for (auto pl : Game().players)
			if (pl.second->getName() == Login)
			{
				Msg("!! ERROR: Повторный вход с одного аккаунта");
				SendConnectResult(CL, 0, ecr_data_verification_failed, "Повторный вход с одного аккаунта.");
				return false;
			}

	if (JsonMain.has<Array>("ACCOUNTS:"))
	{
		bool FindAccount = false;
		LogIns = JsonMain.get<Array>("ACCOUNTS:");
		for (int i = 0; i != LogIns.size(); i++)
		{
			Object Account = LogIns.get<Object>(i);
			if (Account.has<String>("login"))
			{
				std::string AccountLogin = Account.get<String>("login");
				if (Login == AccountLogin)
				{
					if (Account.has<String>("password"))
					{
						FindAccount = true;
						std::string AccountPassword = Account.get<String>("password");
						if (Password != AccountPassword)
						{
							Msg("!! ERROR: Пользователь ввел неверный пароль");
							SendConnectResult(CL, 0, ecr_data_verification_failed, "Проверьте пароль.");
							return false;
						}
						return true;
					}
				}
			}
			
		}

		if (!FindAccount)
		{
			Msg("!! ERROR: Пользователь ввел неверный логин");
			SendConnectResult(CL, 0, ecr_data_verification_failed, "Неверный Логин.");
			return false;
		}
	}

	Msg("!! ERROR: Нет аккаунтов!");
	SendConnectResult(CL, 0, ecr_data_verification_failed, "Еще не существует зарегистрированных аккаунтов!");
	return false;
}

bool xrServer::CheckHwidBlock(IClient* CL, std::string Hwid, Object& JsonMain)
{
	Array BlockingArray;
	if (JsonMain.has<Array>("BLOCKING HWIDS:"))
	{
		BlockingArray = JsonMain.get<Array>("BLOCKING HWIDS:");
		for (int i = 0; i != BlockingArray.size(); i++)
		{
			Object Obj = BlockingArray.get<Object>(i);
			if (Obj.has<String>("hwid"))
			{
				std::string NowCheckHwid = Obj.get<String>("hwid");
				if (NowCheckHwid == Hwid)
				{
					std::string reason = Obj.get<String>("reason");
					std::string ServerReason = "!! ERROR: пользователь был заблокирован по причине: ";
					ServerReason += reason;

					std::string ClientReason = "Вы были заблокированны по причине: ";
					ClientReason += reason;

					char Res[128];
					strcpy(Res, ClientReason.c_str());
					Msg(ServerReason.c_str());
					SendConnectResult(CL, 0, ecr_data_verification_failed, Res);
					return false;
				}
			}
		}
	}
	
	if (!CheckBadRegister(CL, Hwid, JsonMain))
		return false;

	return true;
}

bool xrServer::CheckBadRegister(IClient* CL, std::string Hwid, Object& JsonMain)
{
	Array RejectArray;
	Array ReloadReject;
	bool NeedDisconnect = false;
	if (JsonMain.has<Array>("REJECTED REQUESTS:"))
	{
		RejectArray = JsonMain.get<Array>("REJECTED REQUESTS:");
		for (int i = 0; i != RejectArray.size(); i++)
		{
			Object Obj = RejectArray.get<Object>(i);
			if (Obj.has<String>("hwid"))
			{
				std::string NowCheckHwid = Obj.get<String>("hwid");
				if (NowCheckHwid == Hwid)
				{
					std::string reason = Obj.get<String>("reason");
					std::string ServerReason = "!! ERROR: пользователь был отключен по причине: ";
					ServerReason += reason;

					std::string ClientReason = "Запрос не регистрацию отклонен по причине: ";
					ClientReason += reason;

					char Res[128];
					strcpy(Res, ClientReason.c_str());
					Msg(ServerReason.c_str());
					SendConnectResult(CL, 0, ecr_data_verification_failed, Res);

					NeedDisconnect = true;
				}
				else
				{
					ReloadReject << Obj;
				}
			}
		}
	}

	if (NeedDisconnect)
	{
		JsonMain << "REJECTED REQUESTS:" << ReloadReject;

		IWriter* writer = FS.w_open(JsonFilePath);
		if (writer)
		{
			writer->w_string(JsonMain.json().c_str());
			FS.w_close(writer);
		}
		return false;
	}

	return true;
}

bool xrServer::CheckAlreadyExistAccount(IClient* CL, std::string Login,std::string Hwid, Object& JsonMain)
{
	Array AccountsArray;
	if (JsonMain.has<Array>("ACCOUNTS:"))
	{
		AccountsArray = JsonMain.get<Array>("ACCOUNTS:");
		for (int i = 0; i != AccountsArray.size(); i++)
		{
			Object Account = AccountsArray.get<Object>(i);
			if (Account.has<String>("login"))
			{
				std::string NowChekHwid = Account.get<String>("hwid");

				if (NowChekHwid == Hwid)
				{
					Msg("!! ERROR: Попытка повторного запроса на регистрацию от пользователя с HWid: %s", Hwid.c_str());
					SendConnectResult(CL, 0, ecr_data_verification_failed, "У вас уже имеется зарегистрированный аккаунт!");
					return false;
				}

				std::string NowCheckLogin = Account.get<String>("login");
				if (Login == NowCheckLogin)
				{
					Msg("!! ERROR: Попытка регистрации занятого никнейма!");
					SendConnectResult(CL, 0, ecr_data_verification_failed, "Данный никнейм уже зарегистрирован!");
					return false;
				}
			}
		}
	}

	return true;
}

void xrServer::Check_BuildVersion_Success			( IClient* CL )
{
	CL->flags.bVerified = TRUE;
	SendConnectResult(CL, 1, 0, "All Ok");
};