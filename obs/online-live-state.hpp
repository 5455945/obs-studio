#pragma once

#include <QtCore/QObject>
#include <QtWebSockets/QWebSocket>
#include <QtNetwork/QSslError>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QWebSocket)

class OBSBasic;

class OnlineLiveState : public QObject
{
	Q_OBJECT
public:
	explicit OnlineLiveState(const QUrl &url, QString &origin, OBSBasic *parent = Q_NULLPTR);
	virtual ~OnlineLiveState();

public Q_SLOTS:
	void onClose(
		QWebSocketProtocol::CloseCode closeCode = QWebSocketProtocol::CloseCodeNormal,
		const QString &reason = QString());
	void onConnected();
	void onDisconnected();
	void onStateChanged(QAbstractSocket::SocketState state);
	void OnTextFrameReceived(const QString &frame, bool isLastFrame);
	void binaryFrameReceived(const QByteArray &frame, bool isLastFrame);
	void onTextMessageReceived(QString message);
	void onBinaryMessageReceived(const QByteArray &message);
	void onSslErrors(const QList<QSslError> &errors);
	void onPong(quint64 elapsedTime, const QByteArray &payload);

public:
	signals:
	void OnlineLiveMessage(const QString &type, const QString &context);

private slots:
	void CheckOnlineLiveState();

private:
	int        m_nStatus;
	OBSBasic*  m_main;
	QWebSocket m_webSocket;
	QUrl       m_url;
	quint64    m_elapsedTime;
	quint64    m_tTimeout;
	QTimer     m_CheckTimer;
};
