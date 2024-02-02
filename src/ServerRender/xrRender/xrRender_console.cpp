#include	"stdafx.h"
#pragma		hdrstop

#include	"xrRender_console.h"
#include	"dxRenderDeviceRender.h"

#include "../../build_config_defines.h"


// SSR quality option
u32			dt_ssr_samp = 5;
xr_token							ssr_samp_token[] = {
	{ "ssr_lowesr",					0												},
	{ "ssr_low",				1													},
	{ "ssr_medium",					2												},
	{ "ssr_high",				3													},
	{ "ssr_veryhigh",					4											},
	{ "ssr_ultra",					5												},
	{ 0,							0												}
};

// SMAP Control

u32 ps_r2_smapsize = 2048;
xr_token qsmapsize_token[] =
{
	{ "512",						512											},
	{ "1024",						1024										},
	{ "1536",						1536										},
	{ "2048",						2048										},
	{ "2560",						2560										},
	{ "3072",						3072										},
	{ "4096",						4096										},
	{ "8192",						8192										},
	{ 0,							0											}
};

u32			ps_Preset				=	2	;
xr_token							qpreset_token							[ ]={
	{ "Minimum",					0											},
	{ "Low",						1											},
	{ "Default",					2											},
	{ "High",						3											},
	{ "Extreme",					4											},
	{"Ultra_SSR",					5											},
	{ 0,							0											}
};

u32			ps_r_ssao_mode			=	2;
xr_token							qssao_mode_token						[ ]={
	{ "disabled",					0											},
	{ "default",					1											},
	{ "hdao",						2											},
	{ "hbao",						3											},
	{ "hbao_plus",						4											},
	{ 0,							0											}
};

u32			ps_r_sun_shafts				=	2;
xr_token							qsun_shafts_token							[ ]={
	{ "st_opt_off",					0												},
	{ "st_opt_low",					1												},
	{ "st_opt_medium",				2												},
	{ "st_opt_high",				3												},
	{ 0,							0												}
};

//ogse sunshafts
u32 ps_sunshafts_mode = 0;
xr_token sunshafts_mode_token[] = {
	{ "volumetric", 0 },
	{ "screen_space", 1 },
	{ "combine_sunshafts", 2 },
	{ NULL, 0 }
};


u32			ps_r_ssao				=	3;
xr_token							qssao_token									[ ]={
	{ "st_opt_off",					0												},
	{ "st_opt_low",					1												},
	{ "st_opt_medium",				2												},
	{ "st_opt_high",				3												},
#if defined(USE_DX10) || defined(USE_DX11)
	{ "st_opt_ultra",				4												},
#endif
	{ 0,							0												}
};

u32			ps_r_sun_quality		=	1;			//	=	0;
xr_token							qsun_quality_token							[ ]={
	{ "st_opt_low",					0												},
	{ "st_opt_medium",				1												},
	{ "st_opt_high",				2												},
#if defined(USE_DX10) || defined(USE_DX11)
	{ "st_opt_ultra",				3												},
	{ "st_opt_extreme",				4												},
#endif	//	USE_DX10
	{ 0,							0												}
};

u32			ps_r3_msaa				=	0;			//	=	0;
xr_token							qmsaa_token							[ ]={
	{ "st_opt_off",					0												},
	{ "2x",							1												},
	{ "4x",							2												},
//	{ "8x",							3												},
	{ 0,							0												}
};

u32			ps_r3_msaa_atest		=	0;			//	=	0;
xr_token							qmsaa__atest_token					[ ]={
	{ "st_opt_off",					0												},
	{ "st_opt_atest_msaa_dx10_0",	1												},
	{ "st_opt_atest_msaa_dx10_1",	2												},
	{ 0,							0												}
};

u32			ps_r3_minmax_sm			=	3;			//	=	0;
xr_token							qminmax_sm_token					[ ]={
	{ "off",						0												},
	{ "on",							1												},
	{ "auto",						2												},
	{ "autodetect",					3												},
	{ 0,							0												}
};

//	УOffФ
//	УDX10.0 style [Standard]Ф
//	УDX10.1 style [Higher quality]Ф

// Common
extern int			psSkeletonUpdate;
extern float		r__dtex_range;

Flags32 ps_r__common_flags = {/*RFLAG_NO_RAM_TEXTURES*/ }; // All renders

//int		ps_r__Supersample			= 1		;
int			ps_r__LightSleepFrames		= 10	;

float		ps_r__Detail_l_ambient		= 0.9f	;
float		ps_r__Detail_l_aniso		= 0.25f	;
float		ps_r__Detail_density		= 0.3f	;
float		ps_r__Detail_height = 1.0f;
float		ps_r__Detail_rainbow_hemi	= 0.75f	;

float		ps_r__Tree_w_rot			= 10.0f	;
float		ps_r__Tree_w_speed			= 1.00f	;
float		ps_r__Tree_w_amp			= 0.005f;
Fvector		ps_r__Tree_Wave				= {.1f, .01f, .11f};
float		ps_r__Tree_SBC				= 1.5f	;	// scale bias correct

float		ps_r__WallmarkTTL			= 50.f	;
float		ps_r__WallmarkSHIFT			= 0.0001f;
float		ps_r__WallmarkSHIFT_V		= 0.0001f;

float		ps_r__GLOD_ssa_start		= 256.f	;
float		ps_r__GLOD_ssa_end			=  64.f	;
float		ps_r__LOD					=  0.75f	;
//. float		ps_r__LOD_Power				=  1.5f	;
float		ps_r__ssaDISCARD			=  3.5f	;					//RO
float		ps_r__ssaDONTSORT			=  32.f	;					//RO
float		ps_r__ssaHZBvsTEX			=  96.f	;					//RO

int			ps_r__tf_Anisotropic		= 8		;

// R1
float		ps_r1_ssaLOD_A				= 64.f	;
float		ps_r1_ssaLOD_B				= 48.f	;
float		ps_r1_tf_Mipbias			= 0.0f	;
Flags32		ps_r1_flags					= { R1FLAG_DLIGHTS };		// r1-only
float		ps_r1_lmodel_lerp			= 0.1f	;
float		ps_r1_dlights_clip			= 40.f	;
float		ps_r1_pps_u					= 0.f	;
float		ps_r1_pps_v					= 0.f	;
extern float			hbao_plus_radius = 1.5;
extern float			hbao_plus_bias = 0.15;
extern float hbao_plus_power_exponent = 1.5;
extern float hbao_plus_blur_sharp = 32;

// R1-specific
int			ps_r1_GlowsPerFrame			= 16	;					// r1-only
float		ps_r1_fog_luminance			= 1.1f	;					// r1-only
int			ps_r1_SoftwareSkinning		= 0		;					// r1-only

// R2
float		ps_r2_ssaLOD_A				= 64.f	;
float		ps_r2_ssaLOD_B				= 48.f	;
float		ps_r2_tf_Mipbias			= 0.0f	;

// R2-specific
Flags32		ps_r2_ls_flags				= { R2FLAG_SUN 
	//| R2FLAG_SUN_IGNORE_PORTALS
	| R2FLAG_EXP_DONT_TEST_UNSHADOWED 
	| R2FLAG_USE_NVSTENCIL | R2FLAG_EXP_SPLIT_SCENE 
	| R3FLAG_DYN_WET_SURF
	| R3FLAG_VOLUMETRIC_SMOKE
	//| R3FLAG_MSAA 
	//| R3FLAG_MSAA_OPT
	| R3FLAG_GBUFFER_OPT
	|R2FLAG_DETAIL_BUMP
	|R2FLAG_DOF
	|R2FLAG_SOFT_PARTICLES
	|R2FLAG_SOFT_WATER
	|R2FLAG_STEEP_PARALLAX
	|R2FLAG_SUN_FOCUS
	|R2FLAG_SUN_TSM
	|R2FLAG_TONEMAP
	//|R2FLAG_VOLUMETRIC_LIGHTS
	};	// r2-only

Flags32		ps_r2_ls_flags_ext			= {
		/*R2FLAGEXT_SSAO_OPT_DATA |*/ R2FLAGEXT_SSAO_HALF_DATA
		|R2FLAGEXT_ENABLE_TESSELLATION
	};

int			ps_no_scale_on_fade = 0;
float		ps_r2_df_parallax_h			= 0.02f;
float		ps_r2_df_parallax_range		= 75.f;
float		ps_r2_tonemap_middlegray = 1.3f;
float		ps_r2_tonemap_adaptation = 10.f;
float		ps_r2_tonemap_low_lum = 0.5f;
float		ps_r2_tonemap_amount = 1.0f;
float		ps_r2_ls_bloom_kernel_g		= 3.f;				// r2-only
float		ps_r2_ls_bloom_kernel_b = 1.0f;
float		ps_r2_ls_bloom_speed = 50.f;
float		ps_r2_ls_bloom_kernel_scale = .9f; // gauss
float		ps_r2_ls_dsm_kernel			= .7f;				// r2-only
float		ps_r2_ls_psm_kernel			= .7f;				// r2-only
float		ps_r2_ls_ssm_kernel			= .7f;				// r2-only
float		ps_r2_ls_bloom_threshold = .03f;
Fvector		ps_r2_aa_barier				= { .8f, .1f, 0};	// r2-only
Fvector		ps_r2_aa_weight				= { .25f,.25f,0};	// r2-only
float		ps_r2_aa_kernel				= .5f;				// r2-only
float		ps_r2_mblur					= .0f;				// .5f
int			ps_r2_GI_depth				= 1;				// 1..5
int			ps_r2_GI_photons			= 16;				// 8..64
float		ps_r2_GI_clip				= EPS_L;			// EPS
float		ps_r2_GI_refl				= .9f;				// .9f
float		ps_r2_ls_depth_scale		= 1.00001f;			// 1.00001f
float		ps_r2_ls_depth_bias			= -0.001f;			// -0.0001f
float		ps_r2_ls_squality			= 1.0f;				// 1.00f
float		ps_r2_sun_tsm_projection	= 0.3f;			// 0.18f
float		ps_r2_sun_tsm_bias			= -0.01f;			// 
float		ps_r2_sun_near				= 20.f;				// 12.0f

extern float OLES_SUN_LIMIT_27_01_07;	//	actually sun_far

float		ps_r2_sun_near_border		= 0.75f;			// 1.0f
float		ps_r2_sun_depth_far_scale	= 1.00000f;			// 1.00001f
float		ps_r2_sun_depth_far_bias	= -0.00002f;			// -0.0000f
float		ps_r2_sun_depth_near_scale	= 1.0000f;			// 1.00001f
float		ps_r2_sun_depth_near_bias	= 0.00001f;		// -0.00005f
float		ps_r2_sun_lumscale			= 1.0f;				// 1.0f
float		ps_r2_sun_lumscale_hemi		= 1.0f;				// 1.0f
float		ps_r2_sun_lumscale_amb		= 1.0f;
float		ps_r2_gmaterial				= 2.2f;				// 
float		ps_r2_zfill					= 0.25f;				// .1f

float		ps_r2_dhemi_sky_scale		= 0.08f;				// 1.5f
float		ps_r2_dhemi_light_scale     = 0.2f	;
float		ps_r2_dhemi_light_flow      = 0.1f	;
int			ps_r2_dhemi_count			= 5;				// 5
int			ps_r2_wait_sleep			= 0;

float		ps_r2_lt_smooth				= 1.f;				// 1.f
float		ps_r2_slight_fade			= 0.5f;				// 1.f

// Screen Space Shaders Stuff
// Anomaly
extern ENGINE_API float ps_r2_img_exposure;		// r2-only
extern ENGINE_API float ps_r2_img_gamma;		// r2-only
extern ENGINE_API float ps_r2_img_saturation;	// r2-only
extern ENGINE_API Fvector ps_r2_img_cg;			// r2-only

// Ascii1457's Screen Space Shaders
extern ENGINE_API Fvector3 ps_ssfx_shadow_cascades;
extern ENGINE_API Fvector4 ps_ssfx_grass_shadows;
extern ENGINE_API Fvector4 ps_ssfx_grass_interactive;
extern ENGINE_API Fvector4 ps_ssfx_int_grass_params_1;
extern ENGINE_API Fvector4 ps_ssfx_int_grass_params_2;
extern ENGINE_API Fvector4 ps_ssfx_hud_drops_1;
extern ENGINE_API Fvector4 ps_ssfx_hud_drops_2;
extern ENGINE_API Fvector4 ps_ssfx_blood_decals;
extern ENGINE_API Fvector4 ps_ssfx_rain_1;
extern ENGINE_API Fvector4 ps_ssfx_rain_2;
extern ENGINE_API Fvector4 ps_ssfx_rain_3;

extern ENGINE_API Fvector4 ps_ssfx_wind_grass;
extern ENGINE_API Fvector4 ps_ssfx_wind_trees;


int ps_screen_space_shaders = 0;


Fvector3	ps_r2_dof					= Fvector3().set(-1.25f, 1.4f, 10000.f);
float		ps_r2_dof_sky				= 30;				//	distance to sky
float		ps_r2_dof_kernel_size		= 5.0f;						//	7.0f

float		ps_r3_dyn_wet_surf_near		= 10.f;				// 10.0f
float		ps_r3_dyn_wet_surf_far		= 30.f;				// 30.0f
int			ps_r3_dyn_wet_surf_sm_res	= 256;				// 256

int			ps_r__detail_radius = 100;
#ifdef DETAIL_RADIUS // управление радиусом отрисовки травы
u32 dm_size = 24;
u32 dm_cache1_line = 12; //dm_size*2/dm_cache1_count
u32 dm_cache_line = 49; //dm_size+1+dm_size
u32 dm_cache_size = 2401; //dm_cache_line*dm_cache_line
float dm_fade = 47.5; //float(2*dm_size)-.5f;
u32 dm_current_size = 24;
u32 dm_current_cache1_line = 12; //dm_current_size*2/dm_cache1_count
u32 dm_current_cache_line = 49; //dm_current_size+1+dm_current_size
u32 dm_current_cache_size = 2401; //dm_current_cache_line*dm_current_cache_line
float dm_current_fade = 47.5; //float(2*dm_current_size)-.5f;
#endif
float ps_current_detail_density = 1.f;
float ps_current_detail_height = 1.2f;

//raindrops
float		droplets_power_debug = 0.f;
float		ps_r2_rain_drops_intensity = 0.00025f;
float		ps_r2_rain_drops_speed = 1.25f;

//ogse sunshafts
float		ps_r2_ss_sunshafts_length = 1.f;
float		ps_r2_ss_sunshafts_radius = 1.f;

//Pseudopbr
float ps_r3_pbr_intensity = 25.0f;
float ps_r3_pbr_roughness = 0.5f;
Flags32	ps_r3_pbr_flags = { R_FLAG_PSEUDOPBR };
//Geometry optimization from Anomaly
int opt_static = 0;
int opt_dynamic = 0;

//SFZ Lens Flares
int ps_r2_lfx = 0;

//Static on dx11
Flags32	ps_r2_static_flags = { R2FLAG_USE_BUMP
	| R2FLAG_STATIC_SUN
};

Flags32 psDeviceFlags2 = { 0 };


Flags32		ps_actor_shadow_flags = { 1 };


Flags32		ps_r2_rain_drops_flags = { R2FLAG_RAIN_DROPS
	| R2FLAG_RAIN_DROPS_CONTROL
};

Flags32		ps_r2_hud_mask_flags = { R_FLAG_HUD_MASK };

float ps_r2_gloss_factor = 10.0f;
float ps_r2_gloss_min = 0.0f;
#ifndef _EDITOR
#include	"../../xrEngine/xr_ioconsole.h"
#include	"../../xrEngine/xr_ioc_cmd.h"

#if defined(USE_DX10) || defined(USE_DX11)
#include "../xrRenderDX10/StateManager/dx10SamplerStateCache.h"
#endif	//	USE_DX10

class CCC_ssfx_cascades : public CCC_Vector3
	 {
public:
	void apply()
		 {
		#if defined(USE_DX10) || defined(USE_DX11)
			RImplementation.init_cascades();
		#endif
			 }
	
		CCC_ssfx_cascades(LPCSTR N, Fvector3* V, const Fvector3 _min, const Fvector3 _max) : CCC_Vector3(N, V, _min, _max)
		 {
		};
	
		virtual void Execute(LPCSTR args)
		 {
		CCC_Vector3::Execute(args);
		apply();
		}
	
		virtual void GetStatus(TStatus& S)
		 {
		CCC_Vector3::Status(S);
		apply();
		}
	 };

void		xrRender_initconsole	()
{
}

#endif
