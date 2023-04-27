#pragma once
#include "HighIncWindows.h"
#include "Keyboard.h"
#include "ChiliException.h"
#include "Mouse.h"
class Window
{
public:
	class Exception : public ChiliException
	{
	public:
		Exception(int line, const char* file, HRESULT hr) noexcept;
		const char* what() const noexcept override;
		virtual const char* GetType() const noexcept;
		static std::string TranslateErrorCode(HRESULT hr) noexcept;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorString() const noexcept;
	private:
		HRESULT hr;
	};
private:
	class WindowClass
	{
	public:

		static const LPCWSTR GetName() noexcept;
		static HINSTANCE GetInstance() noexcept;
	private:
		WindowClass();
		~WindowClass();
		WindowClass(const WindowClass&) = delete;
		WindowClass& operator=(const WindowClass&) = delete;
		static constexpr const LPCWSTR wndClassName = L"LYS";
		static WindowClass wndClass;
		HINSTANCE hInst;
	};
public:
	Window(int width, int height, const LPCWSTR name);
	~Window();
	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	void SetTitle(const std::string& title);
private:
	static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
	LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;//use as member function
public:
	Keyboard kbd;
	Mouse mouse;
private:
	int width;
	int height;
	HWND hWnd;
};
#define CHWND_EXEPT(hr) Window::Exception(__LINE__,__FILE__,hr)


#define CHWND_LAST_EXCEPT() Window::Exception( __LINE__,__FILE__,GetLastError() )