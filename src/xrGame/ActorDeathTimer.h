#pragma once
#include "Actor.h"


class CDeathTimer
{

public:
	CDeathTimer(CActor* act);
	~CDeathTimer();

	void Renderable();
	void UpdateTimer();
	void StartTimer() { TimerIsStart = true; }
	void StopTimer();


	bool			TimerIsStart;

private:
	CActor*			pActor;
	u32				DeathTimerRender;
	u32				Timer;
};