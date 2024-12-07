#include "stdafx.h"
#include "ActorDeathTimer.h"
#include <ui_base.h>

CDeathTimer::CDeathTimer(CActor* act) : pActor(act)
{
	TimerIsStart = false;
	DeathTimerRender = 60;
	Timer = 0;
}

CDeathTimer::~CDeathTimer()
{
}

void CDeathTimer::Renderable()
{
	if (!pActor->g_Alive())
		return;

	if (TimerIsStart)
	{
		UI().Font().pFontGraffiti32Russian->SetInterval(1.f, 0.5f);
		UI().Font().pFontGraffiti32Russian->OutSetI(-0.01, -0.13);
		UI().Font().pFontGraffiti32Russian->SetColor(D3DCOLOR_XRGB(255, 0, 0));
		UI().Font().pFontGraffiti32Russian->OutNext("Вы умрете через: %d секунд", DeathTimerRender);;
	}
}

void CDeathTimer::UpdateTimer()
{
	if (!pActor->g_Alive())
		return;

	if (!TimerIsStart)
		return;


	if (Timer <= Device.dwTimeGlobal)
	{
		Timer = Device.dwTimeGlobal + 1000;
		DeathTimerRender--;
	}
	if (DeathTimerRender < 1)
		pActor->NeedKillPlayer();
}

void CDeathTimer::StopTimer()
{
	TimerIsStart = false;
	DeathTimerRender = 60;
}
