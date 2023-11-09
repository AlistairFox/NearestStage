#include "StdAfx.h"
#include "../../xrEngine/_d3d_extensions.h"
#include "../../xrEngine/xrLevel.h"
#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/environment.h"
#include "../../utils/xrLC_Light/R_light.h"
#include "light_db.h"

CLight_DB::CLight_DB()
{
}

CLight_DB::~CLight_DB()
{
}

void CLight_DB::Load			(IReader *fs) 
{
	IReader* F = fs->open_chunk(fsL_LIGHT_DYNAMIC);
	{
		// Light itself
		sun = NULL;

		u32 size		= F->length();
		u32 element		= sizeof(Flight)+4;
		u32 count = (size / element);
		VERIFY((count * element) == size);
		v_static.reserve(count);
		for (u32 i = 0; i < count; ++i)
		{
			Flight		Ldata;
			u32 controller				= 0;
			F->r						(&controller,4);
			F->r						(&Ldata,sizeof(Flight));

			light* L = Create();
			L->flags.bStatic = true;

			if (Ldata.type==D3DLIGHT_DIRECTIONAL)	{
				Fvector tmp_R;		tmp_R.set(1,0,0);

				L->set_type			(IRender_Light::DIRECT);
				L->set_shadow		(true);
				L->set_rotation		(Ldata.direction,tmp_R);
				
				sun = L;
			}
			else
			{
				Fvector tmp_D,tmp_R;
				tmp_D.set			(0,0,-1);	// forward
				tmp_R.set			(1,0,0);	// right

				// point
				L->set_type(IRender_Light::POINT);
				L->set_position		(Ldata.position		);
				L->set_rotation		(tmp_D, tmp_R		);
				L->set_range		(Ldata.range		);
				L->set_color		(Ldata.diffuse		);
				L->set_shadow(true);
				L->set_active		(true				);

				v_static.push_back(L);
			}
		}

	}
	F->close();

	R_ASSERT2(sun, "Where is sun?");
}

void	CLight_DB::LoadHemi	()
{
	string_path fn_game;
	if ( FS.exist( fn_game, "$level$", "build.lights" ) )
	{
		IReader *F	= FS.r_open( fn_game );
		{
			// Hemispheric light chunk
			IReader* chunk = F->open_chunk(fsL_HEADER);
			
			if (chunk)
			{
				u32 size		= chunk->length();
				u32 element		= sizeof(R_Light);
				u32 count		= size/element;
				VERIFY			(count*element == size);
				v_hemi.reserve(count);
				for (u32 i=0; i<count; i++) 
				{
					R_Light		Ldata;
					chunk->r(&Ldata, sizeof(R_Light));

					if (Ldata.type == D3DLIGHT_POINT)
					{
						Fvector tmp_D,tmp_R;
						tmp_D.set			(0,0,-1);	// forward
						tmp_R.set			(1,0,0);	// right

						light* L = Create();
						L->flags.bStatic = true;
						L->set_type(IRender_Light::POINT);
						L->set_position		(Ldata.position		);
						L->set_rotation		(tmp_D, tmp_R		);
						L->set_range		(Ldata.range		);
						L->set_color		(Ldata.diffuse.x, Ldata.diffuse.y, Ldata.diffuse.z);
						L->set_active		(true				);
						L->spatial.type = STYPE_LIGHTSOURCEHEMI;
						L->set_attenuation_params(Ldata.attenuation0, Ldata.attenuation1, Ldata.attenuation2, Ldata.falloff);
						v_hemi.push_back(L);
					}
				}
				chunk->close			();
			}
		}
		FS.r_close(F);
	}
}

void			CLight_DB::Unload	()
{
	v_static.clear			();
	v_hemi.clear			();
	sun.destroy();
}

light*			CLight_DB::Create	()
{
	light*	L			= xr_new<light>	();
	L->flags.bStatic	= false;
	L->flags.bActive	= false;
	L->flags.bShadow	= true;
	return				L;
}


void CLight_DB::add_light(light* L)
{
	if (Device.dwFrame == L->frame_render)
		return;
	L->frame_render = Device.dwFrame;
	if (RImplementation.o.noshadows)
		L->flags.bShadow = FALSE;
	if (L->flags.bStatic && !ps_r2_ls_flags.test(R2FLAG_R1LIGHTS))
		return;
	L->export_to(package);
}

void			CLight_DB::Update			()
{
	// set sun params
	if (sun)
	{
		light* _sun = (light*)sun._get();
		CEnvDescriptor&	E			= *g_pGamePersistent->Environment().CurrentEnv;
		VERIFY						(_valid(E.sun_dir));


		VERIFY2						(E.sun_dir.y<0,"Invalid sun direction settings in evironment-config");
		Fvector dir, pos;
		dir.set(E.sun_dir).normalize();
		pos.mad(Device.vCameraPosition, dir, -500.f);

		sun->set_rotation(dir, _sun->right);
		sun->set_position(pos);
		sun->set_color(E.sun_color.x*ps_r2_sun_lumscale,E.sun_color.y*ps_r2_sun_lumscale,E.sun_color.z*ps_r2_sun_lumscale);
		sun->set_range(600.f);
	}
	// Clear selection
	package.clear	();
}
