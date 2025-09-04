#include <cstdio>
#include "pch.hpp"
//Debug mem leak check
#include <crtdbg.h>
//#include <WinBase.h>
#include <windows.h>
/*
	Open Console in DEBUG mode (Project --> Linker --> System --> Subsystem == Console)
	Open without console in RELEASE mode (Subsystem == Windows).
*/
#ifdef _DEBUG
int main(int argc, char* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
#endif
#ifndef _DEBUG
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

		// Enable run-time memory check for debug builds.
		//Disable asan if enabled.
#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif


}