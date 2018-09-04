#pragma once

#include <QDialog>
#include <QPointer>

namespace Ui {
	class OBSBasicLogin;
}

class OBSBasic;
class WebLoginThread;

class OBSBasicLogin : public QDialog
{
	Q_OBJECT

signals :
	void LoginSucceeded(const QString& data);
	void LoginToMainWindow(const QString& type, const QString& context);

public:
	explicit OBSBasicLogin(QWidget *parent = 0, const QString info = QString());
	~OBSBasicLogin();

private:
	OBSBasic *main;
	QPointer<WebLoginThread> loginThread;
	Ui::OBSBasicLogin *ui;
	bool loading = true;
	bool isClose = false;  // 是调用close函数关闭，不是点击X按钮
	bool isDialog = true;  // 是对话框登陆
    bool isStartup = false;
	void ClearLogin();

	void LoadLogin();
	void SaveLogin();
	void WebLogin();
	void LoginEnd();
	void EncryptRotateMoveBit(char* dst, const int len, const int key);
    void NormalShow();
    void RemoteShow();

private slots:
	void on_btnRegister_clicked();
	void on_btnLogin_clicked();
	void on_cbxRememberPassword_StateChanged(int state);
	void loginFinished(const QString &text, const QString &error);

public Q_SLOTS:
    virtual int exec();
	virtual void done(int status);
	virtual void accept();
	virtual void reject();
	bool close();
};
