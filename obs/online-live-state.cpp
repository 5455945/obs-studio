#include "online-live-state.hpp"
#include "window-basic-main.hpp"
#include <QtCore/QDebug>
#include <QtWebSockets/QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>

QT_USE_NAMESPACE

OnlineLiveState::OnlineLiveState(const QUrl &url, QString &origin, OBSBasic *parent) :
	m_nStatus(0), m_webSocket(origin), m_main(parent),
	m_url(url), m_elapsedTime(0), m_tTimeout(10000), m_CheckTimer(this)
{
	connect(&m_webSocket, &QWebSocket::connected, this, &OnlineLiveState::onConnected, Qt::QueuedConnection);
	connect(&m_webSocket, &QWebSocket::close, this, &OnlineLiveState::onClose, Qt::QueuedConnection);
	connect(&m_webSocket, &QWebSocket::disconnected, this, &OnlineLiveState::onDisconnected, Qt::QueuedConnection);
	connect(&m_webSocket, &QWebSocket::stateChanged, this, &OnlineLiveState::onStateChanged, Qt::QueuedConnection);
	typedef void (QWebSocket:: *sslErrorsSignal)(const QList<QSslError> &);
	connect(&m_webSocket, static_cast<sslErrorsSignal>(&QWebSocket::sslErrors),this, &OnlineLiveState::onSslErrors, Qt::QueuedConnection);
	connect(&m_webSocket, &QWebSocket::textFrameReceived, this, &OnlineLiveState::OnTextFrameReceived, Qt::QueuedConnection);
	connect(&m_webSocket, &QWebSocket::binaryFrameReceived, this, &OnlineLiveState::binaryFrameReceived, Qt::QueuedConnection);
	connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &OnlineLiveState::onTextMessageReceived, Qt::QueuedConnection);
	connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this, &OnlineLiveState::onBinaryMessageReceived, Qt::QueuedConnection);
	connect(&m_webSocket, &QWebSocket::pong, this, &OnlineLiveState::onPong, Qt::QueuedConnection);
	connect(this, &OnlineLiveState::OnlineLiveMessage, m_main, &OBSBasic::OnlineLiveMessage, Qt::QueuedConnection);
	connect(&m_CheckTimer, SIGNAL(timeout()), this, SLOT(CheckOnlineLiveState()));
	
	m_webSocket.open(m_url);
}

OnlineLiveState::~OnlineLiveState()
{
	m_webSocket.disconnect();
	m_webSocket.close();
}

void OnlineLiveState::onConnected()
{
	qDebug() << "WebSocket connected";
	//m_webSocket.sendTextMessage(QStringLiteral("Hello, world!"));
}

void OnlineLiveState::onClose(QWebSocketProtocol::CloseCode closeCode, const QString &reason)
{
	qDebug() << "onClose:";
}

void OnlineLiveState::onDisconnected()
{
	qDebug() << "onDisconnected:" << "closeCode: " << m_webSocket.closeCode() << " closeReason: " << m_webSocket.closeReason();
}

void OnlineLiveState::onStateChanged(QAbstractSocket::SocketState state)
{
	qDebug() << "onStateChanged: " << state;
}

void OnlineLiveState::OnTextFrameReceived(const QString &frame, bool isLastFrame)
{
	//qDebug() << "OnTextFrameReceived:";
}

void OnlineLiveState::binaryFrameReceived(const QByteArray &frame, bool isLastFrame)
{
	//qDebug() << "binaryFrameReceived:";
}

void OnlineLiveState::onTextMessageReceived(QString message)
{
	qDebug() << "Message received:" << message;
	// "{\"type\": \"auth\",  \"status\": true}"
	// "{\"type\": \"logout\", \"msg\": \"ÖØ¸´µÇÂ¼\"}"
	QJsonObject msg = QJsonDocument::fromJson(QByteArray(message.toStdString().c_str())).object();
	if ((msg.value("type") == QJsonValue("auth")) && (msg.value("status") == QJsonValue(true)))
	{
		m_nStatus = 1;
		m_CheckTimer.start(m_tTimeout);
	}
	else if ((msg.value("type") == QJsonValue("logout")))
	{
		m_nStatus = 2;
		OnlineLiveMessage(QString("logout"), QString(""));
	}
}

void OnlineLiveState::onBinaryMessageReceived(const QByteArray &message)
{
	//qDebug() << "onBinaryMessageReceived:";
}

void OnlineLiveState::onSslErrors(const QList<QSslError> &errors)
{
	Q_UNUSED(errors);
	m_webSocket.ignoreSslErrors();
}

void OnlineLiveState::onPong(quint64 elapsedTime, const QByteArray &payload)
{
	qDebug() << "pong: " << elapsedTime << QString(payload);
	m_elapsedTime = elapsedTime;
}

void OnlineLiveState::CheckOnlineLiveState()
{
	switch (m_webSocket.state())
	{
	case QAbstractSocket::UnconnectedState:
		if (m_nStatus == 1) {
			m_webSocket.open(m_url);
		}
		break;
	case QAbstractSocket::HostLookupState:
	case QAbstractSocket::ConnectingState:
	case QAbstractSocket::ConnectedState:
		if (m_nStatus == 1) {
			m_elapsedTime = 0;
			m_webSocket.ping(QByteArray("ping"));
		}
		break;
	case QAbstractSocket::BoundState:
	case QAbstractSocket::ListeningState:
	case QAbstractSocket::ClosingState:
		break;
	}
	if (m_elapsedTime >= m_tTimeout) {
		qDebug() << "Timeout m_elapsedTime: " << m_elapsedTime;
	}
}