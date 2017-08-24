#include "win-monitor.hpp"
#include "qt-wrappers.hpp"
#include "qstring.h"
#include "obs-app.hpp"
#include "obs-data.h"
#include "time.h"
#include <tlhelp32.h>
#include <sstream>
#include <fstream>
#include "remote-data.hpp"


struct obs_data_array {
	volatile long        ref;
	DARRAY(obs_data_t*)   objects;
};

const string winmons_path("xmf/obs-studio/winmons");
ACTIVE_WINDOW_INFO g_awi;                  // 当前活动窗口日志

// 获取时间标识的文件名称，默认获取本地时间，如果是服务器时间，只需修改本函数
string GenerateFilename(time_t& now, const char *extension, const char* type)
{
	char      file[256] = {};
	struct tm *cur_time;

	cur_time = localtime(&now);

	// 默认以每小时一个文件，或者以天、月为单位
	snprintf(file, sizeof(file), "%s_%04d%02d%02d%02d.%s",
		type,
		cur_time->tm_year + 1900,
		cur_time->tm_mon + 1,
		cur_time->tm_mday,
		cur_time->tm_hour,
		extension);

	stringstream dst;
	dst << winmons_path.c_str() << "/" << file;

	return string(GetConfigPathPtr(dst.str().c_str()));
}

// 枚举窗口子程，获得本次检查时的活动窗口信息
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD dwProcessId;
	DWORD dwThreadId;
	static TCHAR szWindowTitle[MAX_PATH];
	static TCHAR szWindowClassName[MAX_PATH];
	static TCHAR szProcessName[MAX_PATH];
	memset(szWindowTitle, 0, (sizeof(TCHAR)*MAX_PATH));
	memset(szWindowClassName, 0, (sizeof(TCHAR)*MAX_PATH));
	memset(szProcessName, 0, (sizeof(TCHAR)*MAX_PATH));

	GetClassName(hwnd, szWindowClassName, sizeof(szWindowClassName));
	if (0 == _tcsicmp(szWindowClassName, _T("IME")) ||
		0 == _tcsicmp(szWindowClassName, _T("MSCTFIME UI"))) {
	}
	GetWindowText(hwnd, szWindowTitle, sizeof(szWindowTitle));
	if (0 == _tcsicmp(szWindowTitle, _T(""))) {
		memcpy(szWindowTitle, _T("-"), sizeof(_T("-")));
	}

	HWND hForegroundWindow = GetForegroundWindow();
	// 只记录当期最前面的活动窗口信息
	if (hForegroundWindow != hwnd) {
		return TRUE;
	}

	// 获得活动窗口对应的进程名称
	dwThreadId = GetWindowThreadProcessId(hwnd, &dwProcessId);
	WinMonitor::GetProcessName(dwProcessId, szProcessName);

	// 只有在最前端窗口发生变化时，才做输出记录
	if (WinMonitor::GetLastActiveHwnd() != hwnd) {
#if _DEBUG
		TCHAR szBuffer[MAX_PATH];
		memset(szBuffer, 0, (sizeof(TCHAR)*MAX_PATH));
		_stprintf_s(szBuffer, MAX_PATH,
			_T("\n HWND:%08lX: WindowTitle:%s WindowClassName:%s 父窗口:%08lX, 进程名称:%s\n"),
			(unsigned long)hwnd, szWindowTitle, szWindowClassName, (unsigned long)GetParent(hwnd), szProcessName);
		OutputDebugString(szBuffer);
#endif

		if (WinMonitor::GetLastActiveHwnd() == NULL) { // 初始化状态
			g_awi.nVersion = WinMonitor::GetMonitorVersion();
			memcpy(g_awi.szTitle, szWindowTitle, (sizeof(TCHAR)*MAX_PATH));
			memcpy(g_awi.szClassName, szWindowClassName, (sizeof(TCHAR)*MAX_PATH));
			memcpy(g_awi.szProcessName, szProcessName, (sizeof(TCHAR)*MAX_PATH));
			g_awi.hwnd = hwnd;
			g_awi.tStartTime = WinMonitor::GetServerTime();
			g_awi.tEndTime = 0;
			WinMonitor::WriteActiveWindowFile(g_awi, false);
		}
		else { //结束状态
			// 记录上一条记录
			g_awi.tEndTime = WinMonitor::GetServerTime();
			WinMonitor::WriteActiveWindowFile(g_awi, true); // 更新上一条记录的结束时间
			// 新纪录前半条
			g_awi.nVersion = WinMonitor::GetMonitorVersion();
			memcpy(g_awi.szTitle, szWindowTitle, (sizeof(TCHAR)*MAX_PATH));
			memcpy(g_awi.szClassName, szWindowClassName, (sizeof(TCHAR)*MAX_PATH));
			memcpy(g_awi.szProcessName, szProcessName, (sizeof(TCHAR)*MAX_PATH));
			g_awi.hwnd = hwnd;
			g_awi.tStartTime = g_awi.tEndTime;
			g_awi.tEndTime = 0;
			WinMonitor::WriteActiveWindowFile(g_awi, false);
		}

		// 设置最后的活动窗口句柄
		WinMonitor::SetLastActiveHwnd(hwnd);
	}

	return TRUE;
}

void WinMonitor::WriteLastActiveWindowFile()
{
	if (g_awi.hwnd != NULL) {
		g_awi.tEndTime = GetServerTime();
		WinMonitor::WriteActiveWindowFile(g_awi, true);
	}
}

// 静态成员初始化
HWND WinMonitor::m_hLastActiveHwnd = NULL;
time_t WinMonitor::m_tMonitorUploadAwLastTime = 0;
time_t WinMonitor::m_tMonitorUploadMkLastTime = 0;
BOOL WinMonitor::m_bMonitorUploadStatus = FALSE;
BOOL WinMonitor::m_bMonitorUploadAwNext = FALSE;
BOOL WinMonitor::m_bMonitorUploadMkNext = FALSE;
time_t WinMonitor::m_tlanding_server_time = time(nullptr);
time_t WinMonitor::m_tlanding_client_tick_count = GetTickCount();

WinMonitor::WinMonitor()
{
	m_MouseKeyboardTimer = nullptr;
	m_ActiveWindowTimer = nullptr;
	m_MonitorUploadTimer = nullptr;
	m_MonitorUploadThread = nullptr;
	m_pfuncGetLastActiveTime = NULL;
	m_tlanding_server_time = time(nullptr);
	m_tlanding_client_tick_count = GetTickCount();
	m_nMonitorLevel = 0;
	m_record_dir = "";
	m_least_free_space_size = 100 * 1024 * 1024;  // 100MB
}

WinMonitor::~WinMonitor()
{
	WinMonitorStop();
}

// 根据进程ID获取进程名称
bool WinMonitor::GetProcessName(DWORD dwProcessId, LPTSTR lpszProcessName)
{
	bool bRet = false;
	HANDLE hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hsnap)
	{
		OutputDebugString(_T("CreateToolhelp32Snapshot调用失败\n"));
		return bRet;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hsnap, &pe);
	do {
		if (pe.th32ProcessID == dwProcessId) {
			_stprintf_s(lpszProcessName, MAX_PATH, _T("%s"), pe.szExeFile);
			bRet = true;
			break;
		}
	} while (::Process32Next(hsnap, &pe));

	::CloseHandle(hsnap);
	return bRet;
}

// 异或加解密
void WinMonitor::EncryptRotateMoveBit(char* dst, const int len, const int key)
{
	if (!dst) {
		return;
	}

	char* p = dst;
	for (int i = 0; i < len; i++) {
		*p++ ^= key;
	}
}

// 客户端监控模块的版本号，版本很多时，监控数据可能结构会发生变化，直接在上传时带上客户端版本号
int WinMonitor::GetMonitorVersion()
{
	return WINMONITOR_VERSION;
}

// 把监控获得的活动窗口信息写入本地监控文件
void WinMonitor::WriteActiveWindowFile(ACTIVE_WINDOW_INFO awi, bool bOverwrite)
{
	fstream file;
	time_t now = awi.tStartTime;
	string filename = GenerateFilename(now, "mon", "aw");
	file.open(filename.c_str(), ios_base::out | ios_base::in | ios_base::ate | ios_base::binary);
	if (!file.is_open()) {
		file.open(filename.c_str(), ios_base::out | ios_base::ate | ios_base::binary);
		if (!file.is_open()) {
			blog(LOG_ERROR, "写文件[%s]打开失败！严重错误！！", filename.c_str());
			file.close();
			return;
		}
	}

	file.seekp(0, ios::end);

	// 监控日志文件需要做到只写当期行的数据
	if (bOverwrite) {
		if (file.tellp() >= sizeof(awi)) {
			file.seekp(-(int)sizeof(awi), ios::end);
		}
	}
	EncryptRotateMoveBit((char*)&awi, sizeof(awi), 1);
	file.write((char*)&awi, sizeof(awi));
	file.flush();

	file.close();
}

// 写入鼠标键盘监控信息
void WinMonitor::WriteMouseKeyboardFile(MOUSE_KEYBOARD_INFO mki, bool bOverwrite)
{
	fstream file;
	time_t now = mki.tCheckTime;
	string filename = GenerateFilename(now, "mon", "mk");
	file.open(filename.c_str(), ios_base::out | ios_base::in | ios_base::ate | ios_base::binary);
	if (!file.is_open()) {
		file.open(filename.c_str(), ios_base::out | ios_base::ate | ios_base::binary);
		if (!file.is_open()) {
			blog(LOG_ERROR, "写文件[%s]打开失败！严重错误！！", filename.c_str());
			file.close();
			return;
		}
	}

	file.seekp(0, ios::end);

	// 监控日志文件需要做到只写当期行的数据
	if (bOverwrite) {
		if (file.tellp() >= sizeof(mki)) {
			file.seekp(-(int)sizeof(mki), ios::end);
		}
	}
	EncryptRotateMoveBit((char*)&mki, sizeof(mki), 2);
	file.write((char*)&mki, sizeof(mki));
	file.flush();

	file.close();
}

bool WinMonitor::WinMonitorStart()
{
	m_nMonitorLevel = config_get_int(GetGlobalConfig(), "WinMonitor", "MonitorLevel");

	SetLastActiveHwnd(NULL);

	CheckLiveLastTime();  // 检查并处理最有一条不完整监控日志

	// 启动监控活动窗口状态timer
	if (m_ActiveWindowTimer) {
		delete m_ActiveWindowTimer;
	}

	// 活动窗口的检查频度
	m_nActiveWindowCheckInterval = config_get_int(GetGlobalConfig(), "WinMonitor", "ActiveWindowCheckInterval");
	if (m_nActiveWindowCheckInterval <= 0) {
		m_nActiveWindowCheckInterval = 10;    // 默认10秒
	}
	m_ActiveWindowTimer = new QTimer(this);
	connect(m_ActiveWindowTimer, SIGNAL(timeout()), this, SLOT(CheckActiveWindow()));
	m_ActiveWindowTimer->start(m_nActiveWindowCheckInterval * 1000);

	// 启动监控鼠标键盘超时timer
	if (m_MouseKeyboardTimer) {
		delete m_MouseKeyboardTimer;
	}

	// 键盘鼠标不活动上报时间，秒
	m_nMouseKeyboardAlarmInterval = config_get_int(GetGlobalConfig(), "WinMonitor", "MouseKeyboardAlarmInterval");
	if (m_nMouseKeyboardAlarmInterval <= 0) {
		m_nMouseKeyboardAlarmInterval = 3 * 60;    // 默认3分钟
	}

	// 键盘鼠标的检查频度
	m_nMouseKeyboardCheckInterval = config_get_int(GetGlobalConfig(), "WinMonitor", "MouseKeyboardCheckInterval");
	if (m_nMouseKeyboardCheckInterval <= 0) {
		m_nMouseKeyboardCheckInterval = 10;    // 默认10秒
	}
	m_MouseKeyboardTimer = new QTimer(this);
	connect(m_MouseKeyboardTimer, SIGNAL(timeout()), this, SLOT(CheckMouseKeyboard()));
	m_MouseKeyboardTimer->start(m_nMouseKeyboardCheckInterval * 1000);

	m_tMouseKeyboardLastActiveTime = 0;

	HMODULE hModule = GetModuleHandleA("win-monitor.dll");
	if (!hModule) {
		OutputDebugString(_T("\n获得win-monitor.dll模块失败\n"));
		return false;
	}

	m_pfuncGetLastActiveTime = (PFUN_GetLastActiveTime)GetProcAddress(hModule, "GetLastActiveTime");
	if (!m_pfuncGetLastActiveTime) {
		OutputDebugString(_T("\n获得win-monitor.dll模块的GetLastActiveTime地址失败\n"));
		return false;
	}

	// 获取记录上报时间
	m_nMonitorUploadInterval = config_get_int(GetGlobalConfig(), "WinMonitor", "MonitorUploadInterval");
	if (m_nMonitorUploadInterval <= 0) {
		m_nMonitorUploadInterval = 3 * 60;    // 默认3分钟
	}
	
	// 启动监控记录上报timer
	m_MonitorUploadTimer = new QTimer(this);
	connect(m_MonitorUploadTimer, SIGNAL(timeout()), this, SLOT(MonitorUpload()));
	m_MonitorUploadTimer->start(m_nMonitorUploadInterval * 1000);


	// 记录[监控的最后活动时间]
	m_MonitorLiveLastTimer = new QTimer(this);
	connect(m_MonitorLiveLastTimer, SIGNAL(timeout()), this, SLOT(MonitorLiveLastTime()));
	m_MonitorLiveLastTimer->start(1000);  // 每秒记录一次

	return true;
}

bool WinMonitor::UpdateConfig()
{
	bool bRet = WinMonitorStart();
	return bRet;
}

// 响应m_ActiveWindowTimer
void WinMonitor::CheckActiveWindow()
{
	HWND hwnd = GetDesktopWindow();

	EnumWindows(EnumWindowsProc, (LPARAM)hwnd);
}

// 响应m_MouseKeyboardTimer
void WinMonitor::CheckMouseKeyboard()
{
	if (!m_pfuncGetLastActiveTime) {
		blog(LOG_ERROR, "获得win-monitor.dll模块的GetLastActiveTime地址失败！");
		return;
	}

	// 获取鼠标键盘最后活动时间
	time_t tLastActiveTime = m_pfuncGetLastActiveTime();
	if (tLastActiveTime == 0) {
		blog(LOG_INFO, "未关注鼠标键盘无响应情况！");
		return;
	}

	// 这两个调用认为无时间差
	time_t tCurrentTime = GetTickCount();  // 鼠标键盘不活动最后检查时间

	// 初始化m_mki(两次检查的时间间隔小于[日志上报时间]和[未活动超时时间])
	if (m_tMouseKeyboardLastActiveTime == 0) {
		if (tLastActiveTime == 0) {  // 正常情况不可能为0
			return;
		}

		if (tCurrentTime < tLastActiveTime) { // 正常情况不可能发生
			return;
		}

		m_mki.nVersion = GetMonitorVersion();
		m_mki.tStartTime = m_tlanding_server_time + abs(tLastActiveTime - m_tlanding_client_tick_count) / 1000;
		m_mki.tCheckTime = m_tlanding_server_time + abs(tCurrentTime - m_tlanding_client_tick_count) / 1000;

		m_tMouseKeyboardLastActiveTime = tLastActiveTime;

		return;
	}
	
	// 更新tCheckTime
	if (m_tMouseKeyboardLastActiveTime == tLastActiveTime) {
		m_mki.tCheckTime = m_tlanding_server_time + abs(tCurrentTime - m_tlanding_client_tick_count) / 1000;
	}
	else {
		m_mki.tCheckTime = m_tlanding_server_time + abs(tLastActiveTime - m_tlanding_client_tick_count) / 1000;
	}
	
	//blog(LOG_INFO, "m_mki.tCheckTime: %lld m_mki.tStartTime: %lld  CheckTime-StartTime: %d m_nMouseKeyboardAlarmInterval:%d", m_mki.tCheckTime, m_mki.tStartTime, (int)(m_mki.tCheckTime - m_mki.tStartTime), m_nMouseKeyboardAlarmInterval);
	// 按照条件记录日志
	if (int(m_mki.tCheckTime - m_mki.tStartTime) >= (m_nMouseKeyboardAlarmInterval)) {
		time_t old_time = m_tlanding_server_time + abs(m_tMouseKeyboardLastActiveTime - m_tlanding_client_tick_count) / 1000;
		if (m_mki.tStartTime == old_time) {
			WriteMouseKeyboardFile(m_mki, true);
		}
		else {
			WriteMouseKeyboardFile(m_mki, false);
		}
	}

	m_mki.nVersion = GetMonitorVersion();
	m_mki.tStartTime = m_tlanding_server_time + abs(tLastActiveTime - m_tlanding_client_tick_count) / 1000;
	m_mki.tCheckTime = m_tlanding_server_time + abs(tCurrentTime - m_tlanding_client_tick_count) / 1000;

	// 更新缓存变量
	if (m_tMouseKeyboardLastActiveTime != tLastActiveTime) {
		m_tMouseKeyboardLastActiveTime = tLastActiveTime;
	}
}

// 获取监控日志上传成功时间
time_t WinMonitor::GetUploadSuccessLastTime(const string& filename)
{
	fstream file;
	stringstream sfilename;
	time_t lasttime = 0;
	sfilename << winmons_path.c_str() << "/" << filename.c_str();  // 上次监控上传成功的时间
	string fname = string(GetConfigPathPtr(sfilename.str().c_str()));
	bool exists = os_file_exists(fname.c_str());
	if (!exists) {
		return lasttime;
	}

	file.open(fname.c_str(), ios_base::in | ios_base::binary);
	if (!file.is_open()) {
		blog(LOG_INFO, "\n读文件[%s]打开失败！\n", fname.c_str());
	}
	else {
		file.read((char*)&lasttime, sizeof(lasttime));
		EncryptRotateMoveBit((char*)&lasttime, sizeof(lasttime), 1);
	}
	file.close();

	return lasttime;
}

// 设置监控日志最后一次成功上传的最后日志时间
void WinMonitor::SetUploadSuccessLastTime(const string& filename, time_t time)
{
	time_t last_time = GetUploadSuccessLastTime(filename);
	if (last_time == time) {
		return;
	}

	fstream file;
	stringstream sfilename;
	sfilename << winmons_path.c_str() << "/" << filename.c_str();  // 上次监控上传成功的时间
	string fname = string(GetConfigPathPtr(sfilename.str().c_str()));
	file.open(fname.c_str(), ios_base::out | ios_base::ate | ios_base::binary);
	if (!file.is_open()) {
		blog(LOG_INFO, "\n文件[%s]打开失败！\n", fname.c_str());
	}
	else {
		EncryptRotateMoveBit((char*)&time, sizeof(time), 1);
		file.write((char*)&time, sizeof(time));
		file.flush();
	}
	file.close();
}

// 读取各种监控信息并发送到web端
bool WinMonitor::UploadMonitorInfoToWeb()
{
	bool bRet = false;
	obs_data_t* pmonitor = nullptr;
	obs_data_array_t* pawarray = nullptr;
	obs_data_array_t* pmkarray = nullptr;
	obs_data_array_t* psparray = nullptr;
	time_t curtime = GetServerTime();

	pawarray = (obs_data_array_t*)PreUploadAwData(curtime);
	if (!m_bMonitorUploadAwNext) {
		pmkarray = (obs_data_array_t*)PreUploadMkData(curtime);
	}

	psparray = (obs_data_array_t*)PreUploadSpData(curtime);

	if (pawarray || pmkarray || psparray) {
		pmonitor = obs_data_create();

		string user_id = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "user_id");
		obs_data_set_string(pmonitor, "user_id", user_id.c_str());

		string token("");
		const char* ptoken = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "token");
		if (ptoken) {
			token = ptoken;
		}
		obs_data_set_string(pmonitor, "token", token.c_str());

		if (pawarray) {
			obs_data_set_array(pmonitor, "aw", pawarray);
			obs_data_array_release(pawarray);
			pawarray = nullptr;
		}

		if (pmkarray) {
			obs_data_set_array(pmonitor, "mk", pmkarray);
			obs_data_array_release(pmkarray);
			pmkarray = nullptr;
		}

		if (psparray) {
			obs_data_set_array(pmonitor, "sp", psparray);
			obs_data_array_release(psparray);
			psparray = nullptr;
		}
	}

	if (pmonitor) {
		bRet = true;
		if (m_nMonitorLevel == 2001) {
			SaveLocalFile(pmonitor);  // 测试时，手动配置
		}
		SendMonitorFile(pmonitor);
		obs_data_release(pmonitor);
		pmonitor = nullptr;
	}

	return bRet;
}

void* WinMonitor::PreUploadAwData(time_t curtime)
{
	obs_data_array_t* pawarray = nullptr;   // 按版本的数组

	// 01 获取上次发送成功时间，如果获取不到，处理所有小于当前时间的文件
	fstream file;
	stringstream sfilename;
	
	m_bMonitorUploadAwNext = FALSE;
	time_t aw_lasttime = GetUploadSuccessLastTime(string(UPLOAD_SUCCESS_LASTTIME_AW));

	char cur_aw_filename[256] = {};
	char last_aw_filename[256] = {};

	struct tm *time_local;
	time_local = localtime(&curtime);
	// 默认以每小时一个文件，或者以天、月为单位
	snprintf(cur_aw_filename, sizeof(cur_aw_filename), "%s_%04d%02d%02d%02d.%s",
		"aw",
		time_local->tm_year + 1900, time_local->tm_mon + 1, time_local->tm_mday, time_local->tm_hour,
		"mon");

	if (aw_lasttime > 0) {
		// 如果已经发送过活动窗口监控日志，获得最后的发送日志文件名称
		time_local = localtime(&aw_lasttime);
		snprintf(last_aw_filename, sizeof(last_aw_filename), "%s_%04d%02d%02d%02d.%s",
			"aw",
			time_local->tm_year + 1900, time_local->tm_mon + 1, time_local->tm_mday, time_local->tm_hour,
			"mon");
	}

	// 02 遍历监控日志目录，获取需要的监控内容
	BPtr<char>       monitorDir(GetConfigPathPtr(winmons_path.c_str()));
	struct os_dirent *entry;
	os_dir_t         *dir = os_opendir(monitorDir);
	if (dir) {
		int  nAwVersion = 0;
		obs_data_t* paw = nullptr;
		obs_data_array_t* pawrecord = nullptr;  // 日志记录数组
		ACTIVE_WINDOW_INFO awi;
		int nAwRecordCount = 0;

		while ((entry = os_readdir(dir)) != NULL) {
			if (entry->directory || *entry->d_name == '.') {
				continue;
			}

			if ((strnicmp("aw_", entry->d_name, 3) == 0)) {  // 如果是活动窗口日志
				if (((aw_lasttime == 0) && (stricmp(entry->d_name, cur_aw_filename) <= 0)) // 获取所有小于等于当前时间的wa文件
					|| ((stricmp(entry->d_name, last_aw_filename) >= 0)              // 注意等于：如果最后一条记录未完成，应该不上传
						&& (stricmp(entry->d_name, cur_aw_filename) <= 0))
					) {
					sfilename.str("");
					sfilename << winmons_path.c_str() << "/" << entry->d_name;
					string filename = string(GetConfigPathPtr(sfilename.str().c_str()));

					file.open(filename.c_str(), ios_base::in | ios_base::binary);
					if (!file.is_open()) {
						blog(LOG_INFO, "\n文件[%s]打开失败！\n", filename.c_str());
						continue;
					}
					else {
						while (!file.eof()) {
							memset(&awi, 0, sizeof(awi));
							file.read((char*)&awi, sizeof(awi));
							if (file.eof()) { // 文件尾
								break;
							}
							EncryptRotateMoveBit((char*)&awi, sizeof(awi), 1);

							// 只取满足时间范围的记录
							if (awi.tStartTime < aw_lasttime) {  // 已经处理过
								continue;
							}

							// 如果监控日志记录小于当前时间，整理上传（对于等于的下次上传）
							if (awi.tStartTime > curtime) {
								break;
							}

							//// 是不完整记录，并且已经发送过一次，不再发送
							//if ((awi.tEndTime == 0) && (aw_lasttime == awi.tStartTime)) {
							//	break;
							//}

							// 整理记录
							QString sProcessName = QString((QChar*)awi.szProcessName);
							QString sTitle = QString((QChar*)awi.szTitle);
							QString sClassName = QString((QChar*)awi.szClassName);
							obs_data_t* aw_record = obs_data_create();
							obs_data_set_string(aw_record, "ProcessName", QT_TO_UTF8(sProcessName));
							obs_data_set_string(aw_record, "Title", QT_TO_UTF8(sTitle));
							obs_data_set_string(aw_record, "ClassName", QT_TO_UTF8(sClassName));
							char start[128] = {};
							char end[128] = {};
							time_local = localtime(&awi.tStartTime);
							snprintf(start, sizeof(start), "%04d-%02d-%02d %02d:%02d:%02d",
								time_local->tm_year + 1900, time_local->tm_mon + 1, time_local->tm_mday,
								time_local->tm_hour, time_local->tm_min, time_local->tm_sec);
							if (awi.tEndTime == 0) {
								snprintf(end, sizeof(end), "");
							}
							else {
								time_local = localtime(&awi.tEndTime);
								snprintf(end, sizeof(end), "%04d-%02d-%02d %02d:%02d:%02d",
									time_local->tm_year + 1900, time_local->tm_mon + 1, time_local->tm_mday,
									time_local->tm_hour, time_local->tm_min, time_local->tm_sec);
							}
							obs_data_set_string(aw_record, "StartTime", start);
							obs_data_set_string(aw_record, "EndTime", end);

							if (nAwVersion == 0) {
								nAwVersion = awi.nVersion;
							}
							// 如果下一条记录版本号不同，需要更换paw
							if (nAwVersion != awi.nVersion) {
								paw = obs_data_create();
								obs_data_set_int(paw, "Version", awi.nVersion);
								obs_data_set_array(paw, "Data", pawrecord);
								obs_data_array_release(pawrecord);
								pawrecord = nullptr;

								if (!pawarray) {
									pawarray = obs_data_array_create();
								}
								obs_data_array_push_back(pawarray, paw);
								obs_data_release(paw);
								paw = nullptr;

								nAwVersion = awi.nVersion;
							}

							if (!pawrecord)
							{
								pawrecord = obs_data_array_create();
							}
							obs_data_array_push_back(pawrecord, aw_record);
							obs_data_release(aw_record);
							aw_record = nullptr;

							// 判断当前种类日志文件大小，如果超大，先发送当前部分日志
							nAwRecordCount++;
							m_tMonitorUploadAwLastTime = awi.tStartTime;

							if ((nAwRecordCount * sizeof(ACTIVE_WINDOW_INFO)) >= UPLOAD_FILE_MAX_SIZE) {
								m_bMonitorUploadAwNext = TRUE;
								goto upload_file;
							}
						}
					}
					file.close();
				}
			}
		}
	upload_file:
		os_closedir(dir);

		if (pawrecord) {
			paw = obs_data_create();
			obs_data_set_int(paw, "Version", nAwVersion);
			obs_data_set_array(paw, "Data", pawrecord);
			obs_data_array_release(pawrecord);
			pawrecord = nullptr;

			if (!pawarray) {
				pawarray = obs_data_array_create();
			}
			obs_data_array_push_back(pawarray, paw);
			obs_data_release(paw);
			paw = nullptr;
		}
	}

	return pawarray;
}

void* WinMonitor::PreUploadMkData(time_t curtime)
{
	obs_data_array_t* pmkarray = nullptr;   // 按版本的数组

	// 01 获取上次发送成功时间，如果获取不到，处理所有小于当前时间的文件
	fstream file;
	stringstream sfilename;
	m_bMonitorUploadMkNext = FALSE;
	time_t mk_lasttime = GetUploadSuccessLastTime(string(UPLOAD_SUCCESS_LASTTIME_MK));

	char cur_mk_filename[256] = {};
	char last_mk_filename[256] = {};

	struct tm *time_local;
	time_local = localtime(&curtime);
	// 默认以每小时一个文件，或者以天、月为单位
	snprintf(cur_mk_filename, sizeof(cur_mk_filename), "%s_%04d%02d%02d%02d.%s",
		"mk",
		time_local->tm_year + 1900, time_local->tm_mon + 1, time_local->tm_mday, time_local->tm_hour,
		"mon");

	if (mk_lasttime > 0) {
		// 如果已经发送过鼠标键盘监控日志，获得最后的发送日志文件名称
		time_local = localtime(&mk_lasttime);
		snprintf(last_mk_filename, sizeof(last_mk_filename), "%s_%04d%02d%02d%02d.%s",
			"mk",
			time_local->tm_year + 1900, time_local->tm_mon + 1, time_local->tm_mday, time_local->tm_hour,
			"mon");
	}

	// 02 遍历监控日志目录，获取需要的监控内容
	BPtr<char>       monitorDir(GetConfigPathPtr(winmons_path.c_str()));
	struct os_dirent *entry;
	os_dir_t         *dir = os_opendir(monitorDir);
	if (dir) {
		int  nMkVersion = 0;
		obs_data_t* pmk = nullptr;
		obs_data_array_t* pmkrecord = nullptr;  // 日志记录数组
		int nMkRecordCount = 0;
		MOUSE_KEYBOARD_INFO old_mki;
		memset(&old_mki, 0, sizeof(old_mki));
		while ((entry = os_readdir(dir)) != NULL) {
			if (entry->directory || *entry->d_name == '.') {
				continue;
			}

			if ((strnicmp("mk_", entry->d_name, 3) == 0)) {  // 如果是键盘鼠标日志
				if (((mk_lasttime == 0) && (stricmp(entry->d_name, cur_mk_filename) <= 0))  // 获取所有小于等于当前时间的mk文件
					|| ((stricmp(entry->d_name, last_mk_filename) >= 0)    // 注意等于：如果有一条记录未完成，应该不上传
						&& (stricmp(entry->d_name, cur_mk_filename) <= 0))
					) {
					sfilename.str("");
					sfilename << winmons_path.c_str() << "/" << entry->d_name;
					string filename = string(GetConfigPathPtr(sfilename.str().c_str()));
					file.open(filename.c_str(), ios_base::in | ios_base::binary);
					if (!file.is_open()) {
						blog(LOG_INFO, "\n文件[%s]打开失败！\n", filename.c_str());
						continue;
					}
					else {
						MOUSE_KEYBOARD_INFO mki;
						while (!file.eof()) {
							memset(&mki, 0, sizeof(mki));
							file.read((char*)&mki, sizeof(mki));
							if (file.eof()) { // 文件尾
								break;
							}
							EncryptRotateMoveBit((char*)&mki, sizeof(mki), 2);

							// 只取满足时间范围的记录
							if (mki.tCheckTime <= mk_lasttime) {  // 已经处理过
								continue;
							}

							// aw_lasttime < tCheckTime <= curtime
							if (mki.tCheckTime <= curtime) {

								obs_data_t* mk_record = obs_data_create();

								char start[128] = {};
								char end[128] = {};
								time_local = localtime(&mki.tStartTime);
								snprintf(start, sizeof(start), "%04d-%02d-%02d %02d:%02d:%02d",
									time_local->tm_year + 1900, time_local->tm_mon + 1, time_local->tm_mday,
									time_local->tm_hour, time_local->tm_min, time_local->tm_sec);
								time_local = localtime(&mki.tCheckTime);
								snprintf(end, sizeof(end), "%04d-%02d-%02d %02d:%02d:%02d",
									time_local->tm_year + 1900, time_local->tm_mon + 1, time_local->tm_mday,
									time_local->tm_hour, time_local->tm_min, time_local->tm_sec);
								obs_data_set_string(mk_record, "StartTime", start);
								obs_data_set_string(mk_record, "CheckTime", end);

								// 如果是本次处理第一条记录
								if (nMkVersion == 0) {
									nMkVersion = mki.nVersion;
								}

								if (nMkVersion != mki.nVersion) {
									pmk = obs_data_create();
									obs_data_set_int(pmk, "Version", nMkVersion);
									obs_data_set_array(pmk, "Data", pmkrecord);
									obs_data_array_release(pmkrecord);
									pmkrecord = nullptr;

									if (!pmkarray) {
										pmkarray = obs_data_array_create();
									}
									obs_data_array_push_back(pmkarray, pmk);
									obs_data_release(pmk);
									pmk = nullptr;

									nMkVersion = mki.nVersion;
								}

								if (!pmkrecord) {
									pmkrecord = obs_data_array_create();
								}
								
								size_t count = obs_data_array_count(pmkrecord);  // Data 元素个数
								if ((count > 0) && (old_mki.tStartTime == mki.tStartTime)) { // 同一次提交日志，有tStartTime重复记录，把最后一条记录删除
									obs_data_array_erase(pmkrecord, count - 1);
								}
								obs_data_array_push_back(pmkrecord, mk_record);
								obs_data_release(mk_record);
								mk_record = nullptr;
								// 在文件更换文件时，上一文件最后一条记录和新文件第一条记录可能出现在一次上传日志中，
								// 需要过滤，如下类型的日志
								/*
								"mk": [
								{
									"Data": [
									{
										"CheckTime": "2017-06-14 09:59:55",
											"StartTime" : "2017-06-14 09:36:54"
									},
									{
										"CheckTime": "2017-06-14 10:00:15",
										"StartTime" : "2017-06-14 09:36:54"
									}
									],
										"Version": 1
								}
								],
								为：
								"mk": [
								{
								"Data": [
								{
								"CheckTime": "2017-06-14 10:00:15",
								"StartTime" : "2017-06-14 09:36:54"
								}
								],
								"Version": 1
								}
								],
								*/
								// 保存mki==>old_mki
								memset(&old_mki, 0, sizeof(old_mki));
								memcpy(&old_mki, &mki, sizeof(old_mki));

								// 判断当前种类日志文件大小，如果超大，先发送当前部分日志
								nMkRecordCount++;
								m_tMonitorUploadMkLastTime = mki.tCheckTime;
								if ((nMkRecordCount * sizeof(MOUSE_KEYBOARD_INFO)) >= UPLOAD_FILE_MAX_SIZE) {
									m_bMonitorUploadMkNext = TRUE;
									goto upload_file;
								}
							}
						}
					}
					file.close();
				}
			}
		}
	upload_file:
		os_closedir(dir);

		if (pmkrecord) {
			pmk = obs_data_create();
			obs_data_set_int(pmk, "Version", nMkVersion);
			obs_data_set_array(pmk, "Data", pmkrecord);
			obs_data_array_release(pmkrecord);
			pmkrecord = nullptr;

			if (!pmkarray) {
				pmkarray = obs_data_array_create();
			}
			obs_data_array_push_back(pmkarray, pmk);
			obs_data_release(pmk);
			pmk = nullptr;
		}
	}

	return pmkarray;
}

// 检查日志磁盘空间，如果%appdata%的磁盘空间太小，上报日志
void* WinMonitor::PreUploadSpData(time_t curtime)
{
	char* pDir = GetConfigPathPtr("");
	if ((pDir == nullptr) && (m_record_dir.length() < 2)) {
		return nullptr;
	}

	obs_data_array_t* psprecord = nullptr;  // 日志记录数组
	obs_data_array_t* psparray = nullptr;   // 按版本的数组
	obs_data_t* psp = nullptr;

	struct tm *time_local;
	char check_time[128] = {};
	time_local = localtime(&curtime);
	snprintf(check_time, sizeof(check_time), "%04d-%02d-%02d %02d:%02d:%02d",
		time_local->tm_year + 1900, time_local->tm_mon + 1, time_local->tm_mday,
		time_local->tm_hour, time_local->tm_min, time_local->tm_sec);

	BOOL bResult;
	unsigned _int64 i64FreeBytesToCaller = 0;
	unsigned _int64 i64TotalBytes = 0;
	unsigned _int64 i64FreeBytes = 0;
	char szDriveName[3] = {};
	memcpy(szDriveName, pDir, 2);
	bResult = GetDiskFreeSpaceExA(szDriveName,
		(PULARGE_INTEGER)&i64FreeBytesToCaller,
		(PULARGE_INTEGER)&i64TotalBytes,
		(PULARGE_INTEGER)&i64FreeBytes);
	if (bResult) {
		// 如果磁盘空间大于10MB，不做日志
		if (i64FreeBytesToCaller <= m_least_free_space_size) {
			char free_space[128] = {};
			snprintf(free_space, sizeof(free_space), "%llu KB", i64FreeBytesToCaller / 1024);
			obs_data_t* sp_record = obs_data_create();
			obs_data_set_string(sp_record, "CheckType", "AppData");
			obs_data_set_string(sp_record, "CheckTime", check_time);
			obs_data_set_string(sp_record, "DriveName", szDriveName);
			obs_data_set_string(sp_record, "FreeSpace", free_space);
			if (psprecord == nullptr) {
				psprecord = obs_data_array_create();
			}
			obs_data_array_push_back(psprecord, sp_record);
			obs_data_release(sp_record);
			sp_record = nullptr;
		}
	}
	
	if (m_record_dir.length() >= 2) {
		string driver_name = m_record_dir.substr(0, 2);
		if (strnicmp(driver_name.c_str(), szDriveName, 2) != 0)
		{
			i64FreeBytesToCaller = 0;
			i64TotalBytes = 0;
			i64FreeBytes = 0;
			bResult = GetDiskFreeSpaceExA(driver_name.c_str(),
				(PULARGE_INTEGER)&i64FreeBytesToCaller,
				(PULARGE_INTEGER)&i64TotalBytes,
				(PULARGE_INTEGER)&i64FreeBytes);
			if (bResult) {
				// 如果磁盘空间大于10MB，不做日志
				if (i64FreeBytesToCaller <= m_least_free_space_size) {
					char free_space[128] = {};
					snprintf(free_space, sizeof(free_space), "%llu KB", i64FreeBytesToCaller / 1024);
					obs_data_t* sp_record = obs_data_create();
					obs_data_set_string(sp_record, "CheckType", "RecDir");
					obs_data_set_string(sp_record, "CheckTime", check_time);
					obs_data_set_string(sp_record, "DriveName", driver_name.c_str());
					obs_data_set_string(sp_record, "FreeSpace", free_space);
					if (psprecord == nullptr) {
						psprecord = obs_data_array_create();
					}
					obs_data_array_push_back(psprecord, sp_record);
					obs_data_release(sp_record);
					sp_record = nullptr;
				}
			}
		}
	}

	if (psprecord) {
		psp = obs_data_create();
		obs_data_set_int(psp, "Version", GetMonitorVersion());
		obs_data_set_array(psp, "Data", psprecord);
		obs_data_array_release(psprecord);
		psprecord = nullptr;

		psparray = obs_data_array_create();
		obs_data_array_push_back(psparray, psp);
		obs_data_release(psp);
		psp = nullptr;
	}

	return psparray;
}

void WinMonitor::MonitorUpload()
{
	bool login_status = false;
	// 只有登陆成功，才上传监控文件，如果手动修改配置文件，不登陆，也会上传
	login_status = config_get_bool(GetGlobalConfig(), "BasicLoginWindow", "LoginStatus");
	if (login_status) {
		if (!m_bMonitorUploadStatus) {
            InterlockedExchange((long*)&m_bMonitorUploadStatus, TRUE);
			bool bRet = UploadMonitorInfoToWeb();
			if (!bRet) {
				// UploadMonitorInfoToWeb()没有向web端发送
				InterlockedExchange((long*)&m_bMonitorUploadStatus, FALSE);
			}
		}
	}
	else {
		// 容错处理，实际应该放在登陆处设置
		InterlockedExchange((long*)&m_bMonitorUploadStatus, FALSE);
	}
}

void WinMonitor::MonitorUploadFinished(const QString& header, const QString& body, const QString& error)
{
	if (header.isEmpty() || body.isEmpty()) {
		blog(LOG_ERROR, "data recv error!");
	}

	obs_data_t* returnData = obs_data_create_from_json(QT_TO_UTF8(body));
	const char *json = obs_data_get_json(returnData);  // test
	int nResult = obs_data_get_int(returnData, "rt");
	const char* sError = obs_data_get_string(returnData, "error");
	
	if (nResult > 0) {
		// 监控数据发送成功后，记录发送时间
		SetUploadSuccessLastTime(string(UPLOAD_SUCCESS_LASTTIME_AW), m_tMonitorUploadAwLastTime);
		SetUploadSuccessLastTime(string(UPLOAD_SUCCESS_LASTTIME_MK), m_tMonitorUploadMkLastTime);

		// 如果Aw或者Mk是达到最大发送记录数，检查未发送完的日志
		if (m_bMonitorUploadAwNext || m_bMonitorUploadMkNext) {
			UploadMonitorInfoToWeb();
		}

		// 删除过期文件
		DeleteExpiredFiles();
	}
	else {
		blog(LOG_WARNING, "WinMonitor::MonitorUploadFinished error:%s", sError);
	}

	obs_data_release(returnData);

	// 只有在收到消息后才可以在启动新的发送线程
	InterlockedExchange((long*)&m_bMonitorUploadStatus, FALSE);
}

void WinMonitor::DeleteExpiredFiles()
{
	struct tm *time;
	stringstream sfilename;
	time_t aw_lasttime = GetUploadSuccessLastTime(string(UPLOAD_SUCCESS_LASTTIME_AW));
	time_t mk_lasttime = GetUploadSuccessLastTime(string(UPLOAD_SUCCESS_LASTTIME_MK));

	char last_aw_filename[256] = {};
	char last_mk_filename[256] = {};

	if (aw_lasttime > 0) {
		time = localtime(&aw_lasttime);
		snprintf(last_aw_filename, sizeof(last_aw_filename), "%s_%04d%02d%02d%02d.%s",
			"aw",
			time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour,
			"mon");
	}

	if (mk_lasttime > 0) {
		time = localtime(&mk_lasttime);
		snprintf(last_mk_filename, sizeof(last_mk_filename), "%s_%04d%02d%02d%02d.%s",
			"mk",
			time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour,
			"mon");
	}

	if ((aw_lasttime == 0) && (mk_lasttime == 0)) {
		return;
	}

	BPtr<char>       monitorDir(GetConfigPathPtr(winmons_path.c_str()));
	struct os_dirent *entry;
	os_dir_t         *dir = os_opendir(monitorDir);
	vector<string>   delfilelist;
	if (dir) {
		while ((entry = os_readdir(dir)) != NULL) {
			if (entry->directory || *entry->d_name == '.') {
				continue;
			}

			// 根据文件名称判断，只有末尾是.mon的，才是要处理的日志文件
			string sname = entry->d_name;
			if (sname.length() > 4) {
				sname = sname.substr(sname.length() - 4);
			}
			if (sname.compare(".mon") != 0) {
				continue;
			}

			if ( ((strnicmp("aw_", entry->d_name, 3) == 0) && (stricmp(entry->d_name, last_aw_filename) < 0)) ||
				((strnicmp("mk_", entry->d_name, 3) == 0) && (stricmp(entry->d_name, last_mk_filename) < 0))
				) {
				sfilename.str("");
				sfilename << winmons_path.c_str() << "/" << entry->d_name;
				
				delfilelist.push_back(string(GetConfigPathPtr(sfilename.str().c_str())));
			}
		}
	}
	os_closedir(dir);

	if (!delfilelist.empty()) {
		vector<string>::iterator it = delfilelist.begin();
		for (; it != delfilelist.end(); ++it) {
			//// **********************test**************************
			//// 改文件名称
			//string newfile = (*it) + ".bak";
			//rename((*it).c_str(), newfile.c_str());
			//// **********************test**************************
			os_unlink((*it).c_str());
			blog(LOG_INFO, "del file:%s", (*it).c_str());
		}
		delfilelist.clear();
	}
}

int WinMonitor::SendMonitorFile(void* data)
{
	if (data == nullptr) {
		return -1;
	}

	const char *json = obs_data_get_json((obs_data_t*)data);
	if (!json) {
		blog(LOG_ERROR, "Failed to get JSON data for monitor upload");
		return -1;
	}

	const char* url = config_get_string(GetGlobalConfig(), "WinMonitor", "MonitorUploadUrl");
	if (!url) {
		blog(LOG_ERROR, "MonitorUploadUrl is null!");
		return -1;
	}

	std::string token("");
	const char* ptoken = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "token");
	if (ptoken) {
		token = ptoken;
	}
	
	std::string user_id("0");
	const char* sid = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "user_id");
	if (sid) {
		user_id = sid;
	}

	if (m_MonitorUploadThread) {
		delete m_MonitorUploadThread;
		m_MonitorUploadThread = nullptr;
	}
	RemoteDataThread *thread = new RemoteDataThread(
		url,
		"application/x-www-form-urlencoded" // 这个接口只能是这种形式
	);

	thread->PrepareData(string("user_id"), user_id);
	thread->PrepareData(string("token"), token);
	thread->PrepareData(string("data"), (void*)json, strlen(json));
	m_MonitorUploadThread = thread;
	connect(thread, &RemoteDataThread::Result, this, &WinMonitor::MonitorUploadFinished);
	m_MonitorUploadThread->start();

	return 0;
}

// 把监控日志保存到本地，测试使用
int WinMonitor::SaveLocalFile(void* data)
{
	if (data == nullptr) {
		return -1;
	}

	const char *json = obs_data_get_json((obs_data_t*)data);
	if (!json) {
		blog(LOG_ERROR, "Failed to get JSON data for monitor upload");
		return -1;
	}

	stringstream sfilename;
	struct tm *time_local;
	char filepart[128] = {};
	time_t t = GetServerTime();
	time_local = localtime(&t);
	snprintf(filepart, sizeof(filepart), "%04d%02d%02d%02d",
		time_local->tm_year + 1900, time_local->tm_mon + 1,
		time_local->tm_mday, time_local->tm_hour);
	sfilename.str("");
	// 上次监控上传成功的时间
	sfilename << winmons_path.c_str() << "/test" << filepart << ".mon";
	string filename = string(GetConfigPathPtr(sfilename.str().c_str()));
	fstream file;
	file.open(filename.c_str(), ios_base::out | ios_base::app);
	if (!file.is_open()) {
		blog(LOG_INFO, "\n文件[%s]打开失败！\n", filename.c_str());
		return -1;
	}
	else {
		file.write(json, strlen(json));
		file.flush();
	}
	file.close();
	return 0;
}

void WinMonitor::SetLandingServerTime(time_t tlanding_server_time, time_t tlanding_client_tick_count)
{
	m_tlanding_server_time = tlanding_server_time;
	m_tlanding_client_tick_count = tlanding_client_tick_count;
}

void WinMonitor::SetRecordDir(std::string dir, unsigned _int64 least_free_space_size)
{
	m_record_dir = dir;
	m_least_free_space_size = least_free_space_size;
}

time_t WinMonitor::GetServerTime()
{
	return m_tlanding_server_time + abs((time_t)GetTickCount() - m_tlanding_client_tick_count) / 1000;
}

void WinMonitor::WinMonitorStop()
{
	WriteLastActiveWindowFile();

	if (m_MouseKeyboardTimer) {
		delete m_MouseKeyboardTimer;
		m_MouseKeyboardTimer = nullptr;
	}
	if (m_ActiveWindowTimer) {
		delete m_ActiveWindowTimer;
		m_ActiveWindowTimer = nullptr;
	}
	if (m_MonitorUploadTimer) {
		delete m_MonitorUploadTimer;
		m_MonitorUploadTimer = nullptr;
	}
	if (m_MonitorUploadThread) {
		delete m_MonitorUploadThread;
		m_MonitorUploadThread = nullptr;
	}
}

void WinMonitor::MonitorLiveLastTime()
{
	time_t time = m_tlanding_server_time + abs(GetTickCount() - m_tlanding_client_tick_count) / 1000;

	fstream file;
	stringstream sfilename;
	if (time % 2 == 0) {
		sfilename << winmons_path.c_str() << "/" << LIVE_Last_0;
	}
	else {
		sfilename << winmons_path.c_str() << "/" << LIVE_Last_1;
	}
	
	string fname = string(GetConfigPathPtr(sfilename.str().c_str()));
	file.open(fname.c_str(), ios_base::out | ios_base::ate | ios_base::binary);
	if (!file.is_open()) {
		blog(LOG_INFO, "\n文件[%s]打开失败！\n", fname.c_str());
	}
	else {
		EncryptRotateMoveBit((char*)&time, sizeof(time), 4);
		file.write((char*)&time, sizeof(time));
		file.flush();
	}
	file.close();
}

void WinMonitor::CheckLiveLastTime()
{
	time_t time0 = 0;
	time_t time1 = 0;
	time_t time = 0;

	fstream file0;
	stringstream sfilename0;
	sfilename0 << winmons_path.c_str() << "/" << LIVE_Last_0;  // 上次监控上传成功的时间
	string fname0 = string(GetConfigPathPtr(sfilename0.str().c_str()));
	file0.open(fname0.c_str(), ios_base::in | ios_base::binary);
	if (!file0.is_open()) {
		blog(LOG_INFO, "\n读文件[%s]打开失败！\n", fname0.c_str());
	}
	else {
		file0.read((char*)&time0, sizeof(time0));
		EncryptRotateMoveBit((char*)&time0, sizeof(time0), 4);
	}
	file0.close();

	fstream file1;
	stringstream sfilename1;
	sfilename1 << winmons_path.c_str() << "/" << LIVE_Last_1;  // 上次监控上传成功的时间
	string fname1 = string(GetConfigPathPtr(sfilename1.str().c_str()));
	file1.open(fname1.c_str(), ios_base::in | ios_base::binary);
	if (!file1.is_open()) {
		blog(LOG_INFO, "\n读文件[%s]打开失败！\n", fname1.c_str());
	}
	else {
		file1.read((char*)&time1, sizeof(time1));
		EncryptRotateMoveBit((char*)&time1, sizeof(time1), 4);
	}
	file1.close();
	if ((time0 == 0) && time1 == 0) {
		return;
	}

	// 获得上次停止监控的最后时间
	time = time0 >= time1 ? time0 : time1;

	string awfilename = "", mkfilename = "";
	stringstream sfilename;
	BPtr<char>       monitorDir(GetConfigPathPtr(winmons_path.c_str()));
	struct os_dirent *entry;
	os_dir_t         *dir = os_opendir(monitorDir);
	vector<string>   delfilelist;
	if (dir) {
		while ((entry = os_readdir(dir)) != NULL) {
			if (entry->directory || *entry->d_name == '.') {
				continue;
			}

			// 根据文件名称判断，只有末尾是.mon的，才是要处理的日志文件
			string sname = entry->d_name;
			if (sname.length() > 4) {
				sname = sname.substr(sname.length() - 4);
			}
			if (sname.compare(".mon") != 0) {
				continue;
			}

			if ((strnicmp("aw_", entry->d_name, 3) == 0) && (stricmp(entry->d_name, awfilename.c_str()) > 0)) {
				awfilename = entry->d_name;
			}
			if ((strnicmp("mk_", entry->d_name, 3) == 0) && (stricmp(entry->d_name, mkfilename.c_str()) > 0)) {
				mkfilename = entry->d_name;
			}
		}
	}
	os_closedir(dir);

	if (awfilename.length() > 0) {
		sfilename.str("");
		sfilename << winmons_path.c_str() << "/" << awfilename;
		awfilename = string(GetConfigPathPtr(sfilename.str().c_str()));
	}
	if (mkfilename.length() > 0) {
		sfilename.str("");
		sfilename << winmons_path.c_str() << "/" << mkfilename;
		mkfilename = string(GetConfigPathPtr(sfilename.str().c_str()));
	}

	// 检查aw的最后一条记录是否完整，如果EndTime=0,则EndTime=time
	if (awfilename.length() > 0) {
		// 读取最后一条aw日志，如果EndTime=0，设置EndTime=time
		fstream file;
		file.open(awfilename.c_str(), ios_base::out | ios_base::in | ios_base::ate | ios_base::binary);
		if (!file.is_open()) {
			blog(LOG_ERROR, "写文件[%s]打开失败！严重错误！！", awfilename.c_str());
			return;
		}

		ACTIVE_WINDOW_INFO awi;
		file.seekp(0, ios::end);
		if (file.tellp() < sizeof(awi)) {
			return;
		}
		file.seekp(-(int)sizeof(awi), ios::end);
		file.read((char*)&awi, sizeof(awi));
		EncryptRotateMoveBit((char*)&awi, sizeof(awi), 1);
		if (awi.tEndTime == 0) {
			awi.tEndTime = time;

			file.seekp(-(int)sizeof(awi), ios::end);
			EncryptRotateMoveBit((char*)&awi, sizeof(awi), 1);
			file.write((char*)&awi, sizeof(awi));
			file.flush();
		}

		file.close();
	}

	// 检查mk最后一条记录是否完整
	// 目前不需要考虑mk的不完整日志
}