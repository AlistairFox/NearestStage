#pragma once

#include "UIGameMP.h"

class game_cl_freemp;
class CUITextWnd;
class CUIAMode;

class CUIGameFMP : public UIGameMP
{
private:
	game_cl_freemp *	m_game;
	typedef UIGameMP inherited;

protected:
	CUITextWnd*			m_stats;
	CUIAMode* m_anims;
public:
	SDrawStaticStruct* m_game_objective;
				CUIGameFMP();
	virtual		~CUIGameFMP();

	virtual	void Init(int stage);

	virtual void SetClGame(game_cl_GameState* g);

	virtual void HideShownDialogs();

	virtual void	_BCL OnFrame();

	virtual bool IR_UIOnKeyboardPress(int dik);

	virtual void StartUpgrade(CInventoryOwner* pActorInv, CInventoryOwner* pMech);
};
