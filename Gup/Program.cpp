#include "Window.h"

int WINAPI WinMain(HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	Window* pWindow = new Window();

	bool running = true;
	while (running)
	{
		if (!pWindow->ProcessMessages())
		{
			running = false;
		}

		Sleep(10);
	}

	delete pWindow;

	return 0;
}