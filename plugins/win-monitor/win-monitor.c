#include <stdio.h>
#include <util/dstr.h>
#include <util/darray.h>
#include <util/platform.h>
#include <obs-module.h>
#include "win-monitor.h"
#include "windows.h"
#include "tchar.h"
#include <Psapi.h>
#pragma comment(lib,"Psapi.lib")

#ifndef _STDINT_H_INCLUDED
#define _STDINT_H_INCLUDED
#endif


#define do_log(level, format, ...) \
	blog(level, "[win-monitor: '%s'] " format, "", ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)


// 建立数据段
#pragma data_seg("HookData")
HHOOK  g_hMouse = NULL;     // 鼠标hook句柄
HHOOK  g_hKeyboard = NULL;  // 键盘hook句柄
HANDLE g_hInstance = NULL;
DWORD volatile g_lLastActiveTime = 0;
#pragma data_seg()
// 设置数据段为可读可写可共享
#pragma comment(linker,"/SECTION:HookData,RWS")


BOOL APIENTRY DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	// 保存模块实例句柄
	g_hInstance = (HINSTANCE)hModule;

	// 在进程结束或线程结束时卸载钩子
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//OutputDebugStringA("\n000001    DLL_PROCESS_ATTACH \n");
		//if (!g_hActive) {
		//	g_hActive = SetWindowsHookExW(WH_CBT, ForegroundWindowProc, g_hInstance, 0);
		//}
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		UnSetWinMonitorHook();
		break;
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	//BOOL bctrl = GetAsyncKeyState(VK_CONTROL) >> ((sizeof(SHORT) * 8) - 1);
	KBDLLHOOKSTRUCT* pStruct = (KBDLLHOOKSTRUCT*)lParam;
	switch (wParam)
	{
	case WM_KEYDOWN:
		switch (pStruct->vkCode) {
		case VK_LWIN:
		case VK_RWIN:
			break;
		default:
			break;
		}
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	default:
		break;
	}
	
	DWORD lLastActiveTime = GetTickCount();
	InterlockedExchange(&g_lLastActiveTime, lLastActiveTime);

	// 传给系统中的下一个钩子
	return CallNextHookEx(g_hKeyboard, nCode, wParam, lParam);
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		PMOUSEHOOKSTRUCT mhs = (PMOUSEHOOKSTRUCT)lParam;
		switch (wParam)
		{
		case WM_NCLBUTTONDOWN:
			break;
		case WM_NCLBUTTONUP:
			break;
		case WM_LBUTTONUP:
			break;
		case WM_MOUSEWHEEL:
			break;
		default:
			break;
		}
	}

	DWORD lLastActiveTime = GetTickCount();
	InterlockedExchange(&g_lLastActiveTime, lLastActiveTime);

	return CallNextHookEx(g_hMouse, nCode, wParam, lParam);
}

BOOL WINAPI SetWinMonitorHook()
{
	HMODULE hModule = GetModuleHandleA("win-monitor.dll");

	g_hMouse = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hModule, 0);
	if (g_hMouse == NULL) {
		warn("SetWinMonitorHook error, Mouse set hook fail");
	}
	g_hKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hModule, 0);
	if (g_hKeyboard == NULL) {
		warn("SetWinMonitorHook error, Mouse set hook fail");
	}
	
	return TRUE;
}

BOOL WINAPI UnSetWinMonitorHook()
{
	BOOL bRet = FALSE;
	if (g_hMouse) {
		bRet = UnhookWindowsHookEx(g_hMouse);
		if (!bRet)
		{
			warn("UnhookWindowsHookEx error, Mouse Unhook fail");
		}
		else {
			g_hMouse = NULL;
		}
	}

	if (g_hKeyboard) {
		bRet = UnhookWindowsHookEx(g_hKeyboard);
		if (!bRet)
		{
			warn("UnhookWindowsHookEx error, Keyboard Unhook fail");
		}
		else {
			g_hKeyboard = NULL;
		}
	}

	return bRet;
}

DWORD WINAPI GetLastActiveTime()
{
	return g_lLastActiveTime;
}
