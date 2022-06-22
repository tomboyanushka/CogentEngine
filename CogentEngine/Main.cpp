#define NOMINMAX
#include <Windows.h>
#include "Game.h"

#if _MSC_VER >= 1932 // Visual Studio 2022 version 17.2+
#    pragma comment(linker, "/alternatename:__imp___std_init_once_complete=__imp_InitOnceComplete")
#    pragma comment(linker, "/alternatename:__imp___std_init_once_begin_initialize=__imp_InitOnceBeginInitialize")
#endif

// --------------------------------------------------------
// Entry point for a graphical (non-console) Windows application
// --------------------------------------------------------
int WINAPI WinMain(
	HINSTANCE hInstance,		// The handle to this app's instance
	HINSTANCE hPrevInstance,	// A handle to the previous instance of the app (always NULL)
	LPSTR lpCmdLine,			// Command line params
	int nCmdShow)				// How the window should be shown (we ignore this)
{
#if defined(DEBUG) | defined(_DEBUG)

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	{

		char currentDir[1024] = {};
		GetModuleFileName(0, currentDir, 1024);
		char* lastSlash = strrchr(currentDir, '\\');
		if (lastSlash)
		{
			*lastSlash = 0; // End the string at the last slash character
			SetCurrentDirectory(currentDir);
		}
	}

	Game dxGame(hInstance);

	HRESULT hr = S_OK;

	hr = dxGame.InitWindow();
	if (FAILED(hr)) return hr;

	hr = dxGame.InitDirectX();
	if (FAILED(hr)) return hr;

	return dxGame.Run();
}
