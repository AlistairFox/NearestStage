#include "stdafx.h"
#pragma hdrstop

#include "../xrRender/ResourceManager.h"

#include "../xrRender/dxRenderDeviceRender.h"

CRT::CRT			()
{
	pSurface		= NULL;
	pRT				= NULL;
	pZRT			= NULL;
	pUAView			= NULL;

	rtWidth = 0;
	rtHeight = 0;
	format				= DXGI_FORMAT_UNKNOWN;

	vpStored = (ViewPort)0;
}
CRT::~CRT			()
{
	destroy			();

	// release external reference
	DEV->_DeleteRT	(this);
}

void CRT::create(LPCSTR Name, xr_vector<RtCreationParams>& vp_params, DXGI_FORMAT f, VIEW_TYPE view, u32 s)
{
	if (valid()) return;

	rtName = Name;

	R_ASSERT(HW.pDevice && Name && Name[0]);
	_order = CPU::GetCLK();	//Device.GetTimerGlobal()->GetElapsed_clk();


	creationParams = vp_params; // for device reset

	format = f;
	samples = s;

	DXGI_FORMAT dx10FMT = f;
	switch (dx10FMT)
	{
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:	dx10FMT = DXGI_FORMAT_R32G32_TYPELESS;		break;
	case DXGI_FORMAT_D32_FLOAT:				dx10FMT = DXGI_FORMAT_R32_TYPELESS;		break;
	case DXGI_FORMAT_D24_UNORM_S8_UINT:		dx10FMT = DXGI_FORMAT_R24G8_TYPELESS;		break;
	case DXGI_FORMAT_D16_UNORM:				dx10FMT = DXGI_FORMAT_R16_TYPELESS;		break;
	}

	D3D_TEXTURE2D_DESC desc = {};
	desc.Width = 0;//dwWidth;
	desc.Height = 0;//dwHeight;
		desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = dx10FMT;
	desc.SampleDesc.Count = samples;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D_USAGE_DEFAULT;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;



	bool use_uav = HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_0 && view == SRV_RTV_UAV && s <= 1;
	bool use_dsv = view == SRV_DSV || view == SRV_RTV_DSV;
	bool use_rtv = view == SRV_RTV || view == SRV_RTV_UAV || view == SRV_RTV_DSV;
	bool use_srv = HW.FeatureLevel >= D3D_FEATURE_LEVEL_10_1 || !use_dsv || s <= 1;

	if (use_srv) desc.BindFlags |= D3D_BIND_SHADER_RESOURCE;
	if (use_rtv) desc.BindFlags |= D3D_BIND_RENDER_TARGET;
	if (use_uav) desc.BindFlags |= D3D_BIND_UNORDERED_ACCESS;
	if (use_dsv) desc.BindFlags |= D3D_BIND_DEPTH_STENCIL;

	// DX10.1 and later hardware with MSAA
	if (s > 1 && HW.Caps.raster_major >= 4 && HW.Caps.raster_minor >= 1)
		desc.SampleDesc.Quality = UINT(D3D_STANDARD_MULTISAMPLE_PATTERN);


	pTexture = DEV->_CreateTexture(Name);


	for (size_t i = 0; i < vp_params.size(); ++i)
	{
		R_ASSERT(vp_params[i].w && vp_params[i].h);

		// Check width-and-height of render target surface
		if (vp_params[i].w > D3D_REQ_TEXTURE2D_U_OR_V_DIMENSION)
			return;
		if (vp_params[i].h > D3D_REQ_TEXTURE2D_U_OR_V_DIMENSION)
			return;

		desc.Width = vp_params[i].w;
		desc.Height = vp_params[i].h;

		auto it = viewPortStuff.insert(mk_pair(vp_params[i].viewport, ViewPortRT()));

		it.first->second.rtWidth = vp_params[i].w;
		it.first->second.rtHeight = vp_params[i].h;

		// Try to create texture/surface
		DEV->Evict();

		R_CHK(HW.pDevice->CreateTexture2D(&desc, NULL, &it.first->second.textureSurface));

		HW.stats_manager.increment_stats_rtarget(it.first->second.textureSurface);

		//if (bUseAsDepth)
		//	R_CHK(HW.pDevice->CreateDepthStencilView(it.first->second.textureSurface, &ViewDesc, &it.first->second.zBufferInstance));
		//else
		//	R_CHK(HW.pDevice->CreateRenderTargetView(it.first->second.textureSurface, 0, &it.first->second.renderTargetInstance));
		//if (bUseAsDepth)
		//    R_CHK(HW.pDevice->CreateDepthStencilView(it.first->second.textureSurface, &ViewDesc, &it.first->second.zBufferInstance));
		//else
		//    R_CHK(HW.pDevice->CreateRenderTargetView(it.first->second.textureSurface, 0, &it.first->second.renderTargetInstance));
	// Create rendertarget view
		if (use_rtv)
		{
			CHK_DX(HW.pDevice->CreateRenderTargetView(it.first->second.textureSurface, NULL, &it.first->second.renderTargetInstance));
		}

		// Create depth stencil view
		if (use_dsv)
		{
			D3D_DEPTH_STENCIL_VIEW_DESC    depthstencil = {};
			depthstencil.Format = format;
			depthstencil.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2D;
			depthstencil.Texture2D.MipSlice = 0;
			depthstencil.Flags = 0;
			if (samples > 1)
			{
				depthstencil.ViewDimension = D3D_DSV_DIMENSION_TEXTURE2DMS;
				depthstencil.Texture2DMS.UnusedField_NothingToDefine = 0;
			}

			CHK_DX(HW.pDevice->CreateDepthStencilView(it.first->second.textureSurface, &depthstencil, &it.first->second.zBufferInstance));
		}
	
		// Create unordered acces view

		if (use_uav && HW.Caps.raster_major >= 5)
		{
			D3D_UNORDERED_ACCESS_VIEW_DESC unorderedacces = {};
			unorderedacces.Format = format;
			unorderedacces.ViewDimension = D3D_UAV_DIMENSION_TEXTURE2D;
			unorderedacces.Buffer.FirstElement = 0;
			unorderedacces.Buffer.NumElements = vp_params[i].w * vp_params[i].h;

			CHK_DX(HW.pDevice->CreateUnorderedAccessView(it.first->second.textureSurface, &unorderedacces, &it.first->second.unorderedAccessViewInstance));
		}
		it.first->second.shaderResView = pTexture->CreateShaderRes(it.first->second.textureSurface);
		//pTexture

//R_ASSERT2(it.first->second.shaderResView, make_string("%s", rtName.c_str()));

		it.first->second.textureSurface->AddRef();

		if (pTexture->Description().SampleDesc.Count <= 1 || pTexture->Description().Format != DXGI_FORMAT_R24G8_TYPELESS)
			R_ASSERT2(it.first->second.shaderResView, make_string("%s", rtName.c_str()));

	}
	auto it = viewPortStuff.begin();

	pRT = it->second.renderTargetInstance;
	pZRT = it->second.zBufferInstance;
	pUAView = it->second.unorderedAccessViewInstance;
	pSurface = it->second.textureSurface;
	rtWidth = it->second.rtWidth;
	rtHeight = it->second.rtHeight;
	pTexture->SurfaceSetRT(it->second.textureSurface, it->second.shaderResView);



	//pTexture	= DEV->_CreateTexture	(Name);
	//pTexture->surface_set(pSurface);
}

void CRT::destroy		()
{
	if (pTexture._get())	{
		//pTexture->surface_set	(0);
		pTexture->surface_null();
		pTexture				= NULL;
	}
	for (auto it = viewPortStuff.begin(); it != viewPortStuff.end(); it++)
	{
		_RELEASE(it->second.renderTargetInstance);
		_RELEASE(it->second.zBufferInstance);

		HW.stats_manager.decrement_stats_rtarget(it->second.textureSurface);

		_RELEASE(it->second.textureSurface);
		_RELEASE(it->second.unorderedAccessViewInstance);
		_RELEASE(it->second.shaderResView);
	}
}
void CRT::reset_begin	()
{
	destroy		();
}
void CRT::reset_end		()
{
	create(*cName, creationParams, format, view, samples);
}
void CRT::SwitchViewPortResources(ViewPort vp)
{
	if (vpStored == vp && pSurface)
		return;

	vpStored = vp;

	xr_map<u32, ViewPortRT>::iterator it = viewPortStuff.find(vp);

	if (it == viewPortStuff.end())
	{
		it = viewPortStuff.find(MAIN_VIEWPORT);
	}

	R_ASSERT(it != viewPortStuff.end());

	const ViewPortRT& value = it->second;

	pRT = value.renderTargetInstance;
	pZRT = value.zBufferInstance;
	pUAView = value.unorderedAccessViewInstance;
	pSurface = value.textureSurface;
	rtWidth = value.rtWidth;
	rtHeight = value.rtHeight;

	//Msg("SwitchViewPortResources %u %u", rtWidth, rtHeight);

	R_ASSERT2(pRT || pZRT, make_string("%s", rtName.c_str()));
	R_ASSERT2(pSurface, make_string("%s", rtName.c_str()));
	//R_ASSERT2(value.shaderResView, make_string("%s", rtName.c_str()));

	if (pTexture->Description().SampleDesc.Count <= 1 || pTexture->Description().Format != DXGI_FORMAT_R24G8_TYPELESS)
		R_ASSERT2(value.shaderResView, make_string("%s", rtName.c_str()));


	pTexture->SurfaceSetRT(value.textureSurface, value.shaderResView);
}

void resptrcode_crt::create(LPCSTR Name, xr_vector<RtCreationParams>& vp_params, DXGI_FORMAT f, VIEW_TYPE view, u32 samples)
{
	_set(DEV->_CreateRT(Name, vp_params, f, view, samples));
}
