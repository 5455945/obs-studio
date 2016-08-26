#pragma once

#include <QDialog>
#include <QPointer>

namespace Ui {
class OBSBasicLogin;
}

class OBSBasic;

class OBSBasicLogin : public QDialog
{
    Q_OBJECT

public:
    explicit OBSBasicLogin(QWidget *parent = 0);
    ~OBSBasicLogin();

private:
	OBSBasic *main;
	QPointer<QThread> loginThread;
    Ui::OBSBasicLogin *ui;
	bool loading = true;
	void ClearLogin();

	void LoadLogin();
	void SaveLogin();
	void WebLogin();

private slots:
	void on_btnRegister_clicked();
	void on_btnLogin_clicked();
	void on_cbxRememberPassword_StateChanged(int state);
	void loginFinished(const QString &text, const QString &error);
};
