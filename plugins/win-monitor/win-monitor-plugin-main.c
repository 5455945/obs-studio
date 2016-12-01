#include <obs-module.h>
#include <windows.h>
#include "win-monitor.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-monitor", "en-US")

bool obs_module_load(void)
{
	BOOL bRet = FALSE;
	// 安装钩子
#ifndef _DEBUG
	bRet = SetWinMonitorHook();
#endif

	return (bRet == TRUE);
}

void obs_module_unload(void)
{
	// 卸载钩子
	//UnSetWinMonitorHook();
}
