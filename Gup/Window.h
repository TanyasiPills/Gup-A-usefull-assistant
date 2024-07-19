#pragma once

#include <Windows.h>

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class Window
{
public:
	Window();
	~Window();
	bool ProcessMessages();
private:
	HINSTANCE m_hInstance;
	HWND m_hWnd;
};