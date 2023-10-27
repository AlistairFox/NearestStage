#include "stdafx.h"
//#include "../../xrEngine/xr_effgamma.h"
#include "xr_effgamma.h"
#include "dxRenderDeviceRender.h"
#include "../xrRender/tga.h"
#include "../../xrEngine/xrImage_Resampler.h"

#include "d3dx10tex.h"

#define	GAMESAVE_SIZE	128

IC u32 convert(float c) {
	u32 C = iFloor(c);
	if (C > 255) C = 255;
	return C;
}
IC void MouseRayFromPoint(Fvector& direction, int x, int y, Fmatrix& m_CamMat)
{
	int halfwidth = Device.dwWidth / 2;
	int halfheight = Device.dwHeight / 2;

	Ivector2 point2;
	point2.set(x - halfwidth, halfheight - y);

	float size_y = VIEWPORT_NEAR * tanf(deg2rad(60.f) * 0.5f);
	float size_x = size_y / (Device.fHeight_2 / Device.fWidth_2);

	float r_pt = float(point2.x) * size_x / (float)halfwidth;
	float u_pt = float(point2.y) * size_y / (float)halfheight;

	direction.mul(m_CamMat.k, VIEWPORT_NEAR);
	direction.mad(direction, m_CamMat.j, u_pt);
	direction.mad(direction, m_CamMat.i, r_pt);
	direction.normalize();
}

#define SM_FOR_SEND_WIDTH 640
#define SM_FOR_SEND_HEIGHT 480

void CRender::ScreenshotImpl(ScreenshotMode mode, LPCSTR name, CMemoryWriter* memory_writer)
{
	ID3DResource* pSrcTexture;
	//HW.pBaseRT->GetResource(&pSrcTexture);

	auto VP = HW.viewPortsRTZB.find(MAIN_VIEWPORT);
	VP->second.baseRT->GetResource(&pSrcTexture);

	VERIFY(pSrcTexture);

	// Save
	switch (mode)
	{
	case IRender_interface::SM_FOR_GAMESAVE:
	{
		ID3DTexture2D* pSrcSmallTexture;

		D3D_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = GAMESAVE_SIZE;
		desc.Height = GAMESAVE_SIZE;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_BC1_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D_USAGE_DEFAULT;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		CHK_DX(HW.pDevice->CreateTexture2D(&desc, NULL, &pSrcSmallTexture));

		//	D3DX10_TEXTURE_LOAD_INFO *pLoadInfo

		CHK_DX(D3DX11LoadTextureFromTexture(HW.pContext, pSrcTexture,
			NULL, pSrcSmallTexture));

		// save (logical & physical)
		ID3DBlob* saved = 0;

		HRESULT hr = D3DX11SaveTextureToMemory(HW.pContext, pSrcSmallTexture, D3DX11_IFF_DDS, &saved, 0);

		if (hr == D3D_OK)
		{
			IWriter* fs = FS.w_open(name);
			if (fs)
			{
				fs->w(saved->GetBufferPointer(), (u32)saved->GetBufferSize());
				FS.w_close(fs);
			}
		}
		_RELEASE(saved);

		// cleanup
		_RELEASE(pSrcSmallTexture);
	}
	break;
	case IRender_interface::SM_FOR_MPSENDING:
	{

		ID3DTexture2D* pSrcSmallTexture;

		D3D_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = SM_FOR_SEND_WIDTH;
		desc.Height = SM_FOR_SEND_HEIGHT;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_BC1_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D_USAGE_DEFAULT;
		desc.BindFlags = D3D_BIND_SHADER_RESOURCE;
		CHK_DX(HW.pDevice->CreateTexture2D(&desc, NULL, &pSrcSmallTexture));

		//	D3DX10_TEXTURE_LOAD_INFO *pLoadInfo

		CHK_DX(D3DX11LoadTextureFromTexture(HW.pContext, pSrcTexture,
			NULL, pSrcSmallTexture));

		// save (logical & physical)
		ID3DBlob* saved = 0;

		HRESULT hr = D3DX11SaveTextureToMemory(HW.pContext, pSrcSmallTexture, D3DX11_IFF_DDS, &saved, 0);

		if (hr == D3D_OK)
		{
			if (!memory_writer)
			{
				IWriter* fs = FS.w_open(name);
				if (fs)
				{
					fs->w(saved->GetBufferPointer(), (u32)saved->GetBufferSize());
					FS.w_close(fs);
				}
			}
			else
			{
				memory_writer->w(saved->GetBufferPointer(), (u32)saved->GetBufferSize());
			}
		}
		_RELEASE(saved);

		// cleanup
		_RELEASE(pSrcSmallTexture);

	}
	break;
	case IRender_interface::SM_NORMAL:
	{
		string64			t_stemp;
		string_path			buf;

		if (strstr(Core.Params, "-ss_jpg"))
		{
			xr_sprintf(buf, sizeof(buf), "ss_%s_%s_(%s).jpg", Core.UserName, timestamp(t_stemp), (g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu");
			ID3DBlob* saved = 0;

			CHK_DX(D3DX11SaveTextureToMemory(HW.pContext, pSrcTexture, D3DX11_IFF_JPG, &saved, 0));

			IWriter* fs = FS.w_open("$screenshots$", buf); R_ASSERT(fs);
			fs->w(saved->GetBufferPointer(), (u32)saved->GetBufferSize());
			FS.w_close(fs);
			_RELEASE(saved);
		}
		else if (strstr(Core.Params, "-ss_tga"))	 // HQ
		{
			xr_sprintf(buf, sizeof(buf), "ssq_%s_%s_(%s).tga", Core.UserName, timestamp(t_stemp), (g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu");
			ID3DBlob* saved = 0;

			CHK_DX(D3DX11SaveTextureToMemory(HW.pContext, pSrcTexture, D3DX11_IFF_BMP, &saved, 0));

			IWriter* fs = FS.w_open("$screenshots$", buf); R_ASSERT(fs);
			fs->w(saved->GetBufferPointer(), (u32)saved->GetBufferSize());
			FS.w_close(fs);
			_RELEASE(saved);
		}
		else // HQ PNG (Default)
		{
			sprintf_s(buf, sizeof(buf), "ssq_%s_%s_(%s).png", Core.UserName, timestamp(t_stemp), (g_pGameLevel) ? g_pGameLevel->name().c_str() : "mainmenu");
			ID3DBlob* saved = 0;
			//CHK_DX(D3DXSaveSurfaceToFileInMemory(&saved, D3DXIFF_PNG, pFB, 0, srcRect));

			CHK_DX(D3DX11SaveTextureToMemory(HW.pContext, pSrcTexture, D3DX11_IFF_PNG, &saved, 0));

			IWriter* fs = FS.w_open("$screenshots$", buf); R_ASSERT(fs);
			fs->w(saved->GetBufferPointer(), saved->GetBufferSize());
			FS.w_close(fs);
			_RELEASE(saved);
		}
	}
	break;
	case IRender_interface::SM_FOR_LEVELMAP:
	case IRender_interface::SM_FOR_CUBEMAP:
	{
		string_path buf;
		TGAdesc				p;
		VERIFY(name);
		strconcat(sizeof(buf), buf, name, ".tga");
		IWriter* fs = FS.w_open("$screenshots$", buf);
		R_ASSERT(fs);

		ID3DTexture2D* pTex = Target->t_ss_async;
		HW.pContext->CopyResource(pTex, pSrcTexture);

		D3D_MAPPED_TEXTURE2D MappedData;

		HW.pContext->Map(pTex, 0, D3D_MAP_READ, 0, &MappedData);

		// Swap r and b, but don't kill alpha
		{
			u32* pPixel = (u32*)MappedData.pData;
			u32* pEnd = pPixel + (Device.dwWidth * Device.dwHeight);

			for (; pPixel != pEnd; pPixel++)
			{
				u32 p = *pPixel;
				*pPixel = color_argb(color_get_A(p), color_get_B(p), color_get_G(p), color_get_R(p));
			}
		}
		// save
		u32* data = (u32*)xr_malloc(Device.dwHeight * Device.dwHeight * 4);
		imf_Process(data, Device.dwHeight, Device.dwHeight, (u32*)MappedData.pData, Device.dwWidth, Device.dwHeight, imf_lanczos3);

		HW.pContext->Unmap(pTex, 0);

		p.scanlenght = Device.dwHeight * 4;
		p.width = Device.dwHeight;
		p.height = Device.dwHeight;
		p.data = data;
		p.maketga(*fs);
		xr_free(data);
		FS.w_close(fs);
	}
	break;
	}

	_RELEASE(pSrcTexture);
}


void CRender::Screenshot(ScreenshotMode mode, LPCSTR name)
{
	ScreenshotImpl(mode, name, NULL);
}

void CRender::Screenshot(ScreenshotMode mode, CMemoryWriter& memory_writer)
{
	if (mode != SM_FOR_MPSENDING)
	{
		Log("~ Not implemented screenshot mode...");
		return;
	}
	ScreenshotImpl(mode, NULL, &memory_writer);
}

void CRender::ScreenshotAsyncBegin()
{
	VERIFY(!m_bMakeAsyncSS);
	m_bMakeAsyncSS = true;
}


void CRender::ScreenshotAsyncEnd(CMemoryWriter& memory_writer)
{
	VERIFY(!m_bMakeAsyncSS);

	//	Don't own. No need to release.
	ID3DTexture2D* pTex = Target->t_ss_async;

	D3D_MAPPED_TEXTURE2D	MappedData;

	HW.pContext->Map(pTex, 0, D3D_MAP_READ, 0, &MappedData);
	{

		u32* pPixel = (u32*)MappedData.pData;
		u32* pEnd = pPixel + (Device.dwWidth * Device.dwHeight);

		//	Kill alpha and swap r and b.
		for (;pPixel != pEnd; pPixel++)
		{
			u32 p = *pPixel;
			*pPixel = color_xrgb(
				color_get_B(p),
				color_get_G(p),
				color_get_R(p)
			);
		}

		memory_writer.w(&Device.dwWidth, sizeof(Device.dwWidth));
		memory_writer.w(&Device.dwHeight, sizeof(Device.dwHeight));
		memory_writer.w(MappedData.pData, (Device.dwWidth * Device.dwHeight) * 4);
	}

	HW.pContext->Unmap(pTex, 0);
}



void DoAsyncScreenshot()
{
	RImplementation.Target->DoAsyncScreenshot();
}