#include "Window.h"
#include <tchar.h>
#include <iostream>

#include "Graphics.h"
#include "WindowsThrowMacros.h"


const wchar_t* GetWC(const char* c)
{
	const char* errorInfo = c;
	const size_t cSize = strlen(errorInfo) + 1;
	wchar_t* wc = new wchar_t[cSize];
	size_t tmp = 0;
	mbstowcs_s(&tmp, wc, cSize, errorInfo, cSize);

	return wc;
}


int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	try
	{
		Window wnd(800, 300, L"Fenster in die welt des Chaos");
		MSG msg;
		BOOL gResult;
		//look how.app does pass around the window
		Graphics renderer(wnd.GetWInstance(), 800, 800, wnd.GetWName());
		
		bool n = true;
		while ((gResult = GetMessage(&msg, nullptr, 0, 0)) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);


			if (wnd.kbd.KeyIsPressed(VK_MENU))//alt
			{
				MessageBox(nullptr, L"das Licht geht an", L"licht", MB_OK | MB_ICONEXCLAMATION);

			}
			if (wnd.Graphicsmsg == WM_PAINT) {
				renderer.Update();
				renderer.Render();
			}
			if (wnd.Graphicsmsg == VK_F11)
			{
				MessageBox(nullptr, L"das Licht geht an", L"f11", MB_OK | MB_ICONEXCLAMATION);
				renderer.SetFullscreen(true);
			}
			if (wnd.kbd.KeyIsPressed(WM_SIZE))
			{
				RECT clientRect = {};
				::GetClientRect(wnd.GetWInstance(), &clientRect);

				int width = clientRect.right - clientRect.left;
				int height = clientRect.bottom - clientRect.top;

				renderer.Resize(width, height);
			}

		}
		// check if GetMessage call itself worked
		if (gResult == -1)
		{
			
			throw CHWND_LAST_EXCEPT();
			
		}

		// wParam here is the value passed to PostQuitMessage
		return msg.wParam;
	}
	catch (const ChiliException& e)
	{
		MessageBox(nullptr, GetWC(e.what()), GetWC(e.GetType()), MB_OK | MB_ICONEXCLAMATION);
	}
	catch (const std::exception& e)
	{
		MessageBox(nullptr, GetWC(e.what()), L"Standard Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	catch (...)
	{
		MessageBox(nullptr, L"No details available", L"Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
	}
	return -1;
}