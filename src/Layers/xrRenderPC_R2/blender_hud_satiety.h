#pragma once

class CBlender_Hud_Satiety : public IBlender
{
public:
	virtual		LPCSTR		getComment() { return "Hud Stamina"; }
	virtual		BOOL		canBeDetailed() { return FALSE; }
	virtual		BOOL		canBeLMAPped() { return FALSE; }

	virtual		void		Compile(CBlender_Compile& C);

	CBlender_Hud_Satiety();
	virtual ~CBlender_Hud_Satiety();
};