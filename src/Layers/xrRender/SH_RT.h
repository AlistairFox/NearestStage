#ifndef SH_RT_H
#define SH_RT_H
#pragma once

#ifdef USE_DX11
enum VIEW_TYPE
{
	//SRV = 1 << 1, // ShaderResource
	//RTV = 1 << 2, // RenderTarget
	//DSV = 1 << 3, // DepthStencil
	//UAV = 1 << 4, // UnorderedAcces
	SRV_RTV,		// ShaderResource & RenderTarget
	SRV_RTV_UAV,	// ShaderResource & RenderTarget & UnorderedAcces
	SRV_RTV_DSV,	// ShaderResource & RenderTarget & DepthStencil
	SRV_DSV,		// ShaderResource & DepthStencil
};
#endif


#if defined (USE_DX10) || defined (USE_DX11)

enum ViewPort;

struct RtCreationParams
{
	u32 w;
	u32 h;
	ViewPort viewport;

	RtCreationParams(u32 W, u32 H, ViewPort vp)
	{
		w = W;
		h = H;
		viewport = vp;
	};
};

struct ViewPortRT
{
	ViewPortRT()
	{
		rtWidth = 0;
		rtHeight = 0;

		textureSurface = nullptr;
		zBufferInstance = nullptr;
		renderTargetInstance = nullptr;
#ifdef USE_DX11
		unorderedAccessViewInstance = nullptr;
#endif
		shaderResView = nullptr;
	}

	ID3DTexture2D* textureSurface;

	u32	rtWidth;
	u32 rtHeight;

	ID3DDepthStencilView* zBufferInstance;
	ID3DRenderTargetView* renderTargetInstance;
#ifdef USE_DX11
	ID3D11UnorderedAccessView* unorderedAccessViewInstance;
#endif
	ID3DShaderResourceView* shaderResView;
};

#endif

//////////////////////////////////////////////////////////////////////////
class		CRT		:	public xr_resource_named	{
public:
	CRT();
	~CRT();
#if defined (USE_DX11)
private:
	u32						rtWidth;
	u32						rtHeight;
	shared_str				rtName;
public:
	ViewPort					vpStored;
	bool						isTwoViewPorts;


	ID3DTexture2D* temp;
	ID3DDepthStencilView* pZRT;
	xr_map<u32, ViewPortRT> viewPortStuff;
	xr_vector<RtCreationParams>	creationParams;
#endif
#ifdef USE_DX11
	//void	create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1, bool useUAV = false );
	void	create(LPCSTR name, xr_vector<RtCreationParams>& vp_params, DXGI_FORMAT f, VIEW_TYPE view, u32 samples = 1);
	void	SwitchViewPortResources(ViewPort vp);
	u32		RTWidth() { return rtWidth; };
	u32		RTHeight() { return rtHeight; };
	ID3D11UnorderedAccessView* pUAView;
#else
	u32						dwWidth;
	u32						dwHeight;
	void	create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1);
#endif
	void	destroy();
	void	reset_begin();
	void	reset_end();
	IC BOOL	valid()	{ return !!pTexture; }

public:
	ID3DTexture2D*			pSurface;
	ID3DRenderTargetView*	pRT;

	ref_texture				pTexture;

#ifndef USE_DX11

	D3DFORMAT				fmt;
#else
	DXGI_FORMAT format;
	VIEW_TYPE					view;
	u32							samples;
#endif // !USE_DX11

	u64						_order;
};
struct 		resptrcode_crt	: public resptr_base<CRT>
{
#ifdef USE_DX11
	void create(LPCSTR name, xr_vector<RtCreationParams>& vp_params, DXGI_FORMAT f, VIEW_TYPE view, u32 samples = 1);

	void create(LPCSTR name, RtCreationParams creation_params, DXGI_FORMAT f, VIEW_TYPE view, u32 samples = 1)
	{
		xr_vector<RtCreationParams> params;
		params.push_back(creation_params);

		create(name, params, f, view, samples);
	};

	void create(LPCSTR Name, RtCreationParams creation_params_1, RtCreationParams creation_params_2, DXGI_FORMAT f, VIEW_TYPE view, u32 samples = 1)
	{
		xr_vector<RtCreationParams> params;
		params.push_back(creation_params_1);
		params.push_back(creation_params_2);

		create(Name, params, f, view, samples);
	};
#else
	void				create(LPCSTR Name, u32 w, u32 h, D3DFORMAT f, u32 SampleCount = 1);
#endif
	void				destroy			()	{ _set(NULL);		}
};
typedef	resptr_core<CRT, resptrcode_crt>		ref_rt;

#endif // SH_RT_H
