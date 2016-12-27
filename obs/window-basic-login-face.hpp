#pragma once

#include <QDialog>
#include <QPointer>
#include "qt-display.hpp"

namespace Ui {
class OBSBasicLoginFace;
}

class OBSBasic;
class RemotePostThread;

class OBSBasicLoginFace : public QDialog
{
    Q_OBJECT

signals :
	void LoginNormal(const QString& info);
	void LoginSucceeded(const QString& data);

public Q_SLOTS:
	virtual void open();
	virtual int exec();
	virtual void done(int status);
	virtual void accept();
	virtual void reject();
	bool close();
	void show();

public:
    explicit OBSBasicLoginFace(QWidget *parent = 0);
    ~OBSBasicLoginFace();

private:
	bool OpenCVLoginFace();
	void OpenCVLoginFaceEnd();

public slots:
	void OpenCVLoginFaceFinished(const QString& header, const QString& body, const QString& error);

protected:
	virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result);

private:
	bool isOpenCVLoginFaceStop;  // 手动关闭刷脸窗口
	bool bWebLoginThread;        // 是否已经开启到web认证线程
	bool bLoginStatus;           // 登陆状态
	QString sLoginFailInfo;      // 登陆失败信息
	QString sLoginData;          // 登陆返回的数据，需要传递给main
    Ui::OBSBasicLoginFace *ui;
	QPointer<OBSBasic> main;
	QPointer<RemotePostThread> OpenCVLoginFaceThread;
};

