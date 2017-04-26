#pragma once
#include <QtCore/qobject.h>
#include <QTimer>
#include <QThread>
#include <QPointer>
#include "windows.h"
#include "tchar.h"
using namespace std;

// 监控版本号，为了区分不同客户端的日志处理类型，每次修改客户端数据格式时，需要修改这个版本值
// 监控版本号必须大于等于1
#define WINMONITOR_VERSION    1
// 单个种类日志最大上传字节数为1MB
#define UPLOAD_FILE_MAX_SIZE  (1024*1024)
// 活动窗口监控个日志上次上传成功截至时间记录文件
#define UPLOAD_SUCCESS_LASTTIME_AW   "ltaw.mon"
// 鼠标键盘监控个日志上次上传成功截至时间记录文件
#define UPLOAD_SUCCESS_LASTTIME_MK   "ltmk.mon"
typedef time_t (WINAPI *PFUN_GetLastActiveTime)();  // 鼠标键盘最后活动时间获取函数指定

// 当前活动窗口监控信息
typedef struct _ACTIVE_WINDOW_INFO {
	TCHAR  szProcessName[MAX_PATH];   // 进程名称
	TCHAR  szTitle[MAX_PATH];         // 活动窗口名称
	TCHAR  szClassName[MAX_PATH];     // 活动窗口类名称
	time_t tStartTime;                // 开始活动时间
	time_t tEndTime;                  // 结束活动时间
	HWND   hwnd;                      // 活动窗口句柄
	int    nVersion;                  // 监控版本号
	_ACTIVE_WINDOW_INFO() {
		memset(this, 0, sizeof(_ACTIVE_WINDOW_INFO));
	};
} ACTIVE_WINDOW_INFO, *PACTIVE_WINDOW_INFO;

// 鼠标键盘活动监控信息
typedef struct _MOUSE_KEYBOARD_INFO {
	time_t  tStartTime;               // 鼠标键盘不活动起始时间
	time_t  tCheckTime;               // 鼠标键盘不活动最后检查时间
	int     nVersion;                 // 监控版本号
	_MOUSE_KEYBOARD_INFO() {
		memset(this, 0, sizeof(_MOUSE_KEYBOARD_INFO));
	};
}MOUSE_KEYBOARD_INFO, *PMOUSE_KEYBOARD_INFO;

// WinMonitor监控活动窗口和鼠标、键盘活动，并向web服务器上报
class WinMonitor : public QObject
{
	Q_OBJECT

public:
	explicit WinMonitor();
	~WinMonitor();
	bool Init();
	// 更新配置
	bool UpdateConfig();
	static HWND GetLastActiveHwnd() { return m_hLastActiveHwnd; }
	static void SetLastActiveHwnd(HWND hwnd) { m_hLastActiveHwnd = hwnd; }
	// 根据进程IP获得进程名称
	static bool GetProcessName(DWORD dwProcessId, LPTSTR lpszProcessName);
	// 获得监控模块版本信息
	static int GetMonitorVersion();
	// 写入活动窗口监控信息
	static void WriteActiveWindowFile(ACTIVE_WINDOW_INFO awi);
	// 写入最后一条活动窗口日志，结束软件时
	static void WriteLastActiveWindowFile();
	static void SetLandingServerTime(time_t tlanding_server_time, time_t tlanding_client_tick_count);
	static time_t m_tlanding_server_time;             // 登陆时，服务端时间戳
	static time_t m_tlanding_client_tick_count;       // 登陆时，客户端GetTickCount()
	static time_t GetServerTime();

private:
	QPointer<QThread> m_MonitorUploadThread;   // 向web端发送监控信息的线程
	QPointer<QTimer>  m_MonitorUploadTimer;    // 向web端发送监控信息的Timer
	QPointer<QTimer>  m_ActiveWindowTimer;     // 检测活动窗口信息的Timer
	QPointer<QTimer>  m_MouseKeyboardTimer;    // 检测鼠标键盘超时信息的Timer
	int    m_nActiveWindowCheckInterval;       // 活动窗口检查时间间隔,秒
	int    m_nMouseKeyboardCheckInterval;      // 鼠标键盘活动检查时间间隔,秒
	int    m_nMonitorUploadInterval;           // 监控信息上报时间间隔，秒
	int    m_nMouseKeyboardAlarmInterval;      // 鼠标键盘没活动报警时间，秒
	time_t m_tMouseKeyboardLastActiveTime;     // 上次鼠标键盘活动时间，从开机算起
	static HWND m_hLastActiveHwnd;             // 上次顶层活动窗口句柄
	static time_t m_tMonitorUploadAwLastTime;  // 向web端发送活动窗口监控信息的时间
	static time_t m_tMonitorUploadMkLastTime;  // 向web端发送鼠标键盘监控信息的时间
	static BOOL   m_bMonitorUploadStatus;      // 向web端发送监控信息状态，TRUE正在发送。
	static BOOL   m_bMonitorUploadAwNext;      // 是否有未发送完成Aw日志，日质量太大，继续发送
	static BOOL   m_bMonitorUploadMkNext;      // 是否有未发送完成Mk日志，日质量太大，继续发送
	PFUN_GetLastActiveTime m_pfuncGetLastActiveTime;  // 获取由hook模块得到的鼠标键盘最后活动时间

	bool UploadMonitorInfoToWeb();  // 读取各种监控信息并发送到web端
	// 设置监控日志最后一次成功上传的最后日志时间
	void SetUploadSuccessLastTime(const string &filename, time_t time);
	// 获取监控日志上传成功时间
	time_t GetUploadSuccessLastTime(const string& filename);
	// 写入鼠标键盘监控信息
	void WriteMouseKeyboardFile(MOUSE_KEYBOARD_INFO mki, bool bOverwrite);
	// 异或加解密
	static void EncryptRotateMoveBit(char* dst, const int len, const int key);
	void DeleteExpiredFiles();

	int SendMonitorFile(void* data);

	// 把监控日志保存到本地，测试使用
	int SaveLocalFile(void* data);

	private slots:
	void CheckActiveWindow();      // 响应m_ActiveWindowTimer
	void CheckMouseKeyboard();     // 响应m_MouseKeyboardTimer
	void MonitorUpload();          // 监控上报，响应m_MonitorUploadTimer
	
	// 监控信息上传完成响应函数
	void MonitorUploadFinished(const QString& header, const QString& body, const QString& error);

};