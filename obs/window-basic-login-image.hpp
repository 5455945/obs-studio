#pragma once

#include <QDialog>
#include <QPointer>
#include "qt-display.hpp"

namespace Ui {
class OBSBasicLoginImage;
}

class OBSBasic;

class OBSBasicLoginImage : public QDialog
{
    Q_OBJECT

signals :
	void LoginNormal(const QString& info);
	void LoginSucceeded(const QString& data);

public Q_SLOTS:
	virtual int exec();

public:
    explicit OBSBasicLoginImage(QWidget *parent = 0);
    ~OBSBasicLoginImage();

private:
	static void DrawPreview(void *data, uint32_t cx, uint32_t cy);
	bool LoginImage();

private slots:
	void LoginImageFinished(const QString& header, const QString& body, const QString& error);

private:
    Ui::OBSBasicLoginImage *ui;
	static QPointer<OBSBasic> main;
	QPointer<QThread> loginImageThread;
	static QPointer<OBSQTDisplay> view;
};

