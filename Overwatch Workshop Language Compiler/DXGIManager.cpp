#include "stdafx.h"
#include "DXGIManager.h"
#include <gdiplus.h>

using namespace Gdiplus;

DXGIPointerInfo::DXGIPointerInfo(BYTE* pPointerShape, UINT uiPointerShapeBufSize, DXGI_OUTDUPL_FRAME_INFO fi, DXGI_OUTDUPL_POINTER_SHAPE_INFO psi)
	:	m_pPointerShape(pPointerShape),
		m_uiPointerShapeBufSize(uiPointerShapeBufSize),
		m_FI(fi),
		m_PSI(psi)
{
}

DXGIPointerInfo::~DXGIPointerInfo()
{
	if(m_pPointerShape)
	{
		delete [] m_pPointerShape;
	}
}

BYTE* DXGIPointerInfo::GetBuffer()
{
	return m_pPointerShape;
}

UINT DXGIPointerInfo::GetBufferSize()
{
	return m_uiPointerShapeBufSize;
}

DXGI_OUTDUPL_FRAME_INFO& DXGIPointerInfo::GetFrameInfo()
{
	return m_FI;
}

DXGI_OUTDUPL_POINTER_SHAPE_INFO& DXGIPointerInfo::GetShapeInfo()
{
	return m_PSI;
}

DXGIOutputDuplication::DXGIOutputDuplication(IDXGIAdapter1* pAdapter,
	ID3D11Device* pD3DDevice,
	ID3D11DeviceContext* pD3DDeviceContext,
	IDXGIOutput1* pDXGIOutput1,
	IDXGIOutputDuplication* pDXGIOutputDuplication)
	:	m_Adapter(pAdapter),
		m_D3DDevice(pD3DDevice),
		m_D3DDeviceContext(pD3DDeviceContext),
		m_DXGIOutput1(pDXGIOutput1),
		m_DXGIOutputDuplication(pDXGIOutputDuplication)
{
}

HRESULT DXGIOutputDuplication::GetDesc(DXGI_OUTPUT_DESC& desc)
{
	m_DXGIOutput1->GetDesc(&desc);
	return S_OK;
}

HRESULT DXGIOutputDuplication::AcquireNextFrame(IDXGISurface1** pDXGISurface, DXGIPointerInfo*& pDXGIPointer)
{
	DXGI_OUTDUPL_FRAME_INFO fi;
	CComPtr<IDXGIResource> spDXGIResource;
	HRESULT hr = m_DXGIOutputDuplication->AcquireNextFrame(20, &fi, &spDXGIResource);
	if(FAILED(hr))
	{
		__L_INFO("m_DXGIOutputDuplication->AcquireNextFrame failed with hr=0x%08x", hr);
		return hr;
	}

	CComQIPtr<ID3D11Texture2D> spTextureResource = (CComQIPtr<ID3D11Texture2D>) spDXGIResource;

	D3D11_TEXTURE2D_DESC desc;
	spTextureResource->GetDesc(&desc);

	D3D11_TEXTURE2D_DESC texDesc;
	ZeroMemory( &texDesc, sizeof(texDesc) );
	texDesc.Width = desc.Width;
	texDesc.Height = desc.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_STAGING;
	texDesc.Format = desc.Format;
	texDesc.BindFlags = 0;
	texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	texDesc.MiscFlags = 0;

	CComPtr<ID3D11Texture2D> spD3D11Texture2D = NULL;
	hr = m_D3DDevice->CreateTexture2D(&texDesc, NULL, &spD3D11Texture2D);
	if(FAILED(hr))
		return hr;

	m_D3DDeviceContext->CopyResource(spD3D11Texture2D, spTextureResource);

	CComQIPtr<IDXGISurface1> spDXGISurface = (CComQIPtr<IDXGISurface1>) spD3D11Texture2D;

	*pDXGISurface = spDXGISurface.Detach();
	
	// Updating mouse pointer, if visible
	if(fi.PointerPosition.Visible)
	{
		BYTE* pPointerShape = new BYTE[fi.PointerShapeBufferSize];

		DXGI_OUTDUPL_POINTER_SHAPE_INFO psi = {};
		UINT uiPointerShapeBufSize = fi.PointerShapeBufferSize;
		hr = m_DXGIOutputDuplication->GetFramePointerShape(uiPointerShapeBufSize, pPointerShape, &uiPointerShapeBufSize, &psi);
		if(hr == DXGI_ERROR_MORE_DATA)
		{
			pPointerShape = new BYTE[uiPointerShapeBufSize];
	
			hr = m_DXGIOutputDuplication->GetFramePointerShape(uiPointerShapeBufSize, pPointerShape, &uiPointerShapeBufSize, &psi);
		}

		if(hr == S_OK)
		{
			__L_INFO("PointerPosition Visible=%d x=%d y=%d w=%d h=%d type=%d\n", fi.PointerPosition.Visible, fi.PointerPosition.Position.x, fi.PointerPosition.Position.y, psi.Width, psi.Height, psi.Type);

			if((psi.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME ||
			    psi.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR ||
			    psi.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR) &&
				psi.Width <= 128 && psi.Height <= 128)
			{
				// Here we can obtain pointer shape
				if(pDXGIPointer)
				{
					delete pDXGIPointer;
				}

				pDXGIPointer = new DXGIPointerInfo(pPointerShape, uiPointerShapeBufSize, fi, psi);
				
				pPointerShape = NULL;
			}

			DXGI_OUTPUT_DESC outDesc;
			GetDesc(outDesc);

			if(pDXGIPointer)
			{
				pDXGIPointer->GetFrameInfo().PointerPosition.Position.x = outDesc.DesktopCoordinates.left + fi.PointerPosition.Position.x;
				pDXGIPointer->GetFrameInfo().PointerPosition.Position.y = outDesc.DesktopCoordinates.top + fi.PointerPosition.Position.y;
			}
		}

		if(pPointerShape)
		{
			delete [] pPointerShape;
		}
	}

	return hr;
}

HRESULT DXGIOutputDuplication::ReleaseFrame()
{
	m_DXGIOutputDuplication->ReleaseFrame();
	return S_OK;
}

bool DXGIOutputDuplication::IsPrimary()
{
	DXGI_OUTPUT_DESC outdesc;
	m_DXGIOutput1->GetDesc(&outdesc);
			
	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(outdesc.Monitor, &mi);
	if(mi.dwFlags & MONITORINFOF_PRIMARY)
	{
		return true;
	}
	return false;
}
