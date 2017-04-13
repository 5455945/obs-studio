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

OBSBasicLogin::OBSBasicLogin(QWidget *parent, const QString info) :
	QDialog(parent),
	main(qobject_cast<OBSBasic*>(parent)),
	ui(new Ui::OBSBasicLogin)
{
	loginThread = nullptr;
	ui->setupUi(this);
	ui->lblErrorInfo->setVisible(false);
	ui->edtPassword->setEchoMode(QLineEdit::Password);
	//setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
	setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
	LoadLogin();
	connect(ui->cbxRememberPassword, SIGNAL(stateChanged(int)), this, SLOT(on_cbxRememberPassword_StateChanged(int)));
	connect(this, &OBSBasicLogin::LoginSucceeded, main, &OBSBasic::LoginSucceeded, Qt::QueuedConnection);

	if (!info.isEmpty()) {
		isDialog = false;
		ui->lblErrorInfo->setText(info);
		ui->lblErrorInfo->setVisible(true);
		ui->lblErrorInfo->setBackgroundRole(QPalette::HighlightedText);
		ui->lblErrorInfo->setStyleSheet("color:red");
	}
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
	// �����ѡ��״̬���ٱ���һ��
	if (ui->cbxRememberPassword->isChecked()) {
		SaveLogin();
	}

	WebLogin();
}

void OBSBasicLogin::WebLogin()
{
	ui->btnLogin->setEnabled(false);
	std::string user_name = QT_TO_UTF8(ui->edtUserName->text());
	std::string pass_word = QT_TO_UTF8(ui->edtPassword->text());
	long long current_time = (long long)time(NULL);

	// https://api.vathome.cn/user/index/login
	// POST���������
	// user_login ��¼����Email��mobile��
	// timestamp  ʱ���  �����ͣ�1970�굽���ڵ�����
	// code  ��ȫУ��   md5(��¼�� + md5(����) + ʱ��� + �� E12AAD9E3CD85��)
	// sort  ��¼���� ֵ�̶�Ϊ��live������¼�ɹ����������а���ֱ��url��ֱ������
	// ������ݣ�json��ʽ
	// { ��rt�� = >true(�ɹ�) / false(ʧ��), ��token�� = >��26λ�ַ����� , ��app�� = >������ϱ�url����Ϣ�� , ��storage�� = >���ƴ洢���������, ��error�� = >��������Ϣ�� }

	std::string token = "E12AAD9E3CD85";
	std::string url = "https://api.vathome.cn/user/index/login";
	std::string contentType = "";

	// ƴ��postData����
	std::string postData = "";
	postData += "user_login=";
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
	std::string password = MD5(pass_word).toString();
	password += "\^Vangen-2006\$";
	md5_src += MD5(password).toString();
	md5_src += str_time;
	md5_src += token;
	postData += MD5(md5_src).toString();

	postData += "&sort=";
	postData += "live";

	loginThread = new WebLoginThread(url, contentType, postData);
	connect(loginThread, &WebLoginThread::Result, this, &OBSBasicLogin::loginFinished, Qt::QueuedConnection);
	loginThread->start();
}

void OBSBasicLogin::loginFinished(const QString &text, const QString &error)
{
	ui->btnLogin->setEnabled(true);
	obs_data_t* returnData = obs_data_create_from_json(QT_TO_UTF8(text));
	const char *json = obs_data_get_json(returnData);
	bool bResult = false;
	bResult = obs_data_get_bool(returnData, "rt");
	const char* sError = obs_data_get_string(returnData, "error");
	obs_data_release(returnData);

	if (bResult) {
		emit LoginSucceeded(text);
		close();
	}
	else {
		blog(LOG_WARNING, "Bad JSON file received from server, error:%s", sError);
		ui->lblErrorInfo->setText(QApplication::translate("OBSBasicLogin", "LoginFail", 0));
		ui->lblErrorInfo->setVisible(true);
		ui->lblErrorInfo->setBackgroundRole(QPalette::HighlightedText);
		ui->lblErrorInfo->setStyleSheet("color:red");
	}
}

int OBSBasicLogin::exec()
{
	return QDialog::exec();
}

void OBSBasicLogin::done(int status)
{
	QDialog::done(status);
}

void OBSBasicLogin::accept()
{
	QDialog::accept();
}

void OBSBasicLogin::reject()
{
	LoginEnd();
	QDialog::reject();
}

bool OBSBasicLogin::close()
{
	isClose = true;
	return QDialog::close();
}

void OBSBasicLogin::LoginEnd()
{
	disconnect(this, &OBSBasicLogin::LoginSucceeded, main, &OBSBasic::LoginSucceeded);
	disconnect(ui->cbxRememberPassword, SIGNAL(stateChanged(int)), this, SLOT(on_cbxRememberPassword_StateChanged(int)));

	if (loginThread) {
		disconnect(loginThread, &WebLoginThread::Result, this, &OBSBasicLogin::loginFinished);
		loginThread->wait();
		delete loginThread;
		loginThread = nullptr;
	}

	if (!isDialog) {  // ������ǵ����Ի����½
		if (!isClose) {  // ����ǵ��X�رհ�ť
			main->show();
		}
		else {  // �������
			if (main->isHidden()) {
				main->moveShow();
			}
		}
	}
}