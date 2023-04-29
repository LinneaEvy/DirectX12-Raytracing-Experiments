#include "Window.h"
#include <tchar.h>
#include <iostream>
#include "D3D12.h"
#include "Graphics.h"

/*void D3D12::OnInit()
{
	LoadPipeline();
	LoadAssets();
}
void D3D12::OnUpdate()

{
}
void D3D12::OnRender()
{
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
}*/

const wchar_t* GetWC(const char* c)
{
	const char* errorInfo = c;
	const size_t cSize = strlen(errorInfo) + 1;
	wchar_t* wc = new wchar_t[cSize];
	size_t tmp = 0;
	mbstowcs_s(&tmp, wc, cSize, errorInfo, cSize);

	return wc;
}
int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow) 
{
	try
	{
		Window wnd(800, 300, L"Fenster in die welt des Chaos");
		MSG msg;
		BOOL gResult;
		//look how.app does pass around the window
		Graphics renderer(wnd.GetWInstance(), 800, 800, wnd.GetWName());
		
		
		while ((gResult = GetMessage(&msg, nullptr, 0, 0)) > 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (wnd.kbd.KeyIsPressed(WM_PAINT)) {
				renderer.Update();
				renderer.Render();
			}
			if (wnd.kbd.KeyIsPressed(VK_MENU))
			{
				MessageBox(nullptr, L"das Licht geht an", L"The alt key was pressed", MB_OK | MB_ICONEXCLAMATION);
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