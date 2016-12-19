#include "window-basic-login.hpp"
#include "ui_OBSBasicLogin.h"
#include "qt-wrappers.hpp"
#include "window-basic-main.hpp"
#include "md5.h"
#include "obs-scene.h"
#include "web-login.hpp"
#include "remote-post.hpp"
#include "remote-data.hpp"
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QDesktopWidget>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

QPointer<OBSBasic> OBSBasicLogin::main = NULL;
QPointer<OBSQTDisplay> OBSBasicLogin::view = NULL;
using namespace std;

const string imgs_path("vhome/obs-studio/imgs");

OBSBasicLogin::OBSBasicLogin(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OBSBasicLogin)
{
	main = qobject_cast<OBSBasic*>(parent);
    ui->setupUi(this);
	ui->lblErrorInfo->setVisible(false);
	ui->edtPassword->setEchoMode(QLineEdit::Password);
	//setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
	setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
	connect(ui->cbxRememberPassword, SIGNAL(stateChanged(int)), this, SLOT(on_cbxRememberPassword_StateChanged(int)));
	LoadLogin();
	FaceLogin();
}

OBSBasicLogin::~OBSBasicLogin()
{
    delete ui;
}

void OBSBasicLogin::LoadLogin()
{
	loading = true;

	bool bRememberPassword = config_get_bool(GetGlobalConfig(),
		"BasicLoginWindow", "RememberPassword");
	ui->cbxRememberPassword->setChecked(bRememberPassword);

	const char* pUserName = config_get_string(GetGlobalConfig(),
		"BasicLoginWindow", "UserName");
	ui->edtUserName->setText(pUserName);

	const char* pPassword = config_get_string(GetGlobalConfig(),
		"BasicLoginWindow", "Password");
	ui->edtPassword->setText(pPassword);

	loading = false;
}

void OBSBasicLogin::SaveLogin()
{
	config_set_string(GetGlobalConfig(), "BasicLoginWindow", "UserName", QT_TO_UTF8(ui->edtUserName->text()));
	config_set_string(GetGlobalConfig(), "BasicLoginWindow", "Password", QT_TO_UTF8(ui->edtPassword->text()));
	config_set_bool(GetGlobalConfig(), "BasicLoginWindow", "RememberPassword", ui->cbxRememberPassword->isChecked());
}

void OBSBasicLogin::ClearLogin()
{
	config_set_string(GetGlobalConfig(), "BasicLoginWindow", "UserName", "");
	config_set_string(GetGlobalConfig(), "BasicLoginWindow", "Password", "");
	config_set_bool(GetGlobalConfig(), "BasicLoginWindow", "RememberPassword", false);
	//ui->edtUserName->setText("");
	//ui->edtPassword->setText("");
	//ui->cbxRememberPassword->setChecked(false);
}

void OBSBasicLogin::on_cbxRememberPassword_StateChanged(int state)
{
	if (state == Qt::Checked) {
		SaveLogin();
	}
	//else if (state == Qt::PartiallyChecked) {
	//	
	//}
	else {  // Qt::Unchecked
		ClearLogin();
	}
}

void OBSBasicLogin::on_btnRegister_clicked()
{
	QDesktopServices::openUrl(QUrl(QString(QLatin1String(QApplication::translate("OBSBasicLogin", "http://www.vangen.cn", 0).toStdString().c_str()))));

	close();
}

void OBSBasicLogin::on_btnLogin_clicked()
{
	// 如果是选中状态，再保存一遍
	if (ui->cbxRememberPassword->isChecked()) {
		SaveLogin();
	}

	WebLogin();
}

void OBSBasicLogin::WebLogin()
{
	ui->btnLogin->setEnabled(false);

	if (loginThread) {
		loginThread->wait();
		delete loginThread;
	}
	std::string user_name = QT_TO_UTF8(ui->edtUserName->text());
	std::string pass_word = QT_TO_UTF8(ui->edtPassword->text());
	long long current_time = (long long)time(NULL);


	// https://app.cdnunion.com/admin/index/login
	// POST传入参数：
	// admin_name 登录名
	// timestamp  时间戳  长整型，1970年到现在的秒数
	// code  安全校验   md5(登录名+md5(密码)+时间戳+’ E12AAD9E3CD85’)
	// sort  登录类型(可选) 值为 live 时，登录成功返回数据中包含直播url和直播参数
	// 账号 test
	// 密码  vangen.cn 

	std::string token = "E12AAD9E3CD85";
	std::string url = "https://app.cdnunion.com/admin/index/login";
	std::string contentType = "";

	// 拼接postData参数
	std::string postData = "";
	postData += "admin_name=";
	postData += user_name;
	
	std::stringstream sstream;
	std::string str_time;
	sstream << current_time;
	sstream >> str_time;
	postData += "&timestamp=";
	postData += str_time;

	postData += "&code=";
	// md5($username.md5($passwd).$time.$sec),
	std::string md5_src = user_name;
	md5_src += MD5(pass_word).toString();
	md5_src += str_time;
	md5_src += token;
	postData += MD5(md5_src).toString();

	postData += "&sort=";
	postData += "live";

	WebLoginThread *thread = new WebLoginThread(url, contentType, postData);
	loginThread = thread;
	connect(thread, &WebLoginThread::Result, this, &OBSBasicLogin::loginFinished);
	loginThread->start();
}

void OBSBasicLogin::loginFinished(const QString &text, const QString &error)
{
	ui->btnLogin->setEnabled(true);

	if (text.isEmpty()) {
		blog(LOG_ERROR, "Login failed");
		//blog(LOG_ERROR_TIPS, "Login failed: %s", error.toStdString().c_str());
	}

	obs_data_t* returnData = obs_data_create_from_json(QT_TO_UTF8(text));
	const char *json = obs_data_get_json(returnData);  // test
	bool bResult = false;
	bResult = obs_data_get_bool(returnData, "rt");  // 返回结果
	const char* sError = obs_data_get_string(returnData, "error");  // 错误提示

	if (bResult) {
		LoginRecvAnalysis(returnData);
		FaceAdd();
	}
	else {
		blog(LOG_WARNING, "Bad JSON file received from server, error:%s", sError);

		ui->lblErrorInfo->setText(QApplication::translate("OBSBasicLogin", "LoginFail", 0));
		ui->lblErrorInfo->setVisible(true);
		ui->lblErrorInfo->setBackgroundRole(QPalette::HighlightedText);
		ui->lblErrorInfo->setStyleSheet("color:red");
	}

	obs_data_release(returnData);
}

void OBSBasicLogin::DrawPreview(void *data, uint32_t cx, uint32_t cy)
{
	obs_sceneitem_t *sceneitem;
	QString source_name;
	QDesktopWidget* desktopWidget = QApplication::desktop();
	QRect screenRect = desktopWidget->screenGeometry();
	int monitor_width = screenRect.width();
	source_name = QStringLiteral("视频捕获设备");
	sceneitem = obs_scene_find_source(main->GetCurrentScene(), QT_TO_UTF8(source_name));
	obs_source_t* source = NULL;
	if (!sceneitem) {
		source = obs_source_create("dshow_input", QT_TO_UTF8(source_name), NULL, nullptr);
		if (source) {
			sceneitem = obs_scene_add(main->GetCurrentScene(), source);
			//uint32_t dshow_width = obs_source_get_width(sceneitem->source);
			vec2_set(&sceneitem->scale, 0.4f, 0.4f);
			vec2_set(&sceneitem->pos, monitor_width - 640 * 0.4f, 0.0f);
			obs_sceneitem_set_visible(sceneitem, true);
			main->LoginSuccessSetSource(source);
			sceneitem = obs_scene_find_source(main->GetCurrentScene(), QT_TO_UTF8(source_name));
			obs_source_release(source);
			DrawPreview(NULL, cx, cy);
		}
	}
	else {
		source = sceneitem->source;
	}

	gs_viewport_push();
	gs_projection_push();
	//gs_ortho(0.0f, 640.0f, 0.0f, 480.0f, -100.0f, 100.0f);
	//gs_set_viewport(0, 0, 640, 480);
	obs_source_video_render(source);
	gs_projection_pop();
	gs_viewport_pop();
}

bool OBSBasicLogin::FaceLogin()
{
	bool bRet = true;
	resize(660, 500);
	setMinimumSize(660, 500);
	setMaximumSize(QSize(660, 500));
	setWindowTitle(QString(QStringLiteral("开始工作 -- 请正面对着摄像头来验证您的身份信息！")));

	if (!view) {
		view = new OBSQTDisplay(this);
		setLayout(new QVBoxLayout(this));
		layout()->addWidget(view);
		layout()->setAlignment(view, Qt::AlignBottom);

		auto addDrawCallback = [this]()
		{
			obs_display_add_draw_callback(view->GetDisplay(), OBSBasicLogin::DrawPreview, this);
		};

		connect(view, &OBSQTDisplay::DisplayCreated, addDrawCallback);
	}
	view->setMinimumSize(640, 480);
	view->setMaximumSize(QSize(640, 480));
	//view->show();

	do {
		const char* pAdminId = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "admin_id");
		if (!pAdminId) {
			OutputDebugString(_T("\n无账号信息\n"));
			bRet = false;
			break;
		}

		const char* ptoken = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "token");
		if (!ptoken) {
			OutputDebugString(_T("\ntoken读取失败\n"));
			bRet = false;
			break;
		}

		HMODULE hModule = GetModuleHandleA("win-dshow.dll");
		if (!hModule) {
			OutputDebugString(_T("\n获得win-dshow.dll模块失败\n"));
			bRet = false;
			break;
		}

		typedef bool(__stdcall *PFUN_SaveCapturePicture)(bool isSave, wchar_t* filename);

		PFUN_SaveCapturePicture pfuncSaveCapturePicture = (PFUN_SaveCapturePicture)GetProcAddress(hModule, "SaveCapturePicture");
		if (!pfuncSaveCapturePicture) {
			OutputDebugString(_T("\n获得win-dshow.dll模块的SaveCapturePicture地址失败\n"));
			bRet = false;
			break;
		}

		stringstream dst;
		dst << imgs_path.c_str() << "/" << "admin_face.jpg";
		string loginFile = string(GetConfigPathPtr(dst.str().c_str()));
		wchar_t filename[MAX_PATH];
		memset(filename, 0, sizeof(wchar_t) * MAX_PATH);
		size_t len = os_utf8_to_wcs(loginFile.c_str(), 0, filename, MAX_PATH - 1);
		if (len <= 0) {
			OutputDebugString(_T("\n登陆图片文件名称获取错误！\n"));
			bRet = false;
			break;
		}

		// 尝试刷脸登陆
		int i = 0;
		for (i = 0; i < 10; i++) {
			pfuncSaveCapturePicture(true, filename);
			Sleep(200);
			if (os_file_exists(loginFile.c_str())) {
				// 如果文件存在，发送文件和用户名到web端验证
				string url = "https://app.cdnunion.com/admin/index/face_login";
				RemotePostThread *thread = new RemotePostThread(url, string());
				FaceLoginThread = thread;
				connect(thread, &RemotePostThread::Result, this, &OBSBasicLogin::FaceLoginFinished);
				thread->PrepareDataHeader();
				thread->PrepareData("token", ptoken);
				thread->PrepareData("admin_id", pAdminId);
				string timestamp = std::to_string(time(NULL));
				thread->PrepareData("timestamp", timestamp);
				thread->PrepareData("code", MD5(string(pAdminId + timestamp + "E12AAD9E3CD85")).toString());
				thread->PrepareDataFromFile("admin_face", loginFile);
				thread->PrepareDataFoot(true);
				FaceLoginThread->start();

				break;
			}
		}
	} while (false);

	if (!bRet) {
		view->hide();
		resize(340, 186);
		setMinimumSize(340, 186);
		setMaximumSize(QSize(340, 186));
		setWindowTitle(QString(QStringLiteral("开始工作")));
	}

	return bRet;
}

void OBSBasicLogin::FaceLoginFinished(const QString& header, const QString& body, const QString& error)
{
	bool bFaceLoginSuccess = false;
	if (body.isEmpty()) {
		blog(LOG_ERROR, "FaceAddFinished failed");
		return;
	}

	obs_data_t* returnData = obs_data_create_from_json(QT_TO_UTF8(body));
	const char *json = obs_data_get_json(returnData);
	bool bResult = false;
	bResult = obs_data_get_bool(returnData, "rt");
	const char* sError = obs_data_get_string(returnData, "error");
	if (bResult) {
		double confidence = obs_data_get_double(returnData, "confidence");
		bool flag = false;
		flag = obs_data_get_bool(returnData, "flag");
		if (flag) {
			bFaceLoginSuccess = true;
			LoginRecvAnalysis(returnData);
		}
	}
	if (!bFaceLoginSuccess) {
		if (view) {
			view->hide();
			resize(340, 186);
			setMinimumSize(340, 186);
			setMaximumSize(QSize(340, 186));
			setWindowTitle(QString(QStringLiteral("开始工作")));
		}
	}

	obs_data_release(returnData);
}

bool OBSBasicLogin::FaceAdd()
{
	bool bRet = true;

	const char* pAdminId = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "admin_id");
	if (!pAdminId) {
		OutputDebugString(_T("\n无账号信息\n"));
		bRet = false;
		return bRet;
	}

	const char* ptoken = config_get_string(GetGlobalConfig(), "BasicLoginWindow", "token");
	if (!ptoken) {
		OutputDebugString(_T("\ntoken读取失败\n"));
		bRet = false;
		return bRet;
	}

	HMODULE hModule = GetModuleHandleA("win-dshow.dll");
	if (!hModule) {
		OutputDebugString(_T("\n获得win-dshow.dll模块失败\n"));
		bRet = false;
		return bRet;
	}

	typedef bool(__stdcall *PFUN_SaveCapturePicture)(bool isSave, wchar_t* filename);

	PFUN_SaveCapturePicture pfuncSaveCapturePicture = (PFUN_SaveCapturePicture)GetProcAddress(hModule, "SaveCapturePicture");
	if (!pfuncSaveCapturePicture) {
		OutputDebugString(_T("\n获得win-dshow.dll模块的SaveCapturePicture地址失败\n"));
		bRet = false;
		return bRet;
	}

	stringstream dst;
	dst << imgs_path.c_str() << "/" << "admin_face.jpg";
	string loginFile = string(GetConfigPathPtr(dst.str().c_str()));
	wchar_t filename[MAX_PATH];
	memset(filename, 0, sizeof(wchar_t) * MAX_PATH);
	size_t len = os_utf8_to_wcs(loginFile.c_str(), 0, filename, MAX_PATH - 1);
	if (len <= 0) {
		OutputDebugString(_T("\n登陆图片文件名称获取错误！\n"));
		bRet = false;
		return bRet;
	}

	// 尝试刷脸登陆
	int i = 0;
	for (i = 0; i < 10; i++) {
		pfuncSaveCapturePicture(true, filename);
		Sleep(200);
		if (os_file_exists(loginFile.c_str())) {
			string url = "https://app.cdnunion.com/admin/my/face_add";

			RemotePostThread *thread = new RemotePostThread(url, string());
			FaceAddThread = thread;
			connect(thread, &RemotePostThread::Result, this, &OBSBasicLogin::FaceAddFinished);
			thread->PrepareDataHeader();
			thread->PrepareData("token", ptoken);
			thread->PrepareData("admin_id", pAdminId);
			thread->PrepareDataFromFile("admin_face", loginFile);
			thread->PrepareDataFoot(true);
			FaceAddThread->start();
			bRet = true;
			break;
		}
	}

	return bRet;
}

void OBSBasicLogin::FaceAddFinished(const QString& header, const QString& body, const QString& error)
{
	if (body.isEmpty()) {
		blog(LOG_ERROR, "FaceAddFinished failed");
	}

	obs_data_t* returnData = obs_data_create_from_json(QT_TO_UTF8(body));
	const char *json = obs_data_get_json(returnData);
	bool bResult = false;
	bResult = obs_data_get_bool(returnData, "rt");
	const char* sError = obs_data_get_string(returnData, "error");
	obs_data_release(returnData);
}

void OBSBasicLogin::LoginRecvAnalysis(obs_data_t* returnData)
{
	if (!returnData) {
		return;
	}

	std::string rtmp_server = obs_data_get_string(returnData, "live_addr");
	std::string live_param = obs_data_get_string(returnData, "live_param");  // 这个参数在20161130接口中取消了
	std::string token = obs_data_get_string(returnData, "token");
	std::string admin_id("0");
	const char* sid = obs_data_get_string(returnData, "admin_id");
	if (sid) {
		admin_id = sid;
	}
	config_set_string(GetGlobalConfig(), "BasicLoginWindow", "admin_id", admin_id.c_str());
	std::string key = "";
	size_t point = rtmp_server.rfind('/');
	if (point != std::string::npos) {
		key = rtmp_server.substr(point + 1, rtmp_server.length() - point - 1);
		rtmp_server = rtmp_server.substr(0, point);
	}

	// 登陆成功
	bool bLoginStatus = true;
	config_set_bool(GetGlobalConfig(), "BasicLoginWindow", "LoginStatus", bLoginStatus);
	// 如果登陆成功，设置登陆状态

	config_set_string(GetGlobalConfig(), "BasicLoginWindow", "server", rtmp_server.c_str());
	config_set_string(GetGlobalConfig(), "BasicLoginWindow", "key", key.c_str());
	config_set_string(GetGlobalConfig(), "BasicLoginWindow", "token", token.c_str());
	config_set_string(GetGlobalConfig(), "BasicLoginWindow", "live_param", live_param.c_str());

	// 这里采用些配置文件的方式，不修改界面
	//main->Login();
	main->SaveLoginService(rtmp_server, key);
	main->LoadService();

	obs_sceneitem_t *sceneitem;
	QString source_name;
	// 检查添加显示器捕获
	source_name = QStringLiteral("显示器捕获");
	sceneitem = obs_scene_find_source(main->GetCurrentScene(), QT_TO_UTF8(source_name));
	if (!sceneitem) {
		obs_source_t* source = obs_source_create("monitor_capture", QT_TO_UTF8(source_name), NULL, nullptr);
		if (source) {
			sceneitem = obs_scene_add(main->GetCurrentScene(), source);
			obs_sceneitem_set_visible(sceneitem, true);
			main->LoginSuccessSetSource(source);
			obs_source_release(source);
		}
	}
	// 获取显示器的宽带和高度
	//uint32_t monitor_width = obs_source_get_width(sceneitem->source);
	//uint32_t monitor_height = obs_source_get_height(sceneitem->source);
	QDesktopWidget* desktopWidget = QApplication::desktop();
	QRect screenRect = desktopWidget->screenGeometry();
	int monitor_width = screenRect.width();
	//int monitor_height = screenRect.height();

	// 检查添加视频捕获设备
	source_name = QStringLiteral("视频捕获设备");
	sceneitem = obs_scene_find_source(main->GetCurrentScene(), QT_TO_UTF8(source_name));
	if (!sceneitem) {
		obs_source_t* source = obs_source_create("dshow_input", QT_TO_UTF8(source_name), NULL, nullptr);
		if (source) {
			sceneitem = obs_scene_add(main->GetCurrentScene(), source);
			//uint32_t dshow_width = obs_source_get_width(sceneitem->source);
			vec2_set(&sceneitem->scale, 0.4f, 0.4f);
			vec2_set(&sceneitem->pos, monitor_width - 640 * 0.4f, 0.0f);
			obs_sceneitem_set_visible(sceneitem, true);
			main->LoginSuccessSetSource(source);
			sceneitem = obs_scene_find_source(main->GetCurrentScene(), QT_TO_UTF8(source_name));
			obs_source_release(source);
		}
	}

	obs_data_t* app = obs_data_get_obj(returnData, "app");  // 获取监控配置信息
	if (app) {
		int active_window_check_interval = obs_data_get_int(app, "active_window_check_interval");
		int monitor_upload_interval = obs_data_get_int(app, "monitor_upload_interval");
		int mouse_keyboard_alarm_interval = obs_data_get_int(app, "mouse_keyboard_alarm_interval");
		int mouse_keyboard_check_interval = obs_data_get_int(app, "mouse_keyboard_check_interval");
		std::string report = obs_data_get_string(app, "report");

		config_set_int(GetGlobalConfig(), "WinMonitor", "ActiveWindowCheckInterval", active_window_check_interval);
		config_set_int(GetGlobalConfig(), "WinMonitor", "MouseKeyboardCheckInterval", mouse_keyboard_check_interval);
		config_set_int(GetGlobalConfig(), "WinMonitor", "MouseKeyboardAlarmInterval", mouse_keyboard_alarm_interval);
		config_set_int(GetGlobalConfig(), "WinMonitor", "MonitorUploadInterval", monitor_upload_interval);
		config_set_string(GetGlobalConfig(), "WinMonitor", "MonitorUploadUrl", report.c_str());
	}

	obs_data_t* storage = obs_data_get_obj(returnData, "storage");  // 获取断流上传配置信息
	if (storage) {
		std::string storage_access_key = obs_data_get_string(storage, "access_key");
		std::string storage_access_secret = obs_data_get_string(storage, "access_secret");
		std::string storage_bucket = obs_data_get_string(storage, "bucket");
		std::string storage_host = obs_data_get_string(storage, "host");
		std::string storage_key = obs_data_get_string(storage, "key");
		main->setYunStorageInfo(storage_host,
			storage_access_key,
			storage_access_secret,
			"",
			storage_bucket,
			storage_key
		);
	}

	close();
	main->Login();    // 登陆成功，修改界面状态
}