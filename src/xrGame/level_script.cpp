////////////////////////////////////////////////////////////////////////////
//	Module 		: level_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Level script export
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "level.h"
#include "actor.h"
#include "script_game_object.h"
#include "patrol_path_storage.h"
#include "xrServer.h"
#include "client_spawn_manager.h"
#include "../xrEngine/igame_persistent.h"
#include "game_cl_base.h"
#include "UIGameCustom.h"
#include "UI/UIDialogWnd.h"
#include "date_time.h"
#include "ai_space.h"
#include "level_graph.h"
#include "PHCommander.h"
#include "PHScriptCall.h"
#include "script_engine.h"
#include "game_cl_single.h"
#include "game_sv_single.h"
#include "game_sv_mp.h"
#include "map_manager.h"
#include "map_spot.h"
#include "map_location.h"
#include "physics_world_scripted.h"
#include "alife_simulator.h"
#include "alife_time_manager.h"
#include "UI/UIGameTutorial.h"
#include "string_table.h"
#include "ui/UIInventoryUtilities.h"
#include "alife_object_registry.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "../xrEngine/Rain.h"
#include "game_sv_freemp.h"
#include "inventory_item.h"
#include "Inventory.h"
#include "player_hud.h"

using namespace luabind;
extern int g_sv_server_goodwill;

LPCSTR command_line	()
{
	return		(Core.Params);
}
bool IsDynamicMusic()
{
	return !!psActorFlags.test(AF_DYNAMIC_MUSIC);
}

bool IsImportantSave()
{
	return !!psActorFlags.test(AF_IMPORTANT_SAVE);
}

#ifdef DEBUG
void check_object(CScriptGameObject *object)
{
	try {
		Msg	("check_object %s",object->Name());
	}
	catch(...) {
		object = object;
	}
}

CScriptGameObject *tpfGetActor()
{
	static bool first_time = true;
	if (first_time)
		ai().script_engine().script_log(eLuaMessageTypeError,"Do not use level.actor function!");
	first_time = false;
	
	CActor *l_tpActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (l_tpActor)
		return	(smart_cast<CGameObject*>(l_tpActor)->lua_game_object());
	else
		return	(0);
}

CScriptGameObject *get_object_by_name(LPCSTR caObjectName)
{
	static bool first_time = true;
	if (first_time)
		ai().script_engine().script_log(eLuaMessageTypeError,"Do not use level.object function!");
	first_time = false;
	
	CGameObject		*l_tpGameObject	= smart_cast<CGameObject*>(Level().Objects.FindObjectByName(caObjectName));
	if (l_tpGameObject)
		return		(l_tpGameObject->lua_game_object());
	else
		return		(0);
}
#endif

CScriptGameObject *get_object_by_id(u16 id)
{
	CGameObject* pGameObject = smart_cast<CGameObject*>(Level().Objects.net_Find(id));
	if(!pGameObject)
		return NULL;

	return pGameObject->lua_game_object();
}

LPCSTR get_weather	()
{
	return			(*g_pGamePersistent->Environment().GetWeather());
}

void set_weather	(LPCSTR weather_name, bool forced)
{
#ifdef INGAME_EDITOR
	if (!Device.editor())
#endif // #ifdef INGAME_EDITOR
		g_pGamePersistent->Environment().SetWeather(weather_name,forced);
}

bool set_weather_fx	(LPCSTR weather_name)
{
#ifdef INGAME_EDITOR
	if (!Device.editor())
#endif // #ifdef INGAME_EDITOR
		return		(g_pGamePersistent->Environment().SetWeatherFX(weather_name));
	
#ifdef INGAME_EDITOR
	return			(false);
#endif // #ifdef INGAME_EDITOR
}

bool start_weather_fx_from_time	(LPCSTR weather_name, float time)
{
#ifdef INGAME_EDITOR
	if (!Device.editor())
#endif // #ifdef INGAME_EDITOR
		return		(g_pGamePersistent->Environment().StartWeatherFXFromTime(weather_name, time));
	
#ifdef INGAME_EDITOR
	return			(false);
#endif // #ifdef INGAME_EDITOR
}

bool is_wfx_playing	()
{
	return			(g_pGamePersistent->Environment().IsWFXPlaying());
}

float get_wfx_time	()
{
	return			(g_pGamePersistent->Environment().wfx_time);
}

void stop_weather_fx()
{
	g_pGamePersistent->Environment().StopWFX();
}

void set_time_factor(float time_factor)
{
	if (!OnServer())
		return;

#ifdef INGAME_EDITOR
	if (Device.editor())
		return;
#endif // #ifdef INGAME_EDITOR

	Level().Server->game->SetGameTimeFactor(time_factor);
}

float get_time_factor()
{
	return			(Level().GetGameTimeFactor());
}

void set_game_difficulty(ESingleGameDifficulty dif)
{
	g_SingleGameDifficulty		= dif;
	game_cl_Single* game		= smart_cast<game_cl_Single*>(Level().game); VERIFY(game);
	game->OnDifficultyChanged	();
}
ESingleGameDifficulty get_game_difficulty()
{
	return g_SingleGameDifficulty;
}

u32 get_time_days()
{
	u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
	split_time((g_pGameLevel && Level().game) ? Level().GetGameTime() : ai().alife().time_manager().game_time(), year, month, day, hours, mins, secs, milisecs);
	return			day;
}

u32 get_time_hours()
{
	u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
	split_time((g_pGameLevel && Level().game) ? Level().GetGameTime() : ai().alife().time_manager().game_time(), year, month, day, hours, mins, secs, milisecs);
	return			hours;
}

u32 get_time_hours_mp()
{
	u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
	split_time(Level().GetGameTime(), year, month, day, hours, mins, secs, milisecs);

	return hours;
}

u32 get_time_minutes()
{
	u32 year = 0, month = 0, day = 0, hours = 0, mins = 0, secs = 0, milisecs = 0;
	split_time((g_pGameLevel && Level().game) ? Level().GetGameTime() : ai().alife().time_manager().game_time(), year, month, day, hours, mins, secs, milisecs);
	return			mins;
}

void change_game_time(u32 days, u32 hours, u32 mins)
{
	game_sv_freemp	*tpGame = smart_cast<game_sv_freemp*>(Level().Server->game);
	if(tpGame && ai().get_alife())
	{
		u32 value		= days*86400+hours*3600+mins*60;
		float fValue	= static_cast<float> (value);
		value			*= 1000;//msec		
		g_pGamePersistent->Environment().ChangeGameTime(fValue);
		tpGame->alife().time_manager().change_game_time(value);
	}
}

float high_cover_in_direction(u32 level_vertex_id, const Fvector &direction)
{
	float			y,p;
	direction.getHP	(y,p);
	return			(ai().level_graph().high_cover_in_direction(y,level_vertex_id));
}

float low_cover_in_direction(u32 level_vertex_id, const Fvector &direction)
{
	float			y,p;
	direction.getHP	(y,p);
	return			(ai().level_graph().low_cover_in_direction(y,level_vertex_id));
}

float rain_factor()
{
	return			(g_pGamePersistent->Environment().CurrentEnv->rain_density);
}

float rain_wetness()
{
	return (g_pGamePersistent->Environment().wetness_accum);
}

float rain_hemi()
{
	CEffect_Rain* rain = g_pGamePersistent->pEnvironment->eff_Rain;

	if (rain)
	{
		return rain->GetRainHemi();
	}
	else
	{
		CObject* E = g_pGameLevel->CurrentViewEntity();
		if (E && E->renderable_ROS())
		{
			float* hemi_cube = E->renderable_ROS()->get_luminocity_hemi_cube();
			float hemi_val = _max(hemi_cube[0], hemi_cube[1]);
			hemi_val = _max(hemi_val, hemi_cube[2]);
			hemi_val = _max(hemi_val, hemi_cube[3]);
			hemi_val = _max(hemi_val, hemi_cube[5]);

			return hemi_val;
		}

		return 0.f;
	}
}


u32	vertex_in_direction(u32 level_vertex_id, Fvector direction, float max_distance)
{
	direction.normalize_safe();
	direction.mul	(max_distance);
	Fvector			start_position = ai().level_graph().vertex_position(level_vertex_id);
	Fvector			finish_position = Fvector(start_position).add(direction);
	u32				result = u32(-1);
	ai().level_graph().farthest_vertex_in_direction(level_vertex_id,start_position,finish_position,result,0);
	return			(ai().level_graph().valid_vertex_id(result) ? result : level_vertex_id);
}

Fvector vertex_position(u32 level_vertex_id)
{
	return			(ai().level_graph().vertex_position(level_vertex_id));
}

void map_add_object_spot(u16 id, LPCSTR spot_type, LPCSTR text)
{
	CMapLocation* ml = Level().MapManager().AddMapLocation(spot_type,id);
	if ( xr_strlen(text) )
	{
		ml->SetHint(text);
	}
}

void map_add_object_spot_ser(u16 id, LPCSTR spot_type, LPCSTR text)
{
	CMapLocation* ml = Level().MapManager().AddMapLocation(spot_type,id);
	if( xr_strlen(text) )
			ml->SetHint(text);

	ml->SetSerializable(true);
}

void map_change_spot_hint(u16 id, LPCSTR spot_type, LPCSTR text)
{
	CMapLocation* ml	= Level().MapManager().GetMapLocation(spot_type, id);
	if(!ml)				return;
	ml->SetHint			(text);
}

void map_remove_object_spot(u16 id, LPCSTR spot_type)
{
	Level().MapManager().RemoveMapLocation(spot_type, id);
}

u16 map_has_object_spot(u16 id, LPCSTR spot_type)
{
	return Level().MapManager().HasMapLocation(spot_type, id);
}

bool patrol_path_exists(LPCSTR patrol_path)
{
	return		(!!ai().patrol_paths().path(patrol_path,true));
}

LPCSTR get_name()
{
	return		(*Level().name());
}

void prefetch_sound	(LPCSTR name)
{
	Level().PrefetchSound(name);
}


CClientSpawnManager	&get_client_spawn_manager()
{
	return		(Level().client_spawn_manager());
}
/*
void start_stop_menu(CUIDialogWnd* pDialog, bool bDoHideIndicators)
{
	if(pDialog->IsShown())
		pDialog->HideDialog();
	else
		pDialog->ShowDialog(bDoHideIndicators);
}
*/

void add_dialog_to_render(CUIDialogWnd* pDialog)
{
	CurrentGameUI()->AddDialogToRender(pDialog);
}

void remove_dialog_to_render(CUIDialogWnd* pDialog)
{
	CurrentGameUI()->RemoveDialogToRender(pDialog);
}

void hide_indicators()
{
	if(CurrentGameUI())
	{
		CurrentGameUI()->HideShownDialogs();
		CurrentGameUI()->ShowGameIndicators(false);
		CurrentGameUI()->ShowCrosshair(false);
	}
	psActorFlags.set(AF_GODMODE_RT, TRUE);
}

void hide_indicators_safe()
{
	if(CurrentGameUI())
	{
		CurrentGameUI()->ShowGameIndicators(false);
		CurrentGameUI()->ShowCrosshair(false);

		CurrentGameUI()->OnExternalHideIndicators();
	}
	psActorFlags.set(AF_GODMODE_RT, TRUE);
}

void show_indicators()
{
	if(CurrentGameUI())
	{
		CurrentGameUI()->ShowGameIndicators(true);
		CurrentGameUI()->ShowCrosshair(true);
	}
	psActorFlags.set(AF_GODMODE_RT, FALSE);
}

void show_weapon(bool b)
{
	psHUD_Flags.set	(HUD_WEAPON_RT2, b);
}

bool is_level_present()
{
	return (!!g_pGameLevel);
}

void add_call(const luabind::functor<bool> &condition,const luabind::functor<void> &action)
{
	luabind::functor<bool>		_condition = condition;
	luabind::functor<void>		_action = action;
	CPHScriptCondition	* c=xr_new<CPHScriptCondition>(_condition);
	CPHScriptAction		* a=xr_new<CPHScriptAction>(_action);
	Level().ph_commander_scripts().add_call(c,a);
}

void remove_call(const luabind::functor<bool> &condition,const luabind::functor<void> &action)
{
	CPHScriptCondition	c(condition);
	CPHScriptAction		a(action);
	Level().ph_commander_scripts().remove_call(&c,&a);
}

void add_call(const luabind::object &lua_object, LPCSTR condition,LPCSTR action)
{
//	try{	
//		CPHScriptObjectCondition	*c=xr_new<CPHScriptObjectCondition>(lua_object,condition);
//		CPHScriptObjectAction		*a=xr_new<CPHScriptObjectAction>(lua_object,action);
		luabind::functor<bool>		_condition = object_cast<luabind::functor<bool> >(lua_object[condition]);
		luabind::functor<void>		_action = object_cast<luabind::functor<void> >(lua_object[action]);
		CPHScriptObjectConditionN	*c=xr_new<CPHScriptObjectConditionN>(lua_object,_condition);
		CPHScriptObjectActionN		*a=xr_new<CPHScriptObjectActionN>(lua_object,_action);
		Level().ph_commander_scripts().add_call_unique(c,c,a,a);
//	}
//	catch(...)
//	{
//		Msg("add_call excepted!!");
//	}
}

void remove_call(const luabind::object &lua_object, LPCSTR condition,LPCSTR action)
{
	CPHScriptObjectCondition	c(lua_object,condition);
	CPHScriptObjectAction		a(lua_object,action);
	Level().ph_commander_scripts().remove_call(&c,&a);
}

void add_call(const luabind::object &lua_object, const luabind::functor<bool> &condition,const luabind::functor<void> &action)
{

	CPHScriptObjectConditionN	*c=xr_new<CPHScriptObjectConditionN>(lua_object,condition);
	CPHScriptObjectActionN		*a=xr_new<CPHScriptObjectActionN>(lua_object,action);
	Level().ph_commander_scripts().add_call(c,a);
}

void remove_call(const luabind::object &lua_object, const luabind::functor<bool> &condition,const luabind::functor<void> &action)
{
	CPHScriptObjectConditionN	c(lua_object,condition);
	CPHScriptObjectActionN		a(lua_object,action);
	Level().ph_commander_scripts().remove_call(&c,&a);
}

void remove_calls_for_object(const luabind::object &lua_object)
{
	CPHSriptReqObjComparer c(lua_object);
	Level().ph_commander_scripts().remove_calls(&c);
}

cphysics_world_scripted* physics_world_scripted()
{
	return	get_script_wrapper<cphysics_world_scripted>(*physics_world());
}
CEnvironment *environment()
{
	return		(g_pGamePersistent->pEnvironment);
}

CEnvDescriptor *current_environment(CEnvironment *self)
{
	return		(self->CurrentEnv);
}
extern bool g_bDisableAllInput;
void disable_input()
{
	g_bDisableAllInput = true;
#ifdef DEBUG
	Msg("input disabled");
#endif // #ifdef DEBUG
}
void enable_input()
{
	g_bDisableAllInput = false;
#ifdef DEBUG
	Msg("input enabled");
#endif // #ifdef DEBUG
}

void spawn_phantom(const Fvector &position)
{
	Level().spawn_item("m_phantom", position, u32(-1), u16(-1), false);
}

Fbox get_bounding_volume()
{
	return Level().ObjectSpace.GetBoundingVolume();
}

void iterate_sounds					(LPCSTR prefix, u32 max_count, const CScriptCallbackEx<void> &callback)
{
	for (int j=0, N = _GetItemCount(prefix); j<N; ++j) {
		string_path					fn, s;
		LPSTR						S = (LPSTR)&s;
		_GetItem					(prefix,j,s);
		if (FS.exist(fn,"$game_sounds$",S,".ogg"))
			callback				(prefix);

		for (u32 i=0; i<max_count; ++i)
		{
			string_path					name;
			xr_sprintf					(name,"%s%d",S,i);
			if (FS.exist(fn,"$game_sounds$",name,".ogg"))
				callback			(name);
		}
	}
}

void iterate_sounds1				(LPCSTR prefix, u32 max_count, luabind::functor<void> functor)
{
	CScriptCallbackEx<void>		temp;
	temp.set					(functor);
	iterate_sounds				(prefix,max_count,temp);
}

void iterate_sounds2				(LPCSTR prefix, u32 max_count, luabind::object object, luabind::functor<void> functor)
{
	CScriptCallbackEx<void>		temp;
	temp.set					(functor,object);
	iterate_sounds				(prefix,max_count,temp);
}

#include "actoreffector.h"
float add_cam_effector(LPCSTR fn, int id, bool cyclic, LPCSTR cb_func)
{
	CAnimatorCamEffectorScriptCB* e		= xr_new<CAnimatorCamEffectorScriptCB>(cb_func);
	e->SetType					((ECamEffectorType)id);
	e->SetCyclic				(cyclic);
	e->Start					(fn);
	Actor()->Cameras().AddCamEffector(e);
	return						e->GetAnimatorLength();
}

float add_cam_effector2(LPCSTR fn, int id, bool cyclic, LPCSTR cb_func, float cam_fov)
{
	CAnimatorCamEffectorScriptCB* e		= xr_new<CAnimatorCamEffectorScriptCB>(cb_func);
	e->m_bAbsolutePositioning	= true;
	e->m_fov					= cam_fov;
	e->SetType					((ECamEffectorType)id);
	e->SetCyclic				(cyclic);
	e->Start					(fn);
	Actor()->Cameras().AddCamEffector(e);
	return						e->GetAnimatorLength();
}

void remove_cam_effector(int id)
{
	Actor()->Cameras().RemoveCamEffector((ECamEffectorType)id );
}
		
float get_snd_volume()
{
	return psSoundVFactor;
}

void set_snd_volume(float v)
{
	psSoundVFactor = v;
	clamp(psSoundVFactor,0.0f,1.0f);
}
#include "actor_statistic_mgr.h"
void add_actor_points(LPCSTR sect, LPCSTR detail_key, int cnt, int pts)
{
	return Actor()->StatisticMgr().AddPoints(sect, detail_key, cnt, pts);
}

void add_actor_points_str(LPCSTR sect, LPCSTR detail_key, LPCSTR str_value)
{
	return Actor()->StatisticMgr().AddPoints(sect, detail_key, str_value);
}

int get_actor_points(LPCSTR sect)
{
	return Actor()->StatisticMgr().GetSectionPoints(sect);
}



#include "ActorEffector.h"
void add_complex_effector(LPCSTR section, int id)
{
	AddEffector(Actor(),id, section);
}

void remove_complex_effector(int id)
{
	RemoveEffector(Actor(),id);
}

#include "postprocessanimator.h"
void add_pp_effector(LPCSTR fn, int id, bool cyclic)
{
	CPostprocessAnimator* pp		= xr_new<CPostprocessAnimator>(id, cyclic);
	pp->Load						(fn);
	Actor()->Cameras().AddPPEffector	(pp);
}

void remove_pp_effector(int id)
{
	CPostprocessAnimator*	pp	= smart_cast<CPostprocessAnimator*>(Actor()->Cameras().GetPPEffector((EEffectorPPType)id));

	if(pp) pp->Stop(1.0f);

}

void set_pp_effector_factor(int id, float f, float f_sp)
{
	CPostprocessAnimator*	pp	= smart_cast<CPostprocessAnimator*>(Actor()->Cameras().GetPPEffector((EEffectorPPType)id));

	if(pp) pp->SetDesiredFactor(f,f_sp);
}

void set_pp_effector_factor2(int id, float f)
{
	CPostprocessAnimator*	pp	= smart_cast<CPostprocessAnimator*>(Actor()->Cameras().GetPPEffector((EEffectorPPType)id));

	if(pp) pp->SetCurrentFactor(f);
}

#include "relation_registry.h"

int g_community_goodwill(LPCSTR _community, int _entity_id)
 {
	 CHARACTER_COMMUNITY c;
	 c.set					(_community);

 	return RELATION_REGISTRY().GetCommunityGoodwill(c.index(), u16(_entity_id));
 }

void g_set_community_goodwill(LPCSTR _community, int _entity_id, int val)
{
	CHARACTER_COMMUNITY	c;
	c.set					(_community);
	RELATION_REGISTRY().SetCommunityGoodwill(c.index(), u16(_entity_id), val);
}

void g_change_community_goodwill(LPCSTR _community, int _entity_id, int val)
{
	CHARACTER_COMMUNITY	c;
	c.set					(_community);
	RELATION_REGISTRY().ChangeCommunityGoodwill(c.index(), u16(_entity_id), val);
}

int g_get_community_relation( LPCSTR comm_from, LPCSTR comm_to )
{
	CHARACTER_COMMUNITY	community_from;
	community_from.set( comm_from );
	CHARACTER_COMMUNITY	community_to;
	community_to.set( comm_to );

	return RELATION_REGISTRY().GetCommunityRelation( community_from.index(), community_to.index() );
}

bool HavePlayersNearby(Fvector pos, float near_pos)
{
	for (const auto& player : Level().game->players)
	{
		if (player.first.value() == 0)
			continue;

		if (!player.second)
			continue;

		CObject* obj = Level().Objects.net_Find(player.second->GameID);
		if (!obj)
			continue;

		CActor* pA = smart_cast<CActor*>(obj);
		if (!pA)
			continue;

		if (!pA->g_Alive())
			continue;

		if (pos.distance_to(pA->Position()) < near_pos)
		{
			return true;
			break;
		}

	}

	return false;
}

void g_set_community_relation( LPCSTR comm_from, LPCSTR comm_to, int value )
{
	CHARACTER_COMMUNITY	community_from;
	community_from.set( comm_from );
	CHARACTER_COMMUNITY	community_to;
	community_to.set( comm_to );

	RELATION_REGISTRY().SetCommunityRelation( community_from.index(), community_to.index(), value );
}

int g_get_general_goodwill_between ( u16 from, u16 to)
{
	CHARACTER_GOODWILL presonal_goodwill		= RELATION_REGISTRY().GetGoodwill(from, to); VERIFY(presonal_goodwill != NO_GOODWILL);

	CSE_ALifeTraderAbstract* from_obj	= smart_cast<CSE_ALifeTraderAbstract*>(ai().alife().objects().object(from));
	CSE_ALifeTraderAbstract* to_obj		= smart_cast<CSE_ALifeTraderAbstract*>(ai().alife().objects().object(to));

	if (!from_obj||!to_obj){
		ai().script_engine().script_log		(ScriptStorage::eLuaMessageTypeError,"RELATION_REGISTRY::get_general_goodwill_between  : cannot convert obj to CSE_ALifeTraderAbstract!");
		return (0);
	}	
	CHARACTER_GOODWILL community_to_obj_goodwill		= RELATION_REGISTRY().GetCommunityGoodwill	(from_obj->Community(), to					);
	CHARACTER_GOODWILL community_to_community_goodwill	= RELATION_REGISTRY().GetCommunityRelation	(from_obj->Community(), to_obj->Community()	);
	
	return presonal_goodwill + community_to_obj_goodwill + community_to_community_goodwill;
}

u32 vertex_id	(Fvector position)
{
	return	(ai().level_graph().vertex_id(position));
}

u32 render_get_dx_level()
{
	return ::Render->get_dx_level();
}

CUISequencer* g_tutorial = NULL;
CUISequencer* g_tutorial2 = NULL;

void start_tutorial(LPCSTR name)
{
	if(g_tutorial){
		VERIFY				(!g_tutorial2);
		g_tutorial2			= g_tutorial;
	};

	g_tutorial							= xr_new<CUISequencer>();
	g_tutorial->Start					(name);
	if(g_tutorial2)
		g_tutorial->m_pStoredInputReceiver = g_tutorial2->m_pStoredInputReceiver;

}

void stop_tutorial()
{
	if(g_tutorial)
		g_tutorial->Stop();	
}

u32 PlayHudMotion(u8 hand, LPCSTR itm_name, LPCSTR anm_name, bool bMixIn = true, float speed = 1.f, LPCSTR attach_visual = nullptr)
{
	return g_player_hud->script_anim_play(hand, itm_name, anm_name, bMixIn, speed, attach_visual);
}

void StopHudMotion()
{
	g_player_hud->StopScriptAnim();
}

float MotionLength(LPCSTR section, LPCSTR name, float speed)
{
	return g_player_hud->motion_length_script(section, name, speed);
}

bool AllowHudMotion()
{
	return g_player_hud->allow_script_anim();
}

void PlayBlendAnm(LPCSTR name, u8 part, float speed, float power, bool bLooped, bool no_restart)
{
	g_player_hud->PlayBlendAnm(name, part, speed, power, bLooped, no_restart);
}

void StopBlendAnm(LPCSTR name, bool bForce)
{
	g_player_hud->StopBlendAnm(name, bForce);
}

void StopAllBlendAnms(bool bForce)
{
	g_player_hud->StopAllBlendAnms(bForce);
}

float SetBlendAnmTime(LPCSTR name, float time)
{
	return g_player_hud->SetBlendAnmTime(name, time);
}


LPCSTR translate_string(LPCSTR str)
{
	return *CStringTable().translate(str);
}

bool has_active_tutotial()
{
	return (g_tutorial!=NULL);
}

bool is_dedicated()
{
	return g_dedicated_server;
}

void print_msg(LPCSTR str)
{
	Msg("[lua] %s", str);
}

bool check_params(LPCSTR p)
{
	return strstr(Core.Params, p);
}

int GetGoodwill()
{
	if (g_pGamePersistent->GameType() == eGameIDFreeMp)
		return g_sv_server_goodwill;
	else
		return 0;
}

void ChangeSVGoodwill(int delta)
{
	g_sv_server_goodwill += delta;
}

CScriptGameObject *get_object_by_client(u32 clientID)
{
	xrClientData* xrCData = Level().Server->ID_to_client(clientID);
	if (!xrCData || !xrCData->owner) return NULL;

	CGameObject* pGameObject = smart_cast<CGameObject*>(Level().Objects.net_Find(xrCData->owner->ID));
	if (!pGameObject)
		return NULL;

	return pGameObject->lua_game_object();
}

int get_local_player_id()
{
	return Game().local_player->GameID;
}

int get_g_actor_id()
{
	if (!Actor())
		return -1;

	return Actor()->ID();
}

void sv_teleport_player(u32 clientID, const Fvector3 pos)
{
	game_sv_mp* srv = smart_cast<game_sv_mp*>(Level().Server->game);
	if (srv)
	{
		srv->TeleportPlayerTo(clientID, pos);
	}
}

void sv_teleport_player2(u32 clientID, const Fvector3 pos, const Fvector3 dir)
{
	game_sv_mp* srv = smart_cast<game_sv_mp*>(Level().Server->game);
	if (srv)
	{
		srv->TeleportPlayerTo(clientID, pos, dir);
	}
}

void give_money_to_actor(u16 gameid, s32 money)
{
	if (!OnServer())
		return;

	game_sv_freemp* freemp = smart_cast<game_sv_freemp*>(Level().Server->game);

	if (freemp)
	{
		game_PlayerState* ps = freemp->get_eid(gameid);
		if (ps)
			freemp->AddMoneyToPlayer(ps, money);

		xrClientData* data = (xrClientData*)freemp->get_client(gameid);
		if (data)
		{
			string32 tmp = { 0 };

			NET_Packet packet;
			freemp->GenerateGameMessage(packet);
			packet.w_u32(GAME_EVENT_NEWS_MONEY_MESSAGE);
			shared_str news_name = *CStringTable().translate("general_in_money");
			packet.w_stringZ(news_name);
			packet.w_stringZ(itoa(money, tmp, 10));
			packet.w_stringZ("ui_inGame2_Dengi_polucheni");
			freemp->server().SendTo(data->ID, packet, net_flags(true));
		}

	}
};

void remove_money_to_actor(u16 gameid, s32 money)
{
	if (!OnServer())
		return;

	game_sv_freemp* freemp = smart_cast<game_sv_freemp*>(Level().Server->game);

	if (freemp)
	{
		game_PlayerState* ps = freemp->get_eid(gameid);
		xrClientData* data = (xrClientData*)freemp->get_client(gameid);
		if (ps)
			freemp->AddMoneyToPlayer(ps, -money);

		if (data)
		{
			string32 tmp = { 0 };

			NET_Packet packet;
			freemp->GenerateGameMessage(packet);
			packet.w_u32(GAME_EVENT_NEWS_MONEY_MESSAGE);
			shared_str news_name = *CStringTable().translate("general_out_money");
			packet.w_stringZ(news_name);
			packet.w_stringZ(itoa(money, tmp, 10));
			packet.w_stringZ("ui_inGame2_Dengi_otdani");
			freemp->server().SendTo(data->ID, packet, net_flags(true));
		}
	}
};

#include "actor_mp_client.h"

void object_give_to_actor(u16 gameid, LPCSTR name, u16 count)
{
	if (!OnServer())
		return;

	game_sv_freemp* freemp = smart_cast<game_sv_freemp*>(Level().Server->game);

	if (freemp)
		for (int i = 0; i != count; i++)
			freemp->SpawnItemToActor(gameid, name);

	if (freemp)
	{
		xrClientData* data = (xrClientData*)freemp->get_client(gameid);
		if (data)
		{
			shared_str news_name;

			NET_Packet packet;
			freemp->GenerateGameMessage(packet);
			packet.w_u32(GAME_EVENT_NEWS_MESSAGE);
			news_name = *CStringTable().translate("general_in_item");

			packet.w_stringZ(name);
			packet.w_stringZ(news_name);
			packet.w_u16(count);
			packet.w_stringZ("ui_inGame2_Predmet_poluchen");

			freemp->server().SendTo(data->ID, packet, net_flags(true));
		}
	}
}

void change_team(u32 ClientID, u16 team)
{
		if (OnClient()) 
			return;

	xrClientData* CL = smart_cast<xrClientData*>(Level().Server->GetClientByID(ClientID));

	if (CL != nullptr)
	{
		CL->ps->team = team;
		Level().Server->game->signal_Syncronize();
	}
}

void object_destroy(CScriptGameObject* object)
{
	if (!OnServer() || !object)
		return;

	NET_Packet P;
	P.w_begin(M_EVENT);
	P.w_u32(Device.dwTimeGlobal - 2 * NET_Latency);
	P.w_u16(GE_DESTROY);
	P.w_u16(object->ID());
	Level().Send(P, net_flags(TRUE, TRUE));
}

void send_news_item_drop(u16 gameid, LPCSTR name, int count)
{
	if (!OnServer())
		return;

	game_sv_freemp* freemp = smart_cast<game_sv_freemp*>(Level().Server->game);

	if (freemp)
	{
		xrClientData* data = (xrClientData*)freemp->get_client(gameid);
		if (data)
		{
			NET_Packet packet;
			freemp->GenerateGameMessage(packet);

			packet.w_u32(GAME_EVENT_NEWS_MESSAGE);


			shared_str news_name = *CStringTable().translate("general_out_item");


			packet.w_stringZ(name);
			packet.w_stringZ(news_name);
			packet.w_u16(count);
			packet.w_stringZ("ui_inGame2_Predmet_otdan");
			freemp->server().SendTo(data->ID, packet, net_flags(true));
		}
	}
}

u32 get_client_by_gameid(u16 GameID)
{
	for (auto player : Level().game->players)
	{
		if (player.second->GameID == GameID)
			return player.first.value();
	}

	return 0;
}

u8 get_actor_team()
{
	Msg("Actor team: %d", Actor()->g_Team());
	return Actor()->g_Team();
}

// script events

void send_script_event_to_server(NET_Packet& P)
{
	Level().Send(P, net_flags(TRUE, TRUE));
}

NET_Packet* get_last_client_event()
{
	return Level().GetLastClientScriptEvent();
}

void pop_last_client_event()
{
	Level().PopLastClientScriptEvent();
}

u32 get_size_client_events()
{
	return Level().GetSizeClientScriptEvent();
}


void send_script_event_to_client(u32 cleintId, NET_Packet& P)
{
	R_ASSERT2(OnServer(), "Avaliable only on server");
	Level().Server->SendTo(ClientID(cleintId), P, net_flags(TRUE, TRUE));
}

void send_script_event_broadcast(NET_Packet& P)
{
	R_ASSERT2(OnServer(), "Avaliable only on server");
	Level().Server->SendBroadcast(BroadcastCID, P, net_flags(TRUE, TRUE));
}

ScriptEvent* get_last_server_event()
{
	return Level().Server->GetLastServerScriptEvent();
}

void pop_last_server_event()
{
	Level().Server->PopLastServerScriptEvent();
}

u32 get_size_server_events()
{
	return Level().Server->GetSizeServerScriptEvent();
}

void send_event_key_press(int dik)
{
	Level().IR_OnKeyboardPress(dik);
}
void send_event_key_release(int dik)
{
	Level().IR_OnKeyboardRelease(dik);
}
void send_event_key_hold(int dik)
{
	Level().IR_OnKeyboardHold(dik);
}
void send_event_mouse_wheel(int vol)
{
	Level().IR_OnMouseWheel(vol);
}

void block_player_action(EGameActions dik)
{
	Actor()->block_action(dik);
}

void unblock_player_action(EGameActions dik)
{
	Actor()->unblock_action(dik);
}

void add_effector_to_client(u32 clientId, LPCSTR eff)
{
	NET_Packet P;
	P.w_begin(M_SET_EFFECTOR);
	P.w_stringZ(eff);
	Level().Server->SendTo(clientId, P);
}

void add_snd_to_client(u32 clientId, LPCSTR snd)
{
	NET_Packet P;
	P.w_begin(M_SET_SND_EFF);
	P.w_stringZ(snd);
	Level().Server->SendTo(clientId, P);
}

#pragma optimize("s",on)
void CLevel::script_register(lua_State *L)
{
	class_<CEnvDescriptor>("CEnvDescriptor")
		.def_readonly("fog_density",			&CEnvDescriptor::fog_density)
		.def_readonly("far_plane",				&CEnvDescriptor::far_plane),

	class_<CEnvironment>("CEnvironment")
		.def("current",							current_environment);

	module(L,"level")
	[
		// obsolete\deprecated
		def("object_by_id",						get_object_by_id),
#ifdef DEBUG
		def("debug_object",						get_object_by_name),
		def("debug_actor",						tpfGetActor),
		def("check_object",						check_object),
#endif
		
		def("send_event_key_press", &send_event_key_press),
		def("send_event_key_release", &send_event_key_release),
		def("send_event_key_hold", &send_event_key_hold),
		def("send_event_mouse_wheel", &send_event_mouse_wheel),

		def("block_player_action", &block_player_action),
		def("unblock_player_action", &unblock_player_action),
		def("get_client_by_gameid", get_client_by_gameid),

def("get_weather", get_weather),
def("set_weather", set_weather),
def("set_weather_fx", set_weather_fx),
def("start_weather_fx_from_time", start_weather_fx_from_time),
def("is_wfx_playing", is_wfx_playing),
def("get_wfx_time", get_wfx_time),
def("stop_weather_fx", stop_weather_fx),

def("environment", environment),

def("set_time_factor", set_time_factor),
def("get_time_factor", get_time_factor),

def("set_game_difficulty", set_game_difficulty),
def("get_game_difficulty", get_game_difficulty),

def("get_time_days", get_time_days),
def("get_time_hours", get_time_hours),
def("get_time_minutes", get_time_minutes),
def("change_game_time", change_game_time),

def("high_cover_in_direction", high_cover_in_direction),
def("low_cover_in_direction", low_cover_in_direction),
def("vertex_in_direction", vertex_in_direction),
def("rain_factor", rain_factor),
def("patrol_path_exists", patrol_path_exists),
def("vertex_position", vertex_position),
def("name", get_name),
def("prefetch_sound", prefetch_sound),

def("client_spawn_manager", get_client_spawn_manager),

def("rain_wetness", rain_wetness),
def("rain_hemi", rain_hemi),

def("map_add_object_spot_ser", map_add_object_spot_ser),
def("map_add_object_spot", map_add_object_spot),
//-		def("map_add_object_spot_complex",		map_add_object_spot_complex),
def("map_remove_object_spot", map_remove_object_spot),
def("map_has_object_spot", map_has_object_spot),
def("map_change_spot_hint", map_change_spot_hint),

def("add_dialog_to_render", add_dialog_to_render),
def("remove_dialog_to_render", remove_dialog_to_render),
def("hide_indicators", hide_indicators),
def("hide_indicators_safe", hide_indicators_safe),

def("show_indicators", show_indicators),
def("show_weapon", show_weapon),
def("add_call", ((void (*) (const luabind::functor<bool>&, const luabind::functor<void>&))& add_call)),
def("add_call", ((void (*) (const luabind::object&, const luabind::functor<bool>&, const luabind::functor<void>&))& add_call)),
def("add_call", ((void (*) (const luabind::object&, LPCSTR, LPCSTR))& add_call)),
def("remove_call", ((void (*) (const luabind::functor<bool>&, const luabind::functor<void>&))& remove_call)),
def("remove_call", ((void (*) (const luabind::object&, const luabind::functor<bool>&, const luabind::functor<void>&))& remove_call)),
def("remove_call", ((void (*) (const luabind::object&, LPCSTR, LPCSTR))& remove_call)),
def("remove_calls_for_object", remove_calls_for_object),
def("present", is_level_present),
def("disable_input", disable_input),
def("enable_input", enable_input),
def("spawn_phantom", spawn_phantom),

def("get_bounding_volume", get_bounding_volume),

def("iterate_sounds", &iterate_sounds1),
def("iterate_sounds", &iterate_sounds2),
def("physics_world", &physics_world_scripted),
def("get_snd_volume", &get_snd_volume),
def("set_snd_volume", &set_snd_volume),
def("add_cam_effector", &add_cam_effector),
def("add_cam_effector2", &add_cam_effector2),
def("remove_cam_effector", &remove_cam_effector),
def("add_pp_effector", &add_pp_effector),
def("set_pp_effector_factor", &set_pp_effector_factor),
def("set_pp_effector_factor", &set_pp_effector_factor2),
def("remove_pp_effector", &remove_pp_effector),

def("add_complex_effector", &add_complex_effector),
def("remove_complex_effector", &remove_complex_effector),

def("vertex_id", &vertex_id),

def("game_id", &GameID),

// new for mp
		def("get_object_by_client", &get_object_by_client),
		def("get_local_player_id", &get_local_player_id),
		def("get_g_actor_id", &get_g_actor_id)
	],

	module(L, "mp")
	[
		def("sv_teleport_player", &sv_teleport_player),
		def("sv_teleport_player2", &sv_teleport_player2),

		def("give_money", &give_money_to_actor),
		def("remove_money", &remove_money_to_actor),
		def("object_destroy", object_destroy),
		def("object_give_to_actor", object_give_to_actor),
		def("send_news_item_drop", send_news_item_drop),
		def("change_team", change_team),
		def("get_actor_team", get_actor_team),
		def("get_game_time_mp", get_time_hours_mp),
		def("add_cl_eff", &add_effector_to_client),
		def("add_snd_client", &add_snd_to_client),
		def("have_nearby_player", &HavePlayersNearby)
	],
	

	module(L, "script_events")
	[
		def("send_to_server", &send_script_event_to_server),
		def("send_to_client", &send_script_event_to_client),
		def("send_broadcast", &send_script_event_broadcast),

		def("get_last_client_event", &get_last_client_event),
		def("pop_last_client_event", &pop_last_client_event),
		def("get_size_client_events", &get_size_client_events),

		def("get_last_server_event", &get_last_server_event),
		def("pop_last_server_event", &pop_last_server_event),
		def("get_size_server_events", &get_size_server_events)
	];

	module(L,"actor_stats")
	[
		def("add_points",						&add_actor_points),
		def("add_points_str",					&add_actor_points_str),
		def("get_points",						&get_actor_points)
	];

	module(L)
	[
		def("command_line",						&command_line),
		def("IsGameTypeSingle",					&IsGameTypeSingle),
		def("IsDynamicMusic",					&IsDynamicMusic),
		def("render_get_dx_level",				&render_get_dx_level),
		def("IsImportantSave",					&IsImportantSave),
		def("print_msg",						&print_msg),
		def("IsDedicated",						&is_dedicated),
		def("OnClient",							&OnClient),
		def("OnServer",							&OnServer),
		def("CheckParams", 					&check_params),	
		def("GetGoodwill",						&GetGoodwill),
		def("ChangeGoodwill",					&ChangeSVGoodwill)
	];

	module(L,"relation_registry")
	[
		def("community_goodwill",				&g_community_goodwill),
		def("set_community_goodwill",			&g_set_community_goodwill),
		def("change_community_goodwill",		&g_change_community_goodwill),
		
		def("community_relation",				&g_get_community_relation),
		def("set_community_relation",			&g_set_community_relation),
		def("get_general_goodwill_between",		&g_get_general_goodwill_between)
	];
	module(L,"game")
	[
	class_< xrTime >("CTime")
		.enum_("date_format")
		[
			value("DateToDay",		int(InventoryUtilities::edpDateToDay)),
			value("DateToMonth",	int(InventoryUtilities::edpDateToMonth)),
			value("DateToYear",		int(InventoryUtilities::edpDateToYear))
		]
		.enum_("time_format")
		[
			value("TimeToHours",	int(InventoryUtilities::etpTimeToHours)),
			value("TimeToMinutes",	int(InventoryUtilities::etpTimeToMinutes)),
			value("TimeToSeconds",	int(InventoryUtilities::etpTimeToSeconds)),
			value("TimeToMilisecs",	int(InventoryUtilities::etpTimeToMilisecs))
		]
		.def(						constructor<>()				)
		.def(						constructor<const xrTime&>())
		.def(const_self <			xrTime()					)
		.def(const_self <=			xrTime()					)
		.def(const_self >			xrTime()					)
		.def(const_self >=			xrTime()					)
		.def(const_self ==			xrTime()					)
		.def(self +					xrTime()					)
		.def(self -					xrTime()					)

		.def("diffSec"				,&xrTime::diffSec_script)
		.def("add"					,&xrTime::add_script)
		.def("sub"					,&xrTime::sub_script)

		.def("setHMS"				,&xrTime::setHMS)
		.def("setHMSms"				,&xrTime::setHMSms)
		.def("set"					,&xrTime::set)
		.def("get"					,&xrTime::get, out_value<2>() + out_value<3>() + out_value<4>() + out_value<5>() + out_value<6>() + out_value<7>() + out_value<8>())
		.def("dateToString"			,&xrTime::dateToString)
		.def("timeToString"			,&xrTime::timeToString),
		// declarations
		def("time",					get_time),
		def("get_game_time",		get_time_struct),
//		def("get_surge_time",	Game::get_surge_time),
//		def("get_object_by_name",Game::get_object_by_name),
	
	def("start_tutorial",		&start_tutorial),
	def("stop_tutorial",		&stop_tutorial),
	def("has_active_tutorial",	&has_active_tutotial),
	def("translate_string", &translate_string),
	def("play_hud_motion", PlayHudMotion),
	def("stop_hud_motion", StopHudMotion),
	def("get_motion_length", MotionLength),
	def("hud_motion_allowed", AllowHudMotion),
	def("play_hud_anm", PlayBlendAnm),
	def("stop_hud_anm", StopBlendAnm),
	def("stop_all_hud_anms", StopAllBlendAnms),
	def("set_hud_anm_time", SetBlendAnmTime)

	];
}
