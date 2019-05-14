// DXGICaptureSample.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DXGIManager.h"

#include <gdiplus.h>
#include <assert.h>

bool initalized = false;

CaptureSource capture_source;

vector<DXGIOutputDuplication> global_outputs;

CComPtr<IDXGIFactory1> dxgi_factory;
CComPtr<IWICImagingFactory> imging_factory;

DXGIPointerInfo* dxgi_pointer;

RECT rc_current_output;
BYTE* buffer;

ULONG_PTR gdi_token;

using namespace Gdiplus;

BOOL CALLBACK monitor_enum_proc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	int *Count = (int*) dwData;
	(*Count)++;
	return TRUE;
}

int get_monitor_count()
{
	int Count = 0;
	if(EnumDisplayMonitors(NULL, NULL, monitor_enum_proc, (LPARAM) &Count))
		return Count;
	return -1;
}

HRESULT init()
{
	if(initalized)
		return S_OK;

	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdi_token, &gdiplusStartupInput, NULL);

	HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&dxgi_factory));
	if(FAILED(hr))
	{
		__L_ERROR("Failed to CreateDXGIFactory1 hr=%08x", hr);
		return hr;
	}

	// Getting all adapters
	vector<CComPtr<IDXGIAdapter1>> vAdapters;

	CComPtr<IDXGIAdapter1> spAdapter;
	for(int i = 0; dxgi_factory->EnumAdapters1(i, &spAdapter) != DXGI_ERROR_NOT_FOUND; i++)
	{
		vAdapters.push_back(spAdapter);
		spAdapter.Release();
	}

	// Iterating over all adapters to get all outputs
	for(vector<CComPtr<IDXGIAdapter1>>::iterator AdapterIter = vAdapters.begin();
		AdapterIter != vAdapters.end();
		AdapterIter++)
	{
		vector<CComPtr<IDXGIOutput>> vOutputs;

		CComPtr<IDXGIOutput> spDXGIOutput;
		for(int i = 0; (*AdapterIter)->EnumOutputs(i, &spDXGIOutput) != DXGI_ERROR_NOT_FOUND; i++)
		{
			DXGI_OUTPUT_DESC outputDesc;
			spDXGIOutput->GetDesc(&outputDesc);

			__L_INFO("Display output found. DeviceName=%ls  AttachedToDesktop=%d Rotation=%d DesktopCoordinates={(%d,%d),(%d,%d)}",
				outputDesc.DeviceName,
				outputDesc.AttachedToDesktop,
				outputDesc.Rotation,
				outputDesc.DesktopCoordinates.left,
				outputDesc.DesktopCoordinates.top,
				outputDesc.DesktopCoordinates.right,
				outputDesc.DesktopCoordinates.bottom);

			if(outputDesc.AttachedToDesktop)
			{
				vOutputs.push_back(spDXGIOutput);
			}

			spDXGIOutput.Release();
		}

		if(vOutputs.size() == 0)
			continue;

		// Creating device for each adapter that has the output
		CComPtr<ID3D11Device> spD3D11Device;
		CComPtr<ID3D11DeviceContext> spD3D11DeviceContext;
		D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_9_1;
		hr = D3D11CreateDevice((*AdapterIter), D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &spD3D11Device, &fl, &spD3D11DeviceContext);
		if(FAILED(hr))
		{
			__L_ERROR("Failed to create D3D11CreateDevice hr=%08x", hr);
			return hr;
		}

		for(std::vector<CComPtr<IDXGIOutput>>::iterator OutputIter = vOutputs.begin();
			OutputIter != vOutputs.end();
			OutputIter++)
		{
			CComQIPtr<IDXGIOutput1> spDXGIOutput1 = (CComQIPtr<IDXGIOutput1>) (*OutputIter);
			if(!spDXGIOutput1)
			{
				__L_ERROR("spDXGIOutput1 is NULL");
				continue;
			}

			CComQIPtr<IDXGIDevice1> spDXGIDevice = (CComQIPtr<IDXGIDevice1>) spD3D11Device;
			if(!spDXGIDevice)
			{
				__L_ERROR("spDXGIDevice is NULL");
				continue;
			}

			CComPtr<IDXGIOutputDuplication> spDXGIOutputDuplication;
			hr = spDXGIOutput1->DuplicateOutput(spDXGIDevice, &spDXGIOutputDuplication);
			if(FAILED(hr))
			{
				__L_ERROR("Failed to duplicate output hr=%08x", hr);
				continue;
			}

			global_outputs.push_back(
				DXGIOutputDuplication((*AdapterIter),
					spD3D11Device,
					spD3D11DeviceContext,
					spDXGIOutput1,
					spDXGIOutputDuplication));
		}
	}

	hr = imging_factory.CoCreateInstance(CLSID_WICImagingFactory);
	if(FAILED(hr))
	{
		__L_ERROR("Failed to create WICImagingFactory hr=%08x", hr);
		return hr;
	}

	initalized = true;

	return S_OK;
}

vector<DXGIOutputDuplication> get_output_duplication()
{
	vector<DXGIOutputDuplication> outputs;
	switch(capture_source)
	{
	case CSMonitor1:
	{
		// Return the one with IsPrimary
		for(vector<DXGIOutputDuplication>::iterator iter = global_outputs.begin();
			iter != global_outputs.end();
			iter++)
		{
			DXGIOutputDuplication& out = *iter;
			if(out.IsPrimary())
			{
				outputs.push_back(out);
				break;
			}
		}
	}
	break;

	case CSMonitor2:
	{
		// Return the first with !IsPrimary
		for(vector<DXGIOutputDuplication>::iterator iter = global_outputs.begin();
			iter != global_outputs.end();
			iter++)
		{
			DXGIOutputDuplication& out = *iter;
			if(!out.IsPrimary())
			{
				outputs.push_back(out);
				break;
			}
		}
	}
	break;

	case CSDesktop:
	{
		// Return all outputs
		for(vector<DXGIOutputDuplication>::iterator iter = global_outputs.begin();
			iter != global_outputs.end();
			iter++)
		{
			DXGIOutputDuplication& out = *iter;
			outputs.push_back(out);
		}
	}
	break;
	}
	return outputs;
}

HRESULT get_output_rect(RECT& rc)
{
	// Nulling rc just in case...
	SetRect(&rc, 0, 0, 0, 0);

	HRESULT hr = init();
	if(hr != S_OK)
		return hr;

	vector<DXGIOutputDuplication> vOutputs = get_output_duplication();

	RECT rcShare;
	SetRect(&rcShare, 0, 0, 0, 0);

	for(vector<DXGIOutputDuplication>::iterator iter = vOutputs.begin();
		iter != vOutputs.end();
		iter++)
	{
		DXGIOutputDuplication& out = *iter;

		DXGI_OUTPUT_DESC outDesc;
		out.GetDesc(outDesc);
		RECT rcOutCoords = outDesc.DesktopCoordinates;

		UnionRect(&rcShare, &rcShare, &rcOutCoords);
	}

	CopyRect(&rc, &rcShare);

	return S_OK;
}

void draw_mouse_pointer(BYTE* pDesktopBits, RECT rcDesktop, RECT rcDest)
{
	if(!dxgi_pointer)
		return;

	DWORD dwDesktopWidth = rcDesktop.right - rcDesktop.left;
	DWORD dwDesktopHeight = rcDesktop.bottom - rcDesktop.top;

	DWORD dwDestWidth = rcDest.right - rcDest.left;
	DWORD dwDestHeight = rcDest.bottom - rcDest.top;

	int PtrX = dxgi_pointer->GetFrameInfo().PointerPosition.Position.x - rcDesktop.left;
	int PtrY = dxgi_pointer->GetFrameInfo().PointerPosition.Position.y - rcDesktop.top;
	switch(dxgi_pointer->GetShapeInfo().Type)
	{
	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR:
	{
		unique_ptr<Bitmap> bmpBitmap(new Bitmap(dwDestWidth, dwDestHeight, dwDestWidth * 4, PixelFormat32bppARGB, pDesktopBits));
		unique_ptr<Graphics> graphics(Graphics::FromImage(bmpBitmap.get()));
		unique_ptr<Bitmap> bmpPointer(new Bitmap(dxgi_pointer->GetShapeInfo().Width, dxgi_pointer->GetShapeInfo().Height, dxgi_pointer->GetShapeInfo().Width * 4, PixelFormat32bppARGB, dxgi_pointer->GetBuffer()));

		graphics->DrawImage(bmpPointer.get(), PtrX, PtrY);
	}
	break;
	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME:
	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR:
	{
		RECT rcPointer;

		if(dxgi_pointer->GetShapeInfo().Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME)
		{
			SetRect(&rcPointer, PtrX, PtrY, PtrX + dxgi_pointer->GetShapeInfo().Width, PtrY + dxgi_pointer->GetShapeInfo().Height / 2);
		}
		else
		{
			SetRect(&rcPointer, PtrX, PtrY, PtrX + dxgi_pointer->GetShapeInfo().Width, PtrY + dxgi_pointer->GetShapeInfo().Height);
		}

		RECT rcDesktopPointer;
		IntersectRect(&rcDesktopPointer, &rcPointer, &rcDesktop);

		CopyRect(&rcPointer, &rcDesktopPointer);
		OffsetRect(&rcPointer, -PtrX, -PtrY);

		BYTE* pShapeBuffer = dxgi_pointer->GetBuffer();
		UINT* pDesktopBits32 = (UINT*)pDesktopBits;

		if(dxgi_pointer->GetShapeInfo().Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME)
		{
			for(int j = rcPointer.top, jDP = rcDesktopPointer.top;
				j < rcPointer.bottom && jDP < rcDesktopPointer.bottom;
				j++, jDP++)
			{
				for(int i = rcPointer.left, iDP = rcDesktopPointer.left;
					i < rcPointer.right && iDP < rcDesktopPointer.right;
					i++, iDP++)
				{
					BYTE Mask = 0x80 >> (i % 8);
					BYTE AndMask = pShapeBuffer[i / 8 + (dxgi_pointer->GetShapeInfo().Pitch)*j] & Mask;
					BYTE XorMask = pShapeBuffer[i / 8 + (dxgi_pointer->GetShapeInfo().Pitch)*(j + dxgi_pointer->GetShapeInfo().Height / 2)] & Mask;

					UINT AndMask32 = (AndMask) ? 0xFFFFFFFF : 0xFF000000;
					UINT XorMask32 = (XorMask) ? 0x00FFFFFF : 0x00000000;

					pDesktopBits32[jDP*dwDestWidth + iDP] = (pDesktopBits32[jDP*dwDestWidth + iDP] & AndMask32) ^ XorMask32;
				}
			}
		}
		else
		{
			UINT* pShapeBuffer32 = (UINT*)pShapeBuffer;
			for(int j = rcPointer.top, jDP = rcDesktopPointer.top;
				j < rcPointer.bottom && jDP < rcDesktopPointer.bottom;
				j++, jDP++)
			{
				for(int i = rcPointer.left, iDP = rcDesktopPointer.left;
					i < rcPointer.right && iDP < rcDesktopPointer.right;
					i++, iDP++)
				{
					// Set up mask
					UINT MaskVal = 0xFF000000 & pShapeBuffer32[i + (dxgi_pointer->GetShapeInfo().Pitch / 4)*j];
					if(MaskVal)
					{
						// Mask was 0xFF
						pDesktopBits32[jDP*dwDestWidth + iDP] = (pDesktopBits32[jDP*dwDestWidth + iDP] ^ pShapeBuffer32[i + (dxgi_pointer->GetShapeInfo().Pitch / 4)*j]) | 0xFF000000;
					}
					else
					{
						// Mask was 0x00 - replacing pixel
						pDesktopBits32[jDP*dwDestWidth + iDP] = pShapeBuffer32[i + (dxgi_pointer->GetShapeInfo().Pitch / 4)*j];
					}
				}
			}
		}
	}
	break;
	}
}

HRESULT get_output_bits(BYTE* pBits, RECT& rcDest)
{
	HRESULT hr = S_OK;

	DWORD dwDestWidth = rcDest.right - rcDest.left;
	DWORD dwDestHeight = rcDest.bottom - rcDest.top;

	RECT rcOutput;
	hr = get_output_rect(rcOutput);
	if(FAILED(hr))
		return hr;

	DWORD dwOutputWidth = rcDest.right - rcDest.left;
	DWORD dwOutputHeight = rcDest.bottom - rcDest.top;

	BYTE* pBuf = NULL;
	if(rcOutput.right > (LONG)dwDestWidth || rcOutput.bottom > (LONG)dwDestHeight)
	{
		dwOutputWidth = dwDestWidth;
		dwOutputHeight = dwDestHeight;

		// Output is larger than pBits dimensions
		if(!buffer || !EqualRect(&rc_current_output, &rcOutput))
		{
			DWORD dwBufSize = dwOutputWidth * dwOutputHeight * 4;

			if(buffer)
			{
				delete[] buffer;
				buffer = NULL;
			}

			buffer = new BYTE[dwBufSize];

			CopyRect(&rc_current_output, &rcOutput);
		}

		pBuf = buffer;
	}
	else
	{
		// Output is smaller than pBits dimensions
		pBuf = pBits;
		dwOutputWidth = dwDestWidth;
		dwOutputHeight = dwDestHeight;
	}

	vector<DXGIOutputDuplication> vOutputs = get_output_duplication();

	for(vector<DXGIOutputDuplication>::iterator iter = vOutputs.begin();
		iter != vOutputs.end();
		iter++)
	{
		DXGIOutputDuplication& out = *iter;

		DXGI_OUTPUT_DESC outDesc;
		out.GetDesc(outDesc);
		RECT rcOutCoords = outDesc.DesktopCoordinates;

		CComPtr<IDXGISurface1> spDXGISurface1;
		hr = out.AcquireNextFrame(&spDXGISurface1, dxgi_pointer);
		if(FAILED(hr))
			break;

		DXGI_MAPPED_RECT map;
		spDXGISurface1->Map(&map, DXGI_MAP_READ);

		RECT rcDesktop = rcDest;
		DWORD dwWidth = rcDest.right - rcDest.left;
		DWORD dwHeight = rcDest.bottom - rcDest.top;

		OffsetRect(&rcDesktop, -rcDest.left, -rcDest.top);

		DWORD dwMapPitchPixels = map.Pitch / 4;

		switch(outDesc.Rotation)
		{
		case DXGI_MODE_ROTATION_IDENTITY:
		{
			// Just copying
			DWORD dwStripe = dwWidth * 4;
			for(unsigned int i = 0; i < dwHeight; i++)
			{
				memcpy_s(pBuf + (rcDesktop.left + (i + rcDesktop.top)*dwOutputWidth) * 4, dwStripe, (rcDest.left * 4) + map.pBits + (i + rcDest.top) * map.Pitch, dwStripe);
			}
		}
		break;
		case DXGI_MODE_ROTATION_ROTATE90:
		{
			// Rotating at 90 degrees
			DWORD* pSrc = (DWORD*)map.pBits;
			DWORD* pDst = (DWORD*)pBuf;
			for(unsigned int j = 0; j < dwHeight; j++)
			{
				for(unsigned int i = 0; i < dwWidth; i++)
				{
					*(pDst + (rcDesktop.left + (j + rcDesktop.top)*dwOutputWidth) + i) = *(pSrc + j + dwMapPitchPixels * (dwWidth - i - 1));
				}
			}
		}
		break;
		case DXGI_MODE_ROTATION_ROTATE180:
		{
			// Rotating at 180 degrees
			DWORD* pSrc = (DWORD*)map.pBits;
			DWORD* pDst = (DWORD*)pBuf;
			for(unsigned int j = 0; j < dwHeight; j++)
			{
				for(unsigned int i = 0; i < dwWidth; i++)
				{
					*(pDst + (rcDesktop.left + (j + rcDesktop.top)*dwOutputWidth) + i) = *(pSrc + (dwWidth - i - 1) + dwMapPitchPixels * (dwHeight - j - 1));
				}
			}
		}
		break;
		case DXGI_MODE_ROTATION_ROTATE270:
		{
			// Rotating at 270 degrees
			DWORD* pSrc = (DWORD*)map.pBits;
			DWORD* pDst = (DWORD*)pBuf;
			for(unsigned int j = 0; j < dwHeight; j++)
			{
				for(unsigned int i = 0; i < dwWidth; i++)
				{
					*(pDst + (rcDesktop.left + (j + rcDesktop.top)*dwOutputWidth) + i) = *(pSrc + (dwHeight - j - 1) + dwMapPitchPixels * i);
				}
			}
		}
		break;
		}

		spDXGISurface1->Unmap();

		out.ReleaseFrame();
	}

	if(FAILED(hr))
		return hr;

	// Now pBits have the desktop. Let's paint mouse pointer!
	if(pBuf != pBits)
	{
		draw_mouse_pointer(pBuf, rcDest, rcDest);
	}
	else
	{
		draw_mouse_pointer(pBuf, rcDest, rcDest);
	}

	// We have the pBuf filled with current desktop/monitor image.
	if(pBuf != pBits)
	{
		// pBuf contains the image that should be resized
		CComPtr<IWICBitmap> spBitmap = NULL;
		hr = imging_factory->CreateBitmapFromMemory(dwOutputWidth, dwOutputHeight, GUID_WICPixelFormat32bppBGRA, dwOutputWidth * 4, dwOutputWidth*dwOutputHeight * 4, (BYTE*)pBuf, &spBitmap);
		if(FAILED(hr))
			return hr;

		CComPtr<IWICBitmapScaler> spBitmapScaler = NULL;
		hr = imging_factory->CreateBitmapScaler(&spBitmapScaler);
		if(FAILED(hr))
			return hr;

		dwOutputWidth = rcDest.right - rcDest.left;
		dwOutputHeight = rcDest.bottom - rcDest.top;

		double aspect = (double)dwOutputWidth / (double)dwOutputHeight;

		DWORD scaledWidth = dwDestWidth;
		DWORD scaledHeight = dwDestHeight;

		if(aspect > 1)
		{
			scaledWidth = dwDestWidth;
			scaledHeight = (DWORD)(dwDestWidth / aspect);
		}
		else
		{
			scaledWidth = (DWORD)(aspect*dwDestHeight);
			scaledHeight = dwDestHeight;
		}

		spBitmapScaler->Initialize(
			spBitmap, scaledWidth, scaledHeight, WICBitmapInterpolationModeNearestNeighbor);

		spBitmapScaler->CopyPixels(NULL, scaledWidth * 4, dwDestWidth*dwDestHeight * 4, pBits);
	}
	return hr;
}

HRESULT run(RECT rec)
{
	printf("DXGICaptureSample. Fast windows screen capture\n");
	printf("Capturing desktop to: capture.bmp\n");
	printf("Log: logfile.log\n");

	// init
	capture_source = CSUndefined;
	SetRect(&rc_current_output, 0, 0, 0, 0);
	buffer = NULL;
	dxgi_pointer = NULL;

	// init com library
	CoInitialize(NULL);

	capture_source = CSDesktop;

	DWORD dwWidth = rec.right - rec.left;
	DWORD dwHeight = rec.bottom - rec.top;

	printf("dwWidth=%d dwHeight=%d\n", dwWidth, dwHeight);

	DWORD dwBufSize = dwWidth * dwHeight * 4;

	BYTE* pBuf = new BYTE[dwBufSize];

	CComPtr<IWICImagingFactory> spWICFactory = NULL;
	HRESULT hr = spWICFactory.CoCreateInstance(CLSID_WICImagingFactory);
	if(FAILED(hr))
		return hr;

	int i = 0;
	do
	{
		hr = get_output_bits(pBuf, rec);
		i++;
	} while(hr == DXGI_ERROR_WAIT_TIMEOUT || i < 2);

	if(FAILED(hr))
	{
		printf("GetOutputBits failed with hr=0x%08x\n", hr);
		return hr;
	}

	printf("Saving capture to file\n");

	CComPtr<IWICBitmap> spBitmap = NULL;
	hr = spWICFactory->CreateBitmapFromMemory(dwWidth, dwHeight, GUID_WICPixelFormat32bppBGRA, dwWidth * 4, dwBufSize, (BYTE*) pBuf, &spBitmap);
	if(FAILED(hr))
		return hr;

	CComPtr<IWICStream> spStream = NULL;

	hr = spWICFactory->CreateStream(&spStream);
	if(FAILED(hr)) {
		return hr;
	}

	hr = spStream->InitializeFromFilename(L"capture.bmp", GENERIC_WRITE);

	CComPtr<IWICBitmapEncoder> spEncoder = NULL;
	if(FAILED(hr)) {
		return hr;
	}

	hr = spWICFactory->CreateEncoder(GUID_ContainerFormatBmp, NULL, &spEncoder);

	if(FAILED(hr)) {
		return hr;
	}

	hr = spEncoder->Initialize(spStream, WICBitmapEncoderNoCache);

	CComPtr<IWICBitmapFrameEncode> spFrame = NULL;
	if(FAILED(hr)) {
		return hr;
	}
	
	hr = spEncoder->CreateNewFrame(&spFrame, NULL);

	if(FAILED(hr)) {
		return hr;
	}

	hr = spFrame->Initialize(NULL);

	if(FAILED(hr)) {
		return hr;
	}

	hr = spFrame->SetSize(dwWidth, dwHeight);

	WICPixelFormatGUID format;
	spBitmap->GetPixelFormat(&format);

	if(FAILED(hr)) {
		return hr;
	}

	hr = spFrame->SetPixelFormat(&format);

	if(FAILED(hr)) {
		return hr;
	}

	hr = spFrame->WriteSource(spBitmap, NULL);

	if(FAILED(hr)) {
		return hr;
	}

	hr = spFrame->Commit();

	if(FAILED(hr)) {
		return hr;
	}

	hr = spEncoder->Commit();

	if(FAILED(hr)) {
		return hr;
	}

	delete[] pBuf;

	GdiplusShutdown(gdi_token);

	if(buffer)
	{
		delete[] buffer;
		buffer = NULL;
	}

	if(dxgi_pointer)
	{
		delete dxgi_pointer;
		dxgi_pointer = NULL;
	}
}

