#ifndef __WIN_MONITOR_H__
#define __WIN_MONITOR_H__
#include <windows.h>

#ifdef __cplusplus
#define MODULE_EXPORT extern "C" EXPORT
#define MODULE_EXTERN extern "C"
#else
#define MODULE_EXPORT EXPORT
#define MODULE_EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

MODULE_EXPORT BOOL WINAPI SetWinMonitorHook();
MODULE_EXPORT BOOL WINAPI UnSetWinMonitorHook();
MODULE_EXPORT time_t WINAPI GetLastActiveTime();

#ifdef __cplusplus
}
#endif

#endif __WIN_MONITOR_H__
