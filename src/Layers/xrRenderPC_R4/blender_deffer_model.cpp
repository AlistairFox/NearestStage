#include "stdafx.h"
#pragma hdrstop

#include "../xrRender/uber_deffer.h"
#include "Blender_deffer_model.h"

CBlender_deffer_model::CBlender_deffer_model	()	{	
	description.CLS		= B_MODEL;	
	description.version	= 2;
	oTessellation.Count         = 4;
	oTessellation.IDselected	= 0;
	oAREF.value			= 32;
	oAREF.min			= 0;
	oAREF.max			= 255;
	oBlend.value		= false;
}
CBlender_deffer_model::~CBlender_deffer_model	()	{	}

void	CBlender_deffer_model::Save	(	IWriter& fs )
{
	IBlender::Save		(fs);
	xrPWRITE_PROP		(fs,"Use alpha-channel",	xrPID_BOOL,		oBlend);
	xrPWRITE_PROP		(fs,"Alpha ref",			xrPID_INTEGER,	oAREF);
	xrP_TOKEN::Item	I;
	xrPWRITE_PROP	(fs,"Tessellation",	xrPID_TOKEN, oTessellation);
	I.ID = 0; xr_strcpy(I.str,"NO_TESS");	fs.w		(&I,sizeof(I));
	I.ID = 1; xr_strcpy(I.str,"TESS_PN");	fs.w		(&I,sizeof(I));
	I.ID = 2; xr_strcpy(I.str,"TESS_HM");	fs.w		(&I,sizeof(I));
	I.ID = 3; xr_strcpy(I.str,"TESS_PN+HM");	fs.w		(&I,sizeof(I));
}
void	CBlender_deffer_model::Load	(	IReader& fs, u16 version )
{
	IBlender::Load		(fs,version);

	switch (version)	
	{
	case 0: 
		oAREF.value			= 32;
		oAREF.min			= 0;
		oAREF.max			= 255;
		oBlend.value		= FALSE;
		break;
	case 1:
	default:
		xrPREAD_PROP	(fs,xrPID_BOOL,		oBlend);
		xrPREAD_PROP	(fs,xrPID_INTEGER,	oAREF);
		break;
	}
	if (version>1)
	{
		xrPREAD_PROP(fs,xrPID_TOKEN,oTessellation);
	}
}

void	CBlender_deffer_model::Compile(CBlender_Compile& C)
{
	IBlender::Compile		(C);

	BOOL	bForward		= FALSE;
	if (oBlend.value && oAREF.value<16)	bForward	= TRUE;
	if (oStrictSorting.value)			bForward	= TRUE;

	if (bForward)			{
		// forward rendering
		LPCSTR	vsname,psname;
		switch(C.iElement) 
		{
		case 0: 	//
		case 1: 	//
			vsname = psname =	"model_def_lq"; 
			C.r_Pass(vsname, psname, true, true, false, true, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, true, oAREF.value);
			//C.r_Sampler			("s_base",	C.L_textures[0]);
			C.r_dx10Texture		("s_base",	C.L_textures[0]);
			C.r_dx10Sampler		("smp_base");
			C.r_End				();
			break;
		default:
			break;
		}
	} else {
		BOOL	bAref		= oBlend.value;
		// deferred rendering
		// codepath is the same, only the shaders differ

		bool bUseATOC = (bAref && (RImplementation.o.dx10_msaa_alphatest==CRender::MSAA_ATEST_DX10_0_ATOC));

		C.TessMethod = oTessellation.IDselected;
		switch(C.iElement) 
		{
		case SE_R2_NORMAL_HQ: 			// deffer
			if (bUseATOC)
			{
				uber_deffer		(C,true,"model","base_atoc",bAref,0,true);
				C.r_Stencil(TRUE, D3D11_COMPARISON_ALWAYS, 0xff, 0x7f, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_KEEP);
				C.r_StencilRef	(0x01);
				C.r_ColorWriteEnable(false, false, false, false);
				//	Alpha to coverage.
				C.RS.SetRS	(XRDX10RS_ALPHATOCOVERAGE,	TRUE);
				C.r_End			();
			}

			if (C.HudElement)
			{
				uber_deffer(C, true, "model_hud", "base_hud", bAref, 0, true);
				C.r_dx10Texture("s_hud_rain", "shaders\\hud_rain");
			}
			else
				uber_deffer(C, true, "model", "base", bAref, 0, true);

			C.r_Stencil(TRUE, D3D11_COMPARISON_ALWAYS, 0xff, 0x7f, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_KEEP);
			C.r_StencilRef	(0x01);
			if (bUseATOC) C.RS.SetRS(D3DRS_ZFUNC, D3D11_COMPARISON_EQUAL);
			C.r_End			();
			break;
		case SE_R2_NORMAL_LQ: 			// deffer
			if (bUseATOC)
			{
				uber_deffer		(C,false,"model","base_atoc",bAref,0,true);
				C.r_Stencil(TRUE, D3D11_COMPARISON_ALWAYS, 0xff, 0x7f, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_KEEP);
				C.r_StencilRef	(0x01);
				C.r_ColorWriteEnable(false, false, false, false);
				//	Alpha to coverage.
				C.RS.SetRS	(XRDX10RS_ALPHATOCOVERAGE,	TRUE);
				C.r_End			();
			}

			uber_deffer		(C,false,	"model",	"base",bAref,0,true);
			C.r_Stencil(TRUE, D3D11_COMPARISON_ALWAYS, 0xff, 0x7f, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_REPLACE, D3D11_STENCIL_OP_KEEP);
			C.r_StencilRef	(0x01);
			if (bUseATOC) C.RS.SetRS(D3DRS_ZFUNC, D3D11_COMPARISON_EQUAL);
			C.r_End			();
			break;
		case SE_R2_SHADOW:				// smap
			if (bAref)
			{
				C.r_Pass("shadow_direct_model_aref", "shadow_direct_base_aref", false, true, true, false, D3D11_BLEND_ZERO, D3D11_BLEND_ONE, true, 220);
				C.r_dx10Texture		("s_base",C.L_textures[0]);
				C.r_dx10Sampler		("smp_base");
				C.r_dx10Sampler		("smp_linear");
				C.r_ColorWriteEnable(false, false, false, false);
				C.r_End			();
				break;
			} 
			else 
			{
				C.r_Pass("shadow_direct_model", "dumb", false, true, true, false);
				C.r_dx10Texture		("s_base",C.L_textures[0]);
				C.r_dx10Sampler		("smp_base");
				C.r_dx10Sampler		("smp_linear");
				C.r_ColorWriteEnable(false, false, false, false);
				C.r_End			();
				break;
			}
		}
	}
}
