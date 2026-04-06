/*
	Windows Wrapper for game loop functions
	Entry point for windows, to call the application functions.
*/
#include "pch.hpp"
#ifdef PLATFORM_WINDOWS
/*
	Sets the application to not use console if in release mode.
*/
#ifdef NDEBUG
//#pragma comment(linker, "/SUBSYSTEM:WINDOWS")
#endif
#ifdef DEBUG
#define _CRTDBG_MAP_ALLOC
#endif
//Debug mem leak check
#include <crtdbg.h>

#include <thread>
#include <chrono>

//#include <WinBase.h>
#include <windows.h>
#include "main.hpp"
#include "GraphicsSystem.h" // Include GraphicsSystem header

/*
	The entry point of the program.
	Open Console in DEBUG mode (Project --> Linker --> System --> Subsystem == Console)
	Open without console in RELEASE mode (Subsystem == Windows).
*/

#ifdef TRUE //_DEBUG
int main(int argc, char* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

#else
	int wWinMain(_In_ HINSTANCE hInstance,
		_In_opt_ HINSTANCE hPrevInstance,
		_In_ LPWSTR lpCmdLine,
		_In_ int nShowCmd)
	{
		UNREFERENCED_PARAMETER(lpCmdLine);
		UNREFERENCED_PARAMETER(hInstance);
		UNREFERENCED_PARAMETER(hPrevInstance);
		UNREFERENCED_PARAMETER(nShowCmd);
#endif
		Game_Initialize();
		// Use the stored GraphicsSystem pointer to check window state
		//This is windows specific
		while (!glfwWindowShouldClose(EngineFramework::GraphicsSystem::GetInstance().GetGLFWwindow())) // Replace 'true' with window check
		{
			Game_Update();
		}
		Game_Destroy();
		return 0;
}
#endif