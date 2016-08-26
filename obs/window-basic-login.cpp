#include "window-basic-login.hpp"
#include "ui_OBSBasicLogin.h"
#include "qt-wrappers.hpp"
#include "window-basic-main.hpp"
#include "md5.h"
#include "obs-scene.h"
#include "web-login.hpp"
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QDesktopWidget>
#include <iostream>
#include <sstream>
#include <vector>

OBSBasicLogin::OBSBasicLogin(QWidget *parent) :
    QDialog(parent),
	main(qobject_cast<OBSBasic*>(parent)),
    ui(new Ui::OBSBasicLogin)
{
    ui->setupUi(this);
	ui->lblErrorInfo->setVisible(false);
	//setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
	setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
	LoadLogin();
	connect(ui->cbxRememberPassword, SIGNAL(stateChanged(int)), this, SLOT(on_cbxRememberPassword_StateChanged(int)));
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

	//// 格式化字符串
	//std::string postDataFormat = "";
	//int len = postData.length();
	//postDataFormat.resize(3 * len + 1);
	//int j = 0;
	//for (int i = 0; i < len; i++) {
	//	if (postData[i] == '&') {
	//		postDataFormat[j++] = '%';
	//		postDataFormat[j++] = '2';
	//		postDataFormat[j++] = '6';
	//	}
	//	else if (postData[i] == '+') {
	//		postDataFormat[j++] = '%';
	//		postDataFormat[j++] = '2';
	//		postDataFormat[j++] = 'B';
	//	}
	//	else if (postData[i] == ' ') {
	//		postDataFormat[j++] = '%';
	//		postDataFormat[j++] = '2';
	//		postDataFormat[j++] = '0';
	//	}
	//	else if (postData[i] == '?') {
	//		postDataFormat[j++] = '%';
	//		postDataFormat[j++] = '3';
	//		postDataFormat[j++] = 'F';
	//	}
	//	else if (postData[i] == '%') {
	//		postDataFormat[j++] = '%';
	//		postDataFormat[j++] = '2';
	//		postDataFormat[j++] = '5';
	//	}
	//	else if (postData[i] == '=') {
	//		postDataFormat[j++] = '%';
	//		postDataFormat[j++] = '3';
	//		postDataFormat[j++] = 'D';
	//	}
	//	else if (postData[i] == '/') {
	//		postDataFormat[j++] = '%';
	//		postDataFormat[j++] = '2';
	//		postDataFormat[j++] = 'F';
	//	}
	//	else if (postData[i] == '#') {
	//		postDataFormat[j++] = '%';
	//		postDataFormat[j++] = '2';
	//		postDataFormat[j++] = '3';
	//	}
	//	else {
	//		postDataFormat[j++] = postData[i];
	//	}
	//}
	//postDataFormat.resize(j);

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
	bool bResult = false;
	bResult = obs_data_get_bool(returnData, "rt");  // 返回结果
	const char* sError = obs_data_get_string(returnData, "error");  // 错误提示

	if (bResult) {
		std::string rtmp_server = obs_data_get_string(returnData, "live_addr");
		std::string live_param = obs_data_get_string(returnData, "live_param");
		std::string token = obs_data_get_string(returnData, "token");
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
				vec2_set(&sceneitem->pos, monitor_width - 640*0.4f, 0.0f);
				obs_sceneitem_set_visible(sceneitem, true);
				main->LoginSuccessSetSource(source);
				sceneitem = obs_scene_find_source(main->GetCurrentScene(), QT_TO_UTF8(source_name));
				obs_source_release(source);
			}
		}

		close();
		main->Login();    // 登陆成功，修改界面状态
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
