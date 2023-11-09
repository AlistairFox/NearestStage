#include "stdafx.h"
#include "dxRenderDeviceRender.h"

#include "ResourceManager.h"

dxRenderDeviceRender::dxRenderDeviceRender()
	:	Resources(0)
{
	;
}

void dxRenderDeviceRender::Copy(IRenderDeviceRender &_in)
{
	*this = *(dxRenderDeviceRender*)&_in;
}

void dxRenderDeviceRender::setGamma(float fGamma)
{
	m_Gamma.Gamma(fGamma);
}

void dxRenderDeviceRender::setBrightness(float fGamma)
{
	m_Gamma.Brightness(fGamma);
}

void dxRenderDeviceRender::setContrast(float fGamma)
{
	m_Gamma.Contrast(fGamma);
}

void dxRenderDeviceRender::updateGamma()
{
	m_Gamma.Update();
}

void dxRenderDeviceRender::OnDeviceDestroy( BOOL bKeepTextures)
{
	m_WireShader.destroy();
	m_SelectionShader.destroy();

	Resources->OnDeviceDestroy( bKeepTextures);
	RCache.OnDeviceDestroy();
}

void dxRenderDeviceRender::ValidateHW()
{
	HW.Validate();
}

void dxRenderDeviceRender::DestroyHW()
{
	xr_delete					(Resources);
	HW.DestroyDevice			();
}

void dxRenderDeviceRender::SwitchViewPortRTZB(ViewPort vp)
{
	/*HW.SwitchVP(vp);
	if (RImplementation.Target)
	{
		// Below are just in case
		RImplementation.Target->u_setrt(Device.dwWidth, Device.dwHeight, HW.pBaseRT, NULL, NULL, HW.pBaseZB); // Set up HW base as RT and ZB
		RImplementation.rmNormal(); // Resize viewport as render target
		RImplementation.Target->SwitchViewPort(vp);
	}*/
}



void  dxRenderDeviceRender::Reset( HWND hWnd, u32 &dwWidth, u32 &dwHeight, float &fWidth_2, float &fHeight_2)
{


	Resources->reset_begin	();
	Memory.mem_compact		();




	HW.Reset				(hWnd);


	dwWidth					= HW.m_ChainDesc.BufferDesc.Width;
	dwHeight				= HW.m_ChainDesc.BufferDesc.Height;

	fWidth_2				= float(dwWidth/2);
	fHeight_2				= float(dwHeight/2);
	Resources->reset_end	();

}

void dxRenderDeviceRender::SetupStates()
{
	HW.Caps.Update			();

	SSManager.SetMaxAnisotropy(ps_r__tf_Anisotropic);
	SSManager.SetMipLODBias(ps_r__tf_Mipbias);


}

void dxRenderDeviceRender::OnDeviceCreate(LPCSTR shName)
{
	// Signal everyone - device created
	RCache.OnDeviceCreate		();
	m_Gamma.Update				();
	Resources->OnDeviceCreate	(shName);
	::Render->create			();
	Device.Statistic->OnDeviceCreate	();


		m_WireShader.create			("editor\\wire");
		m_SelectionShader.create	("editor\\selection");

		DUImpl.OnDeviceCreate			();

}

void dxRenderDeviceRender::Create( HWND hWnd, u32 &dwWidth, u32 &dwHeight, float &fWidth_2, float &fHeight_2, bool move_window)
{
	HW.CreateDevice		(hWnd, move_window);

	dwWidth					= HW.m_ChainDesc.BufferDesc.Width;
	dwHeight				= HW.m_ChainDesc.BufferDesc.Height;

	fWidth_2			= float(dwWidth/2)			;
	fHeight_2			= float(dwHeight/2)			;
	Resources			= xr_new<CResourceManager>		();
}

void dxRenderDeviceRender::SetupGPU( BOOL bForceGPU_SW, BOOL bForceGPU_NonPure, BOOL bForceGPU_REF)
{
	HW.Caps.bForceGPU_SW		= bForceGPU_SW;
	HW.Caps.bForceGPU_NonPure	= bForceGPU_NonPure;
	HW.Caps.bForceGPU_REF		= bForceGPU_REF;
}

void dxRenderDeviceRender::overdrawBegin()
{
	//	TODO: DX10: Implement overdrawBegin
	VERIFY(!"dxRenderDeviceRender::overdrawBegin not implemented.");
}

void dxRenderDeviceRender::overdrawEnd()
{

	//	TODO: DX10: Implement overdrawEnd
	VERIFY(!"dxRenderDeviceRender::overdrawBegin not implemented.");

}

void dxRenderDeviceRender::DeferredLoad(BOOL E)
{
	Resources->DeferredLoad(E);
}

void dxRenderDeviceRender::ResourcesDeferredUpload()
{
	Resources->DeferredUpload();
}

void dxRenderDeviceRender::ResourcesDeferredUnload()
{
	Resources->DeferredUnload();
}

void dxRenderDeviceRender::ResourcesStoreNecessaryTextures()
{
	dxRenderDeviceRender::Instance().Resources->StoreNecessaryTextures();
}

dxRenderDeviceRender::DeviceState dxRenderDeviceRender::GetDeviceState()
{
	HW.Validate		();
	//	TODO: DX10: Implement GetDeviceState
	//	TODO: DX10: Implement DXGI_PRESENT_TEST testing
	//VERIFY(!"dxRenderDeviceRender::overdrawBegin not implemented.");


	return dsOK;
}

BOOL dxRenderDeviceRender::GetForceGPU_REF()
{
	return HW.Caps.bForceGPU_REF;
}

u32 dxRenderDeviceRender::GetCacheStatPolys()
{
	return RCache.stat.polys;
}

void dxRenderDeviceRender::Begin()
{

	RCache.OnFrameBegin		();
	RCache.set_CullMode		(CULL_CW);
	RCache.set_CullMode		(CULL_CCW);
	if (HW.Caps.SceneMode)	overdrawBegin	();
}

void dxRenderDeviceRender::Clear()
{
	HW.pContext->ClearDepthStencilView(RCache.get_ZB(), 
		D3D_CLEAR_DEPTH|D3D_CLEAR_STENCIL, 1.0f, 0);

	if (psDeviceFlags.test(rsClearBB))
	{
		FLOAT ColorRGBA[4] = {0.0f,0.0f,0.0f,0.0f};
		HW.pContext->ClearRenderTargetView(RCache.get_RT(), ColorRGBA);
	}
}

void DoAsyncScreenshot();

void dxRenderDeviceRender::End()
{
	VERIFY	(HW.pDevice);

	if (HW.Caps.SceneMode)	overdrawEnd();

	RCache.OnFrameEnd	();


	if (RImplementation.currentViewPort == MAIN_VIEWPORT)
		DoAsyncScreenshot();


	if (RImplementation.needPresenting) //--#SM+#-- +SecondVP+        
	{
		bool bUseVSync = psDeviceFlags.is(rsFullscreen) && psDeviceFlags.test(rsVSync); // xxx: weird tearing glitches when VSync turned on for windowed mode in DX10\11
		HW.m_pSwapChain->Present(bUseVSync ? 1 : 0, 0);
	}

}

void dxRenderDeviceRender::ResourcesDestroyNecessaryTextures()
{
	Resources->DestroyNecessaryTextures();
}

void dxRenderDeviceRender::ClearTarget()
{

	FLOAT ColorRGBA[4] = {0.0f,0.0f,0.0f,0.0f};
	HW.pContext->ClearRenderTargetView(RCache.get_RT(), ColorRGBA);

}

void dxRenderDeviceRender::SetCacheXform(Fmatrix &mView, Fmatrix &mProject)
{
	RCache.set_xform_view(mView);
	RCache.set_xform_project(mProject);
}

bool dxRenderDeviceRender::HWSupportsShaderYUV2RGB()
{
	u32		v_dev	= CAP_VERSION(HW.Caps.raster_major, HW.Caps.raster_minor);
	u32		v_need	= CAP_VERSION(2,0);
	return (v_dev>=v_need);
}

void  dxRenderDeviceRender::OnAssetsChanged()
{
	Resources->m_textures_description.UnLoad();
	Resources->m_textures_description.Load();
}
