

#include "general.h"
#include "DXGICaptureSample.h"

#include <Windows.h>

#include <queue>

bool running;
void* bitmap_memory;
BITMAPINFO info = {};

#define BUTTON_RED 28
#define BUTTON_GREEN 117
#define BUTTON_BLUE 188

#define BUTTON2_RED 200
#define BUTTON2_GREEN 37
#define BUTTON2_BLUE 59

void repaint(HDC hdc, u32 width, u32 height)
{
	StretchDIBits(hdc,
		0, 0, width, height,
		0, height, width, -(s32)height,
		bitmap_memory,
		&info,
		DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK WindowProc(_In_ HWND   hwnd, _In_ UINT   uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) 
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC hdc = BeginPaint(hwnd, &paint);
			int width = paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top + 1;

			repaint(hdc, width, height);

			EndPaint(hwnd, &paint);
		} break;

		case WM_CLOSE:
		{
			running = false;
		} break;

		default:
		{
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		} break;
	}
}

HWND create_screenshot_view_window(u32 width, u32 height)
{
	char* window_class = "ScreenshotViewWindow";

	WNDCLASS wc = {};

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = 0;
	wc.lpszClassName = window_class;

	RegisterClass(&wc);
	RECT rec = {};
	rec.left = 0;
	rec.top = 0;
	rec.right = width;
	rec.bottom = height;

	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

	AdjustWindowRect(&rec, style, false);

	HWND wnd = CreateWindowExA(0, window_class, "Screenshot View", style,
		CW_USEDEFAULT, CW_USEDEFAULT, rec.right - rec.left, rec.bottom - rec.top, 0, 0, 0, 0);

	HDC source = CreateCompatibleDC(0);

	BITMAPINFOHEADER info_header = {0};
	info_header.biSize = sizeof(BITMAPINFOHEADER);
	info_header.biWidth = width;
	info_header.biHeight = height;
	info_header.biPlanes = 1;
	info_header.biBitCount = 32;
	info_header.biCompression = BI_RGB;

	info.bmiHeader = info_header;

	CreateDIBSection(source, &info, DIB_RGB_COLORS, &bitmap_memory, 0, 0);

	return wnd;
}

void byreke(u32 width, u32 height, RECT rec)
{
	take_screenshot(bitmap_memory, rec);

	for (u32 x = 0; x < width; x++)
	{
		for (u32 y = 0; y < height; y++)
		{
			u8* pixel = (u8*)((u32*) bitmap_memory + x + (y * width));

			u8 blue = pixel[0];
			u8 green = pixel[1];
			u8 red = pixel[2];

			if (red == BUTTON_RED && green == BUTTON_GREEN && blue == BUTTON_BLUE)
			{
				pixel[0] = 255;
				pixel[1] = 0;
				pixel[2] = 0;
			}
		}
	}

	u32 tol = 10;
	
	for(u32 x = 0; x < width; x++)
	{
		for(u32 y = 0; y < height; y++)
		{
			u8* pixel = (u8*)((u32*)bitmap_memory + x + (y * width));

			u8 blue = pixel[0];
			u8 green = pixel[1];
			u8 red = pixel[2];

			if(red == BUTTON2_RED && green == BUTTON2_GREEN && blue == BUTTON2_BLUE)
			{
				pixel[0] = 0;
				pixel[1] = 0;
				pixel[2] = 0;

				struct pos
				{
					u32 x, y;
				};

				queue<pos> q;

				pos start = { x, y };
				q.push(start);

				while(q.size() > 0)
				{
					pos s = q.front();


					{
						u32 nx = s.x + 1;
						u32 ny = s.y;

						u8* search_pixel = (u8*)((u32*)bitmap_memory + nx + (ny * width));
						u8 sblue = search_pixel[0];
						u8 sgreen = search_pixel[1];
						u8 sred = search_pixel[2];

						u8 bdelta = max(sblue, blue) - min(sblue, blue);
						u8 gdelta = max(sgreen, green) - min(sgreen, green);
						u8 rdelta = max(sred, red) - min(sred, red);

						if(!(sblue == 0 && sgreen == 0 && sred == 0) && bdelta <= tol && gdelta <= tol && rdelta <= tol)
						{
							search_pixel[0] = 0;
							search_pixel[1] = 0;
							search_pixel[2] = 0;

							pos f = { nx, ny };

							q.push(f);
						}
					}

					{
						u32 nx = s.x - 1;
						u32 ny = s.y;

						u8* search_pixel = (u8*)((u32*)bitmap_memory + nx + (ny * width));
						u8 sblue = search_pixel[0];
						u8 sgreen = search_pixel[1];
						u8 sred = search_pixel[2];

						u8 bdelta = max(sblue, blue) - min(sblue, blue);
						u8 gdelta = max(sgreen, green) - min(sgreen, green);
						u8 rdelta = max(sred, red) - min(sred, red);

						if(!(sblue == 0 && sgreen == 0 && sred == 0) && bdelta <= tol && gdelta <= tol && rdelta <= tol)
						{
							search_pixel[0] = 0;
							search_pixel[1] = 0;
							search_pixel[2] = 0;

							pos f = { nx, ny };

							q.push(f);
						}
					}

					{
						u32 nx = s.x;
						u32 ny = s.y + 1;

						u8* search_pixel = (u8*)((u32*)bitmap_memory + nx + (ny * width));
						u8 sblue = search_pixel[0];
						u8 sgreen = search_pixel[1];
						u8 sred = search_pixel[2];

						u8 bdelta = max(sblue, blue) - min(sblue, blue);
						u8 gdelta = max(sgreen, green) - min(sgreen, green);
						u8 rdelta = max(sred, red) - min(sred, red);

						if(!(sblue == 0 && sgreen == 0 && sred == 0) && bdelta <= tol && gdelta <= tol && rdelta <= tol)
						{
							search_pixel[0] = 0;
							search_pixel[1] = 0;
							search_pixel[2] = 0;

							pos f = { nx, ny };

							q.push(f);
						}
					}

					{
						u32 nx = s.x;
						u32 ny = s.y - 1;

						u8* search_pixel = (u8*)((u32*)bitmap_memory + nx + (ny * width));
						u8 sblue = search_pixel[0];
						u8 sgreen = search_pixel[1];
						u8 sred = search_pixel[2];

						u8 bdelta = max(sblue, blue) - min(sblue, blue);
						u8 gdelta = max(sgreen, green) - min(sgreen, green);
						u8 rdelta = max(sred, red) - min(sred, red);

						if(!(sblue == 0 && sgreen == 0 && sred == 0) && bdelta <= tol && gdelta <= tol && rdelta <= tol)
						{
							search_pixel[0] = 0;
							search_pixel[1] = 0;
							search_pixel[2] = 0;

							pos f = { nx, ny };

							q.push(f);
						}
					}

					q.pop();
				}

				return;
			}
		}
	}
}

void analyze_screenshot(HWND wnd)
{
	printf("Analyzing.. \n");

	RECT rec;
	POINT top_left = { 0 };

	GetClientRect(wnd, &rec);

	u32 width = rec.right;
	u32 height = rec.bottom;

	ClientToScreen(wnd, &top_left);
	rec.left = top_left.x;
	rec.top = top_left.y;
	rec.right += rec.left;
	rec.bottom += rec.top;

	init_screenshot();

	HWND screenshot_wnd = create_screenshot_view_window(width, height);

	byreke(width, height, rec);

	running = true;
	while (running)
	{
		MSG Message;
		while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
		{
			if (Message.message == WM_QUIT)
			{
				running = false;
			}

			TranslateMessage(&Message);
			DispatchMessageA(&Message);
		}

		byreke(width, height, rec);

		HDC hdc = GetDC(screenshot_wnd);
		repaint(hdc, width, height);
		ReleaseDC(screenshot_wnd, hdc);
	}
}