#pragma once

#include <QDialog>
#include <QPointer>
#include "qt-display.hpp"

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
	static QPointer<OBSBasic> main;
	QPointer<QThread> loginThread;
	QPointer<QThread> FaceLoginThread;
	QPointer<QThread> FaceAddThread;
	static QPointer<OBSQTDisplay> view;
    Ui::OBSBasicLogin *ui;
	bool loading = true;
	void ClearLogin();

	void LoadLogin();
	void SaveLogin();
	void WebLogin();
	static void DrawPreview(void *data, uint32_t cx, uint32_t cy);
	bool FaceLogin();
	bool FaceAdd();
	void LoginRecvAnalysis(obs_data_t* returnData);

private slots:
	void on_btnRegister_clicked();
	void on_btnLogin_clicked();
	void on_cbxRememberPassword_StateChanged(int state);
	void loginFinished(const QString &text, const QString &error);
	void FaceLoginFinished(const QString& header, const QString& body, const QString& error);
	void FaceAddFinished(const QString& header, const QString& body, const QString& error);
};
