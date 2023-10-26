#include "stdafx.h"
#pragma hdrstop

#include "blender_hud_thirst.h"

CBlender_Hud_Thirst::CBlender_Hud_Thirst() { description.CLS = 0; }
CBlender_Hud_Thirst::~CBlender_Hud_Thirst() {}

void CBlender_Hud_Thirst::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);
	switch (C.iElement)
	{
	case 0:
		C.r_Pass("stub_notransform_aa_AA", "hud_power", false, false, false);
		C.r_dx10Texture("s_image", r2_RT_generic0);
		C.r_dx10Texture("s_hud_power", "shaders\\hud_mask\\hud_power");
		C.r_dx10Sampler("smp_rtlinear");
		C.r_End();
		break;
	}
}