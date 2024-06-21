////////////////////////////////////////////////////////////////////////////
//	Module 		: DynamicHudGlass.cpp
//	Created 	: 12.05.2021
//  Modified 	: 12.05.2021
//	Author		: Dance Maniac (M.F.S. Team)
//	Description : Dynamic HUD glass functions and variables
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "DynamicHudGlass.h"
#include "ActorHelmet.h"
#include "CustomOutfit.h"
#include "Actor.h"
#include "Inventory.h"

namespace DynamicHudGlass
{
	bool DynamicHudGlassEnabled = false;
	int	HudGlassElement = 0;

	void UpdateDynamicHudGlass()
	{
		CHelmet* helmet = smart_cast<CHelmet*>(Actor()->inventory().ItemFromSlot(HELMET_SLOT));
		CCustomOutfit* outfit = smart_cast<CCustomOutfit*>(Actor()->inventory().ItemFromSlot(OUTFIT_SLOT));

		if (helmet)
		{
			float condition = helmet->GetCondition();
			HudGlassElement = 1;
			bool HelmetHasGlass = helmet->m_b_HasGlass;

			if (HelmetHasGlass)
			{
				DynamicHudGlassEnabled = true;
			}
			else
			{
				DynamicHudGlassEnabled = false;
			}

			if (condition < 0.90)
			{
				if (condition > 0.80)
					HudGlassElement = 2;
				else if (condition > 0.70)
					HudGlassElement = 3;
				else if (condition > 0.60)
					HudGlassElement = 4;
				else if (condition > 0.50)
					HudGlassElement = 5;
				else if (condition > 0.40)
					HudGlassElement = 6;
				else if (condition > 0.30)
					HudGlassElement = 7;
				else if (condition > 0.20)
					HudGlassElement = 8;
				else if (condition > 0.10)
					HudGlassElement = 9;
				else
					HudGlassElement = 10;
			}
		}
		else
			if (outfit)
			{
				float condition = outfit->GetCondition();
				bool OutfitHasGlass = outfit->m_b_HasGlass;
				HudGlassElement = 1;

				if (OutfitHasGlass)
				{
					DynamicHudGlassEnabled = true;
				}
				else
				{
					DynamicHudGlassEnabled = false;
				}

				if (condition < 0.90)
				{
					if (condition > 0.80)
						HudGlassElement = 2;
					else if (condition > 0.70)
						HudGlassElement = 3;
					else if (condition > 0.60)
						HudGlassElement = 4;
					else if (condition > 0.50)
						HudGlassElement = 5;
					else if (condition > 0.40)
						HudGlassElement = 6;
					else if (condition > 0.30)
						HudGlassElement = 7;
					else if (condition > 0.20)
						HudGlassElement = 8;
					else if (condition > 0.10)
						HudGlassElement = 9;
					else
						HudGlassElement = 10;
				}
			}
			else
			{
				HudGlassElement = 0;
				DynamicHudGlassEnabled = false;
			}
	}
}