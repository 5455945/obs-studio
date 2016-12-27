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
	bool isOpenCVLoginFaceStop;  // �ֶ��ر�ˢ������
	bool bWebLoginThread;        // �Ƿ��Ѿ�������web��֤�߳�
	bool bLoginStatus;           // ��½״̬
	QString sLoginFailInfo;      // ��½ʧ����Ϣ
	QString sLoginData;          // ��½���ص����ݣ���Ҫ���ݸ�main
    Ui::OBSBasicLoginFace *ui;
	QPointer<OBSBasic> main;
	QPointer<RemotePostThread> OpenCVLoginFaceThread;
};

